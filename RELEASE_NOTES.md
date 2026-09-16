# Nimbostratus 1.3.4

Bug-fix release. No changes to the audio engine or parameters — existing
sessions load unchanged. Windows x64 and macOS universal builds attached below.

## Fixed

- **macOS: the plugin window no longer swallows the host's keyboard shortcuts.**
  With the UI open and focused, the spacebar reached the plugin instead of the
  DAW, so transport start/stop did nothing while you had Nimbostratus in front
  of you. Key presses the UI does not use are now handed back to the host, so
  the spacebar and other hotkeys behave as they do with the window closed.

  Typing a value still works: the UI holds on to the keyboard only while a
  numeric entry field is active (ctrl-click the **Pitch** knob).

<!-- TODO before tagging 1.3.4 -->
<!-- Windows is unverified. The framework drops the "key handled" flag on every -->
<!-- platform and neither the Win32 nor the X11 backend forwards unhandled keys, -->
<!-- so the same bug is likely present there - but it is untested and unfixed as -->
<!-- of this draft, and the fix is macOS-only (an Objective-C++ shim). -->
<!-- Either extend the fix to Win32 and drop the "macOS:" prefix above, or keep -->
<!-- the prefix and say plainly that Windows is unaffected / still affected. -->

## Install

- **Windows**: unzip and copy `nimbostratus.vst3` to your custom VST3 folder
  (e.g. `%USERPROFILE%\Documents\VST3`) or `C:\Program Files\Common Files\VST3`.
- **macOS**: unzip and copy `nimbostratus.vst3` to
  `~/Library/Audio/Plug-Ins/VST3`, then clear the download quarantine:
  `xattr -cr ~/Library/Audio/Plug-Ins/VST3/nimbostratus.vst3`.

Full attribution and licensing in [LICENSE](LICENSE) and the
[README](README.md).
