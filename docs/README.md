# MesenCE Libretro documentation

This directory is the authoritative documentation source for the
project. It is versioned with the emulator source and can also be
published through GitHub Pages.

> [!NOTE]
> The repository Wiki is not the source of truth. Update these files in
> the same pull request as any behavior, syntax, or workflow change.

## Getting started

| Document | Purpose |
| --- | --- |
| [Install into RetroArch](getting-started/install-retroarch.md) | Install, verify, and roll back the custom core |
| [Build on Windows](getting-started/build-windows.md) | Build the Windows x64 Libretro DLL |
| [FDS BIOS setup](getting-started/fds-bios.md) | Configure legal user-supplied FDS firmware |

## HD packs

| Document | Purpose |
| --- | --- |
| [HD-pack overview](hd-packs/README.md) | Paths, compatibility, and feature overview |
| [Syntax reference](hd-packs/syntax-reference.md) | Classic and fork-only tags |
| [Semantic rules](hd-packs/semantic-rules.md) | Native smarter-rule design and routing |
| [Smoke-test pack](hd-packs/examples/hires.native-smarter-smoke-test.txt) | Minimal semantic syntax test input |

## Development

| Document | Purpose |
| --- | --- |
| [Architecture](development/architecture.md) | Libretro, frame pacing, memory, firmware, and HD rendering |
| [Performance](development/performance.md) | Hot-path rules, test matrix, and parallax fallback |
| [Release process](development/release-process.md) | Build, hash, tag, and GitHub release checklist |

## Reference

| Document | Purpose |
| --- | --- |
| [Repository layout](reference/repository-layout.md) | What belongs in the clone and what is generated |
| [Upstream projects](reference/upstreams.md) | Source attribution and imported revisions |
| [Changelog](../CHANGELOG.md) | Version-by-version project changes |
| [Release notes](releases/README.md) | Detailed notes for published milestones |

## Preserved upstream documentation

The original imported MesenCE documents are kept under
[upstream/](upstream/README.md). They describe the standalone upstream
emulator and may not apply directly to this focused Libretro fork.
