# Install MesenCE Libretro into RetroArch

[Documentation index](../README.md)

## Files

Download these assets from the
[latest GitHub release](https://github.com/FrederickCampbell/MesenCE-Libretro/releases/latest):

```text
mesen_ce_nes_libretro.dll
mesen_ce_nes_libretro.info
mesen_ce_nes_libretro.sha256.txt
```

## Automatic installation

From a source checkout:

```powershell
.\INSTALL_TO_RETROARCH.ps1 -RetroArchPath "C:\Path\To\RetroArch"
```

The installer places the files at:

```text
RetroArch\cores\mesen_ce_nes_libretro.dll
RetroArch\info\mesen_ce_nes_libretro.info
```

It does not modify the stock `mesen_libretro.dll`.

## Manual installation

1. Close RetroArch.
2. Copy the DLL into `RetroArch/cores/`.
3. Copy the `.info` file into `RetroArch/info/`.
4. Reopen RetroArch and select
   `Nintendo - NES / Famicom / FDS (Mesen CE Experimental)`.

## Verify the DLL

```powershell
Get-FileHash ".\mesen_ce_nes_libretro.dll" -Algorithm SHA256
```

Compare the result with the release checksum file.

## Roll back

Keep the stock Mesen core installed. To stop using this fork, select the
stock core for the game or remove only:

```text
mesen_ce_nes_libretro.dll
mesen_ce_nes_libretro.info
```

Do not remove `mesen_libretro.dll`.
