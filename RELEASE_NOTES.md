# Nimbostratus 1.3.3

Maintenance and housekeeping release. No changes to the audio engine or
parameters — existing sessions load unchanged. Windows x64 and macOS universal
builds attached below.

## Changed

- **Brand name** shown by the host is now `digimago` (previously `DigiMago`),
  for consistent branding. Internal plugin identifiers are unchanged, so hosts
  still resolve existing projects to the same plugin.

## Added

- Community health files: a **Code of Conduct** (Contributor Covenant 2.1),
  **Contributing** guidelines covering the build, branch/PR/RC workflow and
  project conventions, and a **Security Policy** with private vulnerability
  reporting.

## Privacy

- Removed personal details from the source tree and documentation. Reporting
  and contact now route through GitHub's private channels rather than a
  personal address.

## Install

- **Windows**: unzip and copy `nimbostratus.vst3` to your custom VST3 folder
  (e.g. `%USERPROFILE%\Documents\VST3`) or `C:\Program Files\Common Files\VST3`.
- **macOS**: unzip and copy `nimbostratus.vst3` to
  `~/Library/Audio/Plug-Ins/VST3`, then clear the download quarantine:
  `xattr -cr ~/Library/Audio/Plug-Ins/VST3/nimbostratus.vst3`.

Full attribution and licensing in [LICENSE](LICENSE) and the
[README](README.md).
