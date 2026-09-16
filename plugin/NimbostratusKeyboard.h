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

#endif // NIMBOSTRATUS_KEYBOARD_H_INCLUDED
