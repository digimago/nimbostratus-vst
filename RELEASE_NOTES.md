# Nimbostratus 1.5.0

Play the buffer from MIDI notes, the way the hardware's 1V/oct input lets a
sequencer play it. Also fixes a crash a damaged or crafted project file could
cause. Windows x64 and macOS universal builds attached below.

Existing sessions load unchanged: note input is off until you switch it on,
and the new parameters are appended after the existing ones, so automation
lanes keep pointing at the same knobs.

## Added

- **Note input (1V/oct).** A new **Notes** selector in the header lets
  incoming MIDI notes transpose the buffer, on top of the **Pitch** knob.
  Middle C plays it as recorded, C5 an octave up; the combined pitch reaches
  ±48 semitones, as on the hardware. Freeze a texture, shorten the grains and
  it becomes a playable voice.

  - **Pitch**: notes set the pitch.
  - **Pitch+Trig**: every note-on also fires **Trigger**, restarting grains,
    excitations or repeats with each note.

  Playing is monophonic: the last note wins, and letting go of it falls back
  to a note still held, without retriggering. Once every key is up the pitch
  stays on the last note, as a sequencer's CV output would; switching back to
  **Off** returns pitch to the knob. A lamp beside the selector lights while a
  note is held.

  Hosts deliver notes to an effect differently. In **Ableton Live**, add a
  MIDI track, set its **MIDI To** to the track holding Nimbostratus and pick
  Nimbostratus in the dropdown below; to follow what a synth plays, set that
  track's **MIDI From** to the synth's track. **Reaper** passes MIDI down the
  effect chain, so Nimbostratus after a synth hears the synth's notes
  directly — which is why note input starts **Off**. The README has the
  details.

## Fixed

- **Out-of-range values in a project file can no longer crash the host.**
  Parameter values restored from a saved project, or sent as CLAP events,
  reached the engine unchecked, and the engine looks values up in tables that
  assume the normal range. A project with, say, a Size far beyond its maximum
  or an unknown playback mode made the plugin read outside those tables,
  which could take the host down with it. Every value is now held to its
  range, and an invalid one falls back to the default.

- **An invalid sample rate no longer crashes the plugin.** A host reporting a
  sample rate the resampler cannot work with now gets silence instead.

## Install

- **Windows**: unzip and copy `nimbostratus.vst3` to your custom VST3 folder
  (e.g. `%USERPROFILE%\Documents\VST3`) or `C:\Program Files\Common Files\VST3`.
- **macOS**: unzip and copy `nimbostratus.vst3` to
  `~/Library/Audio/Plug-Ins/VST3`, then clear the download quarantine:
  `xattr -cr ~/Library/Audio/Plug-Ins/VST3/nimbostratus.vst3`.

Full attribution and licensing in [LICENSE](LICENSE) and the
[README](README.md).
