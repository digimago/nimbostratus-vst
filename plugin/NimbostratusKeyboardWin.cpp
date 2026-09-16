// Hands unused key presses back to the host on Windows.
//
// pugl's window procedure turns WM_KEYDOWN/WM_KEYUP/WM_SYSKEYDOWN/WM_SYSKEYUP
// and WM_CHAR into pugl events and then returns 0 for all of them, so nothing
// ever reaches the host. Inside a DAW that means an open plugin window eats
// the host's key commands - most painfully the spacebar, which normally starts
// and stops transport.
//
// The UI reports once per frame whether Dear ImGui is taking typed input (the
// pitch knob's numeric entry is the only place that happens). On the first
// report the plugin's HWND is subclassed; while text input is inactive the key
// messages are posted to the host's top-level window instead of being handed
// to pugl. Posting rather than sending puts them back through the host's own
// message loop, which is where a DAW runs its accelerator table.
//
// WM_CHAR is dropped rather than forwarded: the host's message loop runs
// TranslateMessage over the WM_KEYDOWN we posted and generates its own.
//
// The original window procedure and the capture flag live in window properties
// rather than in statics, so several open plugin windows cannot tread on each
// other's state.

// GetAncestor() needs Windows 2000 or later; respect the build's own target if
// it already picked one.
#ifndef _WIN32_WINNT
# define _WIN32_WINNT 0x0600
#endif

#include <windows.h>

#include "NimbostratusKeyboard.h"

namespace {

const char* const kProcProp    = "NimbostratusKeyProc";
const char* const kCaptureProp = "NimbostratusKeyCapture";

// pugl builds its window with the TCHAR-generic RegisterClassEx, so whether
// the window is ANSI or Unicode depends on how it was compiled. Ask the window
// itself rather than assuming, otherwise the subclass would sit on the wrong
// side of the runtime's WM_CHAR translation.
WNDPROC setWindowProc(HWND hwnd, WNDPROC proc)
{
    const LONG_PTR replaced =
        IsWindowUnicode(hwnd)
            ? SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)proc)
            : SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)proc);

    return (WNDPROC)replaced;
}

LRESULT callWindowProc(WNDPROC proc, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return IsWindowUnicode(hwnd) ? CallWindowProcW(proc, hwnd, msg, wParam, lParam)
                                 : CallWindowProcA(proc, hwnd, msg, wParam, lParam);
}

bool wantsCapture(HWND hwnd)
{
    return GetPropA(hwnd, kCaptureProp) != NULL;
}

LRESULT CALLBACK keyboardProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC const original = (WNDPROC)GetPropA(hwnd, kProcProp);
    if (original == NULL)
        return DefWindowProc(hwnd, msg, wParam, lParam);

    switch (msg)
    {
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_CHAR:
        if (! wantsCapture(hwnd))
        {
            // Only an embedded view has somewhere to forward to; a top-level
            // window of our own is already where the key belongs.
            HWND const root = GetAncestor(hwnd, GA_ROOT);
            if (root != NULL && root != hwnd)
            {
                if (msg != WM_CHAR)
                    PostMessage(root, msg, wParam, lParam);
                return 0;
            }
        }
        break;

    case WM_NCDESTROY:
        // Last message a window ever sees; drop our state even if the UI was
        // torn down in an order that skipped nimboReleaseKeyboard().
        RemovePropA(hwnd, kProcProp);
        RemovePropA(hwnd, kCaptureProp);
        setWindowProc(hwnd, original);
        return callWindowProc(original, hwnd, msg, wParam, lParam);
    }

    return callWindowProc(original, hwnd, msg, wParam, lParam);
}

} // namespace

void nimboSetKeyboardCapture(const uintptr_t nativeWindow, const bool capture)
{
    HWND const hwnd = (HWND)nativeWindow;
    if (hwnd == NULL || ! IsWindow(hwnd))
        return;

    if (GetPropA(hwnd, kProcProp) == NULL)
    {
        WNDPROC const original = setWindowProc(hwnd, keyboardProc);
        if (original == NULL)
            return;

        if (! SetPropA(hwnd, kProcProp, (HANDLE)original))
        {
            setWindowProc(hwnd, original);
            return;
        }
    }

    if (wantsCapture(hwnd) != capture)
    {
        if (capture)
            SetPropA(hwnd, kCaptureProp, (HANDLE)(LONG_PTR)1);
        else
            RemovePropA(hwnd, kCaptureProp);
    }
}

void nimboReleaseKeyboard(const uintptr_t nativeWindow)
{
    HWND const hwnd = (HWND)nativeWindow;
    if (hwnd == NULL || ! IsWindow(hwnd))
        return;

    WNDPROC const original = (WNDPROC)GetPropA(hwnd, kProcProp);
    if (original == NULL)
        return;

    RemovePropA(hwnd, kProcProp);
    RemovePropA(hwnd, kCaptureProp);
    setWindowProc(hwnd, original);
}
