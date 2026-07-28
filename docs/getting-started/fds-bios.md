# FDS BIOS setup

[Documentation index](../README.md)

Famicom Disk System content requires a valid user-supplied 8192-byte
firmware image.

Place it at:

```text
RetroArch\system\disksys.rom
```

The underlying MesenCE firmware loader also recognizes `FdsBios.bin`.

> [!WARNING]
> No BIOS or FDS game image is distributed with this repository. Do not
> commit firmware or ROM files. The repository ignore policy blocks
> common firmware and ROM names.

After installation, launch an FDS game and confirm that disk-side
switching and RetroAchievements memory exposure behave normally.
