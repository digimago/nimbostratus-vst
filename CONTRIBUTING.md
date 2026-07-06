# Contributing to Nimbostratus

Thanks for your interest in improving Nimbostratus! This is a VST3/CLAP
granular texture processor built on the open-source Mutable Instruments–family
DSP (via the SuperParasites firmware) and the DISTRHO Plugin Framework.

By participating you agree to abide by our
[Code of Conduct](CODE_OF_CONDUCT.md).

## Ways to contribute

- **Bug reports** — open an issue with your OS, host (Ableton Live, etc.),
  plugin format (VST3/CLAP), plugin version, and clear steps to reproduce.
- **Feature ideas** — open an issue describing the musical goal, not just the
  implementation. Context helps.
- **Code** — bug fixes, new playback-mode mappings, UI/UX polish, packaging.
- **Docs** — README, tooltips, this guide.

## Getting the source

The repository uses git submodules for all third-party code (DPF, DPF-Widgets,
speexdsp, superparasites). Clone recursively:

```sh
git clone --recursive https://github.com/digimago/nimbostratus-vst.git
# or, if already cloned:
git submodule update --init --recursive
```

## Building

Any C++14 toolchain with CMake ≥ 3.24 works. No IDE is required.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Outputs land in `build/bin/` as `nimbostratus.vst3` and `nimbostratus.clap`.
On macOS, add `"-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64"` for a universal build.
CI (`.github/workflows/build.yml`) builds Windows x64 and macOS universal on
every push and pull request, so you can rely on it to check both platforms.

## Testing your change

- `test/offline_test.cc` drives the DSP through all eight playback modes and
  checks for non-silent, non-NaN output. Build and run it (see the README) and
  make sure every mode still passes.
- Where possible, validate the plugin with
  [pluginval](https://github.com/Tracktion/pluginval) at strictness level 5:
  `pluginval --strictness-level 5 --validate build/bin/nimbostratus.vst3`.
- Load the plugin in a host and confirm audio and UI behave as expected.

## Coding conventions

- Match the surrounding style (4-space indent, `k`-prefixed constants,
  `kParam*` enum ids). Keep the UI and DSP parameter lists in sync via
  `plugin/NimbostratusParams.h`.
- **Do not modify the vendored DSP** under `superparasites/`, `dpf/`,
  `dpf-widgets/`, or `speexdsp/`. Those are submodules; upstreamable fixes
  should go to the respective projects. The one local firmware patch we carry
  (a `Window::Start` fix) is documented in the README.
- **Never change the plugin's internal identifiers** — `DISTRHO_PLUGIN_URI`,
  `DISTRHO_PLUGIN_CLAP_ID`, `DISTRHO_PLUGIN_BRAND_ID`, and the unique id in
  `getUniqueId()`. They keep existing host sessions working; only the
  human-readable display name may change.
- Add a tooltip for any new control, and keep it mode-aware if the knob's
  meaning changes per playback mode.

## Branching, commits, and pull requests

`main` is always releasable; nothing lands on it directly.

1. Branch from `main`: `feature/<name>` or `fix/<name>`.
2. Keep commits focused; write clear, imperative commit subjects.
3. Open a pull request against `main`. CI must be green (Windows + macOS).
4. A maintainer reviews and merges.

## Releases and versioning

We use [semantic versioning](https://semver.org/). The version lives in exactly
one place: `project(Nimbostratus VERSION x.y.z)` in `CMakeLists.txt`, and is
compiled into the plugin via the `NIMBO_VERSION_*` defines.

- **patch** (x.y.Z): bug fixes, no parameter or behavior changes.
- **minor** (x.Y.z): new features, new parameters appended at the end.
- **major** (X.y.z): breaking changes (e.g. reordering parameter ids, which
  would break saved sessions).

To cut a release, bump the version and update `RELEASE_NOTES.md` in a PR, then
tag: `vX.Y.Z-rc.N` publishes a prerelease for testing, and `vX.Y.Z` publishes
the final release with binaries for both platforms.

## Licensing

Contributions are accepted under the project's [MIT License](LICENSE). Please
respect the attribution and trademark notes there and in the README: do not
brand the project with "Mutable Instruments" or the original module names.

## Security

Please do not open public issues for security problems — see
[SECURITY.md](SECURITY.md) for private reporting.
