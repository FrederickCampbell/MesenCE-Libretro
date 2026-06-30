# Changelog

## 0.1.0 - Initial public preview

- Added an NES/FDS Libretro frontend based on the hunterk Mesen2 prototype.
- Ported the frontend to MesenCE 2.2.1.
- Added a focused NES/FDS Windows x64 build.
- Added RetroAchievements memory exposure and support advertisement.
- Added modern Mesen HD-pack loading through RetroArch's system directory.
- Corrected FDS firmware lookup for `system\disksys.rom`.
- Added persistent decoded-frame delivery for full-resolution HD packs.
- Corrected shutdown ordering.
- Removed MesenCE standalone frame limiting from the Libretro path.
- Added RetroArch-controlled fast-forward and slow motion behavior.
- Added Windows build, installation, and CI scripts.