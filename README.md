# MesenCE Libretro

An experimental NES/Famicom/Famicom Disk System Libretro core based on
[MesenCE](https://github.com/nesdev-org/MesenCE).

This fork combines the MesenCE NES/FDS emulation core with a RetroArch/Libretro
frontend, RetroAchievements-compatible memory exposure, modern Mesen HD-pack
support, FDS firmware handling, and RetroArch-owned frame pacing.

> **Status:** `v1.0.27` is the current experimental Windows x64 release. Keep
> the stock Mesen core installed alongside this core while testing.

## Highlights

- NES, Famicom, and Famicom Disk System emulation
- Separate Libretro core identity, so it does not overwrite stock Mesen
- RetroAchievements support, including Hardcore mode
- RetroArch-controlled fast-forward, slow motion, frame advance, and throttling
- Full-resolution HD-frame delivery through Libretro
- Modern Mesen HD-pack support, including addition rules
- Fork-only semantic HD-pack syntax for smaller, more maintainable packs
- Portable Windows x64 build with static GCC/C++ runtime linkage

## Core identity

Generated core DLL:

```text
mesen_ce_nes_libretro.dll
```

RetroArch display name:

```text
Nintendo - NES / Famicom / FDS (Mesen CE Experimental)
```

## Windows build

Requirements:

- Windows 10 or Windows 11
- PowerShell 7 recommended
- Git
- MSYS2 MinGW64

Build:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\BUILD_WINDOWS_X64.ps1
```

Or with an explicit MSYS2 root:

```powershell
pwsh -ExecutionPolicy Bypass -File ".\BUILD_WINDOWS_X64.ps1" `
  -MsysRoot "C:\_Hub\_Dev\Tools\MSYS2"
```

Build outputs:

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

The installer uses a separate filename and does not replace `mesen_libretro.dll`.

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

### HD-pack syntax support

This fork supports classic Mesen/MesenCE HD-pack syntax and adds fork-only
semantic syntax for reducing duplicate pack rules.

Classic tags include:

```text
<ver>, <scale>, <img>, <tile>, <condition>, <background>,
<overscan>, <patch>, <options>, <bgm>, <sfx>, <addition>
```

Fork-only semantic tags include:

```text
<page>, <watch>, <paletteSet>, <rect>, <context>,
<contextTemplate>, <route>, <replace>
```

These semantic rules are lowered into classic HD-pack rules at load time, so
existing renderer behavior is preserved while large generated packs can stay
much smaller and more readable.

The SMB2J semantic v2b HD pack was verified with:

```text
SHA-256: 9192adb59bb1c0255153cf6a915fab5398a7d3f2f83944d90714ceddddb9c1b0
Bytes:   2,749,621
Lines:   33,681
```

For the full working syntax guide, see:

```text
docs/SemanticHdPack/HD_PACK_SYNTAX_GUIDE.md
```

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

## Scope

The public core currently targets NES/FDS only. The repository retains the
broader MesenCE source layout because shared components are used by the core,
but the Libretro makefile builds the focused NES/FDS source set.

## Known limitations

- Windows x64 is the primary tested platform.
- Save-state compatibility is not promised across preview versions.
- Older HD packs may require conversion to current MesenCE pack semantics.
- Fork-only semantic HD-pack tags require this fork and are not stock MesenCE syntax.
- The RetroAchievements memory map is sufficient for tested FDS targets but
  should receive broader game coverage.
- The core remains experimental and should not replace the stock Mesen core.

## Upstream projects

- Emulation core: `nesdev-org/MesenCE`, based on release `2.2.1`
- Initial Libretro frontend: `hunterk/Mesen2`, commit `516f8ab`
- Libretro integration, NES/FDS build, HD-pack path, FDS, achievements, video,
  shutdown, pacing, and semantic HD-pack work: this project and its contributors

See [UPSTREAMS.md](UPSTREAMS.md) for attribution details.

## License

This is a derivative work of GPL-licensed upstream projects. The repository
retains the upstream license files and source notices. Distributed binaries
must be accompanied by the corresponding source under the applicable GPL terms.

This project is not an official release of MesenCE, nesdev.org, Libretro,
RetroArch, or RetroAchievements.

<!-- HD_PRIORITY_COMPOSITOR_V9_START -->
## HD-pack priority compositor

Current recommended HD renderer: `v1.3.0`

- Draws only the HD sprite candidates already present at the current NES pixel.
- Uses inherited NES sprite priority for stable back-to-front overlap.
- Keeps synthetic `<addition>` artwork attached to its parent through provenance.
- Prevents additions from contaminating native `spriteNearby` and `spriteAtPosition` checks.
- Fixes Mini Mario foot disappearance and stale transformation-frame artwork.
- Uses frame-local synthetic rule caching so pose, direction, route, and RAM-state changes resolve correctly.
- Avoids the experimental full-scene compositor's severe performance regression.
- Requires no `hires.txt` or pack-author changes.
<!-- HD_PRIORITY_COMPOSITOR_V9_END -->
