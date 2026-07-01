# MesenCE Libretro

An independent NES/Famicom/Famicom Disk System Libretro core based on
[MesenCE](https://github.com/nesdev-org/MesenCE).

This project combines the MesenCE NES/FDS emulation core with a Libretro
frontend, RetroAchievements-compatible memory exposure, modern Mesen HD-pack
support, FDS firmware handling, and RetroArch-owned frame pacing.

> **Status:** `v1.0.26` is the current experimental release. Windows x64 is the
> primary tested target. Keep the stock Mesen core installed alongside this
> core while testing.

## Highlights

- NES, Famicom, and Famicom Disk System emulation
- Modern Mesen HD packs, including current addition-rule support
- RetroAchievements support, including Hardcore mode
- RetroArch-controlled fast-forward, slow motion, frame advance, and throttling
- Full-resolution HD-frame delivery through Libretro
- Portable Windows x64 build with static GCC/C++ runtime linkage
- Separate core identity, so it does not overwrite the stock Mesen core

## Core identity

The generated core is:

```text
mesen_ce_nes_libretro.dll
```

RetroArch displays it as:

```text
Nintendo - NES / Famicom / FDS (Mesen CE Experimental)
```

## Windows build

Requirements:

- Windows 10 or Windows 11
- PowerShell 7 recommended
- Git
- MSYS2 MinGW64

Run:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\BUILD_WINDOWS_X64.ps1
```

The finished files are written to:

```text
dist\mesen_ce_nes_libretro.dll
dist\mesen_ce_nes_libretro.info
```

For detailed instructions, see [BUILDING.md](BUILDING.md).

## Install into RetroArch

```powershell
.\INSTALL_TO_RETROARCH.ps1 `
    -RetroArchPath "C:\Path\To\RetroArch"
```

The installer uses a separate filename and does not replace
`mesen_libretro.dll`.

## FDS BIOS

Place a valid 8192-byte FDS BIOS at:

```text
RetroArch\system\disksys.rom
```

`FdsBios.bin` is also recognized by the underlying MesenCE firmware loader.

No BIOS is distributed with this repository.

## HD packs

Enable HD packs in the core options, then place packs under:

```text
RetroArch\system\HdPacks\<ROM filename without extension>\hires.txt
```

Example:

```text
RetroArch\system\HdPacks\Super Mario Bros. 2 Lost Levels (1986) (Japan)\hires.txt
```

HD packs and copyrighted game files are not distributed with this repository.

## RetroArch frame pacing

The Libretro execution path does not use MesenCE's standalone frame limiter.
RetroArch controls:

- normal-speed throttling
- fast-forward
- slow motion
- frame advance
- frontend synchronization

During HD-pack fast-forward, emulation may run ahead of the asynchronous HD
decoder. The core presents completed frames and allows RetroArch to duplicate
the previous frame when no new HD frame is ready.

## Upstream projects

- Emulation core: `nesdev-org/MesenCE`, based on release `2.2.1`
- Initial Libretro frontend: `hunterk/Mesen2`, commit `516f8ab`
- Libretro integration, NES/FDS build, HD-pack path, FDS, achievements, video,
  shutdown, and pacing work: this project and its contributors

See [UPSTREAMS.md](UPSTREAMS.md) for attribution details.

## Scope

The public core currently targets NES/FDS only. The repository retains the
broader MesenCE source layout because shared components are used by the core,
but the Libretro makefile builds the focused NES/FDS source set.

## Known limitations

- Windows x64 is the primary tested platform.
- Save-state compatibility is not promised across preview versions.
- Older HD packs may require conversion to current MesenCE pack semantics.
- The current RetroAchievements memory map is sufficient for the tested FDS
  target but should receive broader game coverage.
- The core remains experimental and should not replace the stock Mesen core.

## License

This is a derivative work of GPL-licensed upstream projects. The repository
retains the upstream license files and source notices. Distributed binaries
must be accompanied by the corresponding source under the applicable GPL
terms.

This project is not an official release of MesenCE, nesdev.org, Libretro,
RetroArch, or RetroAchievements.