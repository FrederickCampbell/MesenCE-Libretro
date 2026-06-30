# Building MesenCE Libretro

## Supported build

The primary supported configuration is:

```text
Operating system: Windows x64
Toolchain:        MSYS2 MinGW64
Output:           mesen_ce_nes_libretro.dll
```

## Automated PowerShell build

From the repository root:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\BUILD_WINDOWS_X64.ps1
```

Optional parameters:

```powershell
.\BUILD_WINDOWS_X64.ps1 `
    -MsysRoot "C:\msys64" `
    -Jobs 8 `
    -Clean
```

The script installs missing MSYS2 build packages through `pacman`, builds the
Libretro target, stages the DLL and `.info` file in `dist`, checks the PE
imports, verifies mandatory Libretro exports, and writes a SHA-256 checksum.

## Direct MSYS2 build

Open an MSYS2 MinGW64 shell and run:

```bash
pacman -S --needed make git patch \
  mingw-w64-x86_64-gcc \
  mingw-w64-x86_64-binutils \
  mingw-w64-x86_64-winpthreads \
  mingw-w64-x86_64-zlib \
  mingw-w64-x86_64-libpng

make --no-print-directory -C Libretro platform=win64 LTO=false -j8
```

## Output

The Makefile writes the core under `Libretro`. The PowerShell wrapper copies
the public artifacts to:

```text
dist\mesen_ce_nes_libretro.dll
dist\mesen_ce_nes_libretro.info
dist\SHA256SUMS.txt
```

## FDS test requirements

FDS content requires a valid 8192-byte BIOS in the RetroArch system directory:

```text
RetroArch\system\disksys.rom
```

No BIOS or ROM images are included in the project.

## Clean rebuild

```powershell
.\BUILD_WINDOWS_X64.ps1 -Clean
```

This removes generated object files and the local `dist` directory. It does
not delete source files.