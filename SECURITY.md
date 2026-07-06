# Security Policy

Nimbostratus is an audio effect plugin (VST3/CLAP) that runs inside a host
application. It performs no networking and reads no untrusted files at runtime,
so its attack surface is small — but plugins do load and execute inside a DAW,
so we take reports seriously. The most relevant classes of issue are:

- Memory-safety bugs in audio or preset/state handling (crashes, out-of-bounds
  reads/writes) that a malicious project file or automation stream could
  trigger.
- Supply-chain issues in the build or in a vendored dependency
  (DPF, DPF-Widgets, speexdsp, superparasites).

## Supported versions

Security fixes are provided for the latest released version only. Please make
sure you can reproduce an issue on the most recent release before reporting.

| Version | Supported |
| ------- | --------- |
| latest release (1.3.x) | ✅ |
| older releases | ❌ |

## Reporting a vulnerability

**Please do not open a public issue for security vulnerabilities.**

Report privately through GitHub's **private vulnerability reporting**: on the
repository, go to the **Security** tab → **Report a vulnerability**. This keeps
the report and discussion confidential between you and the maintainers.

Please include a description, the affected version and platform, and steps to
reproduce (a minimal project file or automation sequence if applicable). If you
have a suggested fix, feel free to include it.

## What to expect

- **Acknowledgement** within 7 days.
- An initial assessment and, if valid, a plan and rough timeline for a fix.
- Coordinated disclosure: we will agree on a disclosure date with you and
  credit you in the release notes unless you prefer to remain anonymous.

If a vulnerability originates in a vendored dependency, we will coordinate with
that upstream project and update our submodule pin once a fix is available.

Thank you for helping keep Nimbostratus and its users safe.
