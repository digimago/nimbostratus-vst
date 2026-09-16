// macOS key-event passthrough for the plugin window. See the .mm file for why
// this is needed; on every other platform there is nothing to do and the
// header is simply never included.

#ifndef NIMBOSTRATUS_KEYBOARD_H_INCLUDED
#define NIMBOSTRATUS_KEYBOARD_H_INCLUDED

#include <cstdint>

// Tells the plugin's NSView whether the UI is currently taking typed input.
// While it is not, key presses are handed to the host instead of being
// swallowed, so transport keys such as the spacebar keep working.
//
// Safe to call every frame; the view is patched on the first call.
void nimboSetKeyboardCapture(uintptr_t nativeView, bool capture);

#endif // NIMBOSTRATUS_KEYBOARD_H_INCLUDED
