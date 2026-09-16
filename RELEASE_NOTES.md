# Nimbostratus 1.3.4

Bug-fix release. No changes to the audio engine or parameters — existing
sessions load unchanged. Windows x64 and macOS universal builds attached below.

## Fixed

- **The plugin window no longer swallows the host's keyboard shortcuts.** With
  the UI open and focused, the spacebar reached the plugin instead of the DAW,
  so transport start/stop did nothing while you had Nimbostratus in front of
  you. Key presses the UI does not use are now handed back to the host, so the
  spacebar and other hotkeys behave as they do with the window closed. Both
  macOS and Windows were affected; both are fixed.

  Typing a value still works: the UI keeps the keyboard only while a numeric
  entry field is active (ctrl-click the **Pitch** knob).

- **The credit line at the bottom of the window is visible again.** It sat
  just below the bottom edge and had never actually been drawn. The window is
  25px taller to make room for it.

## Added

- **The plugin version is shown in the bottom-right corner of the window**, so
  it is clear at a glance which build a session is running.

## Install

- **Windows**: unzip and copy `nimbostratus.vst3` to your custom VST3 folder
  (e.g. `%USERPROFILE%\Documents\VST3`) or `C:\Program Files\Common Files\VST3`.
- **macOS**: unzip and copy `nimbostratus.vst3` to
  `~/Library/Audio/Plug-Ins/VST3`, then clear the download quarantine:
  `xattr -cr ~/Library/Audio/Plug-Ins/VST3/nimbostratus.vst3`.

Full attribution and licensing in [LICENSE](LICENSE) and the
[README](README.md).
