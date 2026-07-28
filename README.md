<h1 align="center">MesenCE Libretro</h1>

<p align="center">
  Experimental NES, Famicom, and Famicom Disk System Libretro core
  based on MesenCE, with RetroAchievements and modern HD-pack support.
</p>

<p align="center">
  <a href="https://github.com/FrederickCampbell/MesenCE-Libretro/releases/latest">
    <img alt="Latest release" src="https://img.shields.io/github/v/release/FrederickCampbell/MesenCE-Libretro?display_name=tag&sort=semver">
  </a>
  <a href="LICENSE">
    <img alt="GPL-3.0 license" src="https://img.shields.io/github/license/FrederickCampbell/MesenCE-Libretro">
  </a>
  <a href="docs/getting-started/build-windows.md">
    <img alt="Windows x64" src="https://img.shields.io/badge/platform-Windows%20x64-0078D4">
  </a>
  <a href="https://github.com/FrederickCampbell/MesenCE-Libretro">
    <img alt="NES and FDS" src="https://img.shields.io/badge/core-NES%20%7C%20Famicom%20%7C%20FDS-red">
  </a>
</p>

<p align="center">
  <a href="https://github.com/FrederickCampbell/MesenCE-Libretro/releases/latest"><strong>Download</strong></a>
  ·
  <a href="docs/README.md"><strong>Documentation</strong></a>
  ·
  <a href="docs/hd-packs/README.md"><strong>HD packs</strong></a>
  ·
  <a href=".github/CONTRIBUTING.md"><strong>Contributing</strong></a>
</p>

> [!IMPORTANT]
> `v1.4.0` is the current tested Windows x64 release. This fork uses the
> separate `mesen_ce_nes_libretro.dll` identity and does not overwrite
> RetroArch's stock `mesen_libretro.dll`. Keep both cores installed while
> the fork remains experimental.

## Why this fork exists

MesenCE Libretro combines the MesenCE NES/FDS emulation core with a
focused RetroArch frontend. It adds or repairs:

- RetroAchievements-compatible memory exposure
- FDS firmware lookup through RetroArch's system directory
- full-resolution HD-frame delivery
- RetroArch-owned frame pacing and fast-forward behavior
- current Mesen/MesenCE HD-pack compatibility
- fork-only semantic HD-pack rules for large maintainable packs
- seamless, gameplay-scrolled, smooth multi-layer parallax backgrounds
- a separate core name, DLL, and RetroArch display identity

## Quick start

1. Download the DLL and `.info` file from the
   [latest release](https://github.com/FrederickCampbell/MesenCE-Libretro/releases/latest).
2. Copy the DLL to `RetroArch/cores/` and the `.info` file to
   `RetroArch/info/`.
3. For FDS games, provide your own valid `disksys.rom` under
   `RetroArch/system/`.

The included installer can perform step 2:

```powershell
.\INSTALL_TO_RETROARCH.ps1 -RetroArchPath "C:\Path\To\RetroArch"
```

See the [installation guide](docs/getting-started/install-retroarch.md)
for verification and rollback instructions.

## HD-pack extensions

Classic Mesen/MesenCE HD-pack syntax remains supported. This fork also
includes semantic routing tags such as:

```text
<page>, <watch>, <paletteSet>, <rect>, <context>,
<contextTemplate>, <route>, <replace>
```

Background extensions include seamless `RepeatX`, `RepeatY`, and
`RepeatXY` wrapping plus `Gameplay` scrolling. Eligible lowest-priority
gameplay layers use the v1.4.0 bulk strip compositor for smooth
HD-pixel parallax, with an automatic compatibility fallback.

Read the [HD-pack guide](docs/hd-packs/README.md) or go directly to the
[syntax reference](docs/hd-packs/syntax-reference.md).

## Documentation

| Area | Guide |
| --- | --- |
| Install the core | [RetroArch installation](docs/getting-started/install-retroarch.md) |
| Build from source | [Windows x64 build](docs/getting-started/build-windows.md) |
| Configure FDS | [FDS BIOS setup](docs/getting-started/fds-bios.md) |
| Author HD packs | [HD-pack documentation](docs/hd-packs/README.md) |
| Understand the port | [Architecture](docs/development/architecture.md) |
| Test performance | [Performance guide](docs/development/performance.md) |
| Publish a release | [Release process](docs/development/release-process.md) |
| Browse every document | [Documentation index](docs/README.md) |

## Compatibility and scope

- The public Libretro target currently focuses on NES/Famicom/FDS.
- Windows x64 is the primary tested platform.
- No formal minimum CPU is claimed. HD performance depends on scale,
  image dimensions, alpha coverage, and active layer count.
- Save-state compatibility is not promised across experimental releases.
- Fork-only semantic tags require this fork.
- ROMs, BIOS files, copyrighted HD packs, and game assets are not included.

## Upstream and license

This is an independent derivative project based on
[MesenCE](https://github.com/nesdev-org/MesenCE) and the initial
[hunterk/Mesen2](https://github.com/hunterk/Mesen2) Libretro prototype.
Attribution and imported revisions are documented in
[upstreams](docs/reference/upstreams.md).

The repository is distributed under the terms in [LICENSE](LICENSE).
It is not an official release of MesenCE, nesdev.org, Libretro,
RetroArch, or RetroAchievements.
