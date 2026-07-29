# MesenCE Libretro documentation

This directory is the authoritative documentation source for the fork.
Behavior, syntax, architecture, and release changes should be documented in the
same commit as the code that implements them.

## Start here

| Document | Purpose |
| --- | --- |
| [Architecture](development/architecture.md) | Complete fork-specific system map |
| [Design philosophy](DESIGN_PHILOSOPHY.md) | The boundaries and principles used for fork development |
| [HD renderer](development/hd-renderer.md) | Tile capture, logical objects, additions, and priority |
| [Audio and video](development/audio-video.md) | APU mixing, Libretro delivery, pacing, and hitch analysis |
| [HD-pack overview](hd-packs/README.md) | Paths, compatibility, semantic syntax, and parallax |
| [Release process](development/release-process.md) | Build, verify, version, tag, and publish |
| [Roadmap](ROADMAP.md) | Work planned after the 0.9.1 alpha |
| [Project history](history/project-history.md) | Consolidated development milestones |
| [0.9.1 Alpha 1 notes](releases/v0.9.1-alpha.1.md) | Current alpha release |

## Getting started

| Document | Purpose |
| --- | --- |
| [Install into RetroArch](getting-started/install-retroarch.md) | Install, verify, and roll back the custom core |
| [Build on Windows](getting-started/build-windows.md) | Build the Windows x64 DLL |
| [FDS BIOS setup](getting-started/fds-bios.md) | Configure user-supplied FDS firmware |

## HD-pack authoring

| Document | Purpose |
| --- | --- |
| [HD-pack overview](hd-packs/README.md) | Feature overview and runtime model |
| [Syntax reference](hd-packs/syntax-reference.md) | Classic and fork-only tags |
| [Semantic rules](hd-packs/semantic-rules.md) | Pages, watches, routes, contexts, and replacements |
| [Examples](hd-packs/examples/) | Minimal test packs and syntax examples |

## Reference

| Document | Purpose |
| --- | --- |
| [Repository layout](reference/repository-layout.md) | Source versus generated/local material |
| [Upstream projects](reference/upstreams.md) | Attribution and imported revisions |
| [Changelog](../CHANGELOG.md) | Public version history |
| [Preserved upstream documentation](upstream/README.md) | Standalone MesenCE material that may not apply directly |

The GitHub Wiki is not the source of truth.