// Key-event passthrough for the plugin window. The host's window framework
// hands every key press to whichever view has focus and never offers the ones
// the UI ignores back to the DAW, so an open plugin window swallows transport
// keys. Implemented per platform in NimbostratusKeyboardMac.mm and
// NimbostratusKeyboardWin.cpp; no other platform needs it, and the header is
// simply not included there.

#ifndef NIMBOSTRATUS_KEYBOARD_H_INCLUDED
#define NIMBOSTRATUS_KEYBOARD_H_INCLUDED

#include <cstdint>

// Tells the plugin's native window whether the UI is currently taking typed
// input. While it is not, key presses are handed to the host instead of being
// swallowed, so transport keys such as the spacebar keep working.
//
// Safe to call every frame; the window is patched on the first call.
void nimboSetKeyboardCapture(uintptr_t nativeWindow, bool capture);

// Undoes that patching. Call before the native window goes away, so nothing
// points into this binary once it can be unloaded.
void nimboReleaseKeyboard(uintptr_t nativeWindow);

// What the native hook has actually seen. Hosts differ in how they route keys
// to a plugin window - some go through the plugin API, some straight to the
// native window, some keep the keys to themselves - and none of that is
// visible from the outside. The UI surfaces this so the routing can be read
// off the screen instead of guessed at.
struct NimboKeyStats
{
    uint32_t seen;       // key messages the hook observed
    uint32_t forwarded;  // ... of those, how many went to the host
    uint32_t lastKey;    // native key code of the most recent one
    bool     hooked;     // the hook is installed on the window
    char     target[96]; // where forwarded messages are aimed
};

void nimboGetKeyStats(uintptr_t nativeWindow, NimboKeyStats& stats);

#endif // NIMBOSTRATUS_KEYBOARD_H_INCLUDED
