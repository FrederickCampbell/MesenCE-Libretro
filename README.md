# MesenCE Libretro

**Alpha NES/Famicom/FDS Libretro core based on MesenCE.**

Current release line: **0.9.1 Alpha**
Primary tested target: **Windows x64 + RetroArch**
[GitHub Releases](https://github.com/FrederickCampbell/MesenCE-Libretro/releases)

> [!IMPORTANT]
> This is an independent experimental fork. It uses the separate
> `mesen_ce_nes_libretro.dll` identity and can remain installed beside
> RetroArch's stock Mesen core.

## What this fork adds

- RetroAchievements-compatible NES/FDS memory exposure
- RetroArch system-directory FDS firmware and HD-pack lookup
- Full-resolution MesenCE HD-frame delivery
- RetroArch-owned pacing, fast-forward, and duplicate-frame handling
- Bounded Libretro audio short-write preservation
- Classic Mesen/MesenCE HD-pack compatibility
- Semantic HD-pack routing for maintainable large packs
- Seamless gameplay-scrolled parallax backgrounds
- Stable whole-object HD sprite priority across OAM rotation and overlap

The 0.9.1 alpha renderer recognizes common native 2x2 and 2x3 metasprites by
geometry, tracks their identity across frames, and gives every native member and
synthetic `<addition>` child one immutable lifetime priority. This fixes
whole-object flashing and midpoint priority swaps without changing NES PPU
timing, RAM, save states, or game logic.

## Install

1. Download `mesen_ce_nes_libretro.dll` and `mesen_ce_nes_libretro.info` from the latest alpha release.
2. Copy the DLL into `RetroArch\cores\`.
3. Copy the `.info` file into `RetroArch\info\`.
4. For FDS content, supply your own valid `disksys.rom` under `RetroArch\system\`.
5. Put HD packs under `RetroArch\system\HdPacks\<ROM name>\`.

The included installer can copy and verify the core:

```powershell
.\INSTALL_TO_RETROARCH.ps1 -RetroArchPath 'C:\Path\To\RetroArch'
```

## Documentation

Start with the [documentation index](docs/README.md).

| Topic | Document |
| --- | --- |
| Architecture | [docs/development/architecture.md](docs/development/architecture.md) |
| Design philosophy | [docs/DESIGN_PHILOSOPHY.md](docs/DESIGN_PHILOSOPHY.md) |
| HD renderer | [docs/development/hd-renderer.md](docs/development/hd-renderer.md) |
| Audio and video delivery | [docs/development/audio-video.md](docs/development/audio-video.md) |
| HD packs | [docs/hd-packs/README.md](docs/hd-packs/README.md) |
| Build and release | [docs/development/release-process.md](docs/development/release-process.md) |
| Roadmap | [docs/ROADMAP.md](docs/ROADMAP.md) |
| Changelog | [CHANGELOG.md](CHANGELOG.md) |

## Scope and compatibility

- Public work is focused on NES, Famicom, and FDS.
- Windows x64 is the primary validated build.
- Existing classic HD packs remain supported.
- Fork-only semantic tags require this core.
- Save-state compatibility is not guaranteed across alpha releases.
- ROMs, BIOS files, copyrighted HD packs, and game assets are not distributed.

## Upstream and license

This project is derived from MesenCE and the earlier hunterk Mesen2 Libretro
prototype. See [upstream attribution](docs/reference/upstreams.md).

Distributed under the terms in [LICENSE](LICENSE). This is not an official
release of MesenCE, nesdev.org, Libretro, RetroArch, or RetroAchievements.