# Upstream projects and attribution

## MesenCE

Repository:

```text
https://github.com/nesdev-org/MesenCE
```

Imported base:

```text
MesenCE 2.2.1
```

MesenCE supplies the emulator core, NES/FDS implementation, HD-pack renderer,
audio/video components, settings model, and shared utilities.

## hunterk/Mesen2 Libretro prototype

Repository:

```text
https://github.com/hunterk/Mesen2
```

Imported frontend revision:

```text
516f8ab
```

That prototype supplied the initial Libretro adapter used as the starting
point for this port.

## This project

This project adapts the frontend to current MesenCE and adds or repairs:

- NES/FDS-only Libretro source selection
- Windows MinGW portability
- portable static runtime linkage
- RetroAchievements memory maps and capability advertisement
- MesenCE HD-pack loading through RetroArch's system directory
- FDS firmware path handling
- persistent decoded-frame presentation
- safe core shutdown
- RetroArch-owned frame pacing and fast-forward behavior
- build, installation, and diagnostic tooling

## Independence statement

MesenCE Libretro is an independent derivative project. It is not an official
release of MesenCE, nesdev.org, Libretro, RetroArch, or RetroAchievements.