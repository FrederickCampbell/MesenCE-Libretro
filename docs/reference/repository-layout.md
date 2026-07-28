# Repository layout and local hygiene

[Documentation index](../README.md)

## Tracked project content

```text
Core/          Emulator and NES/FDS HD-renderer source
Libretro/      Libretro frontend and makefile
info/          Core information file
docs/          Authoritative project documentation
.github/       Community files, issue forms, and workflows
UI/            Preserved upstream UI source and assets
Utilities/     Shared upstream utilities
Lua/           Embedded Lua source
SevenZip/      Embedded archive source
Windows/       Windows platform source
Linux/         Linux platform source
MacOS/         macOS platform source
Sdl/           SDL platform source
InteropDLL/    Upstream interop source
```

## Important root files

```text
README.md
CHANGELOG.md
LICENSE
BUILD_WINDOWS_X64.ps1
INSTALL_TO_RETROARCH.ps1
makefile.libretro
Mesen.sln
```

## Generated local output

These files are rebuildable and ignored:

```text
*.o, *.obj, *.d, *.dep, *.map
Libretro/mesen_ce_nes_libretro.dll
build-transcript.txt
build/, out/, artifacts/, release-stage/
.vs/, bin/, obj/, Debug/, Release/, x64/, x86/
```

`dist/` is also ignored but may be retained locally because it contains
the exact tested release artifacts.

## Never delete manually

- `.git/`
- tracked source directories
- tracked UI dependencies or source assets
- `docs/`
- `info/`
- `LICENSE`

Use `git status`, `git ls-files`, and `git check-ignore` to classify
files. Do not use `git clean -fdx` on this repository.
