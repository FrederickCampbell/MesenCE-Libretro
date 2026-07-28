# Changelog

## 1.4.0

- Added HD-pixel smooth gameplay parallax at 8x scale.
- Added the bulk strip compositor for eligible repeating gameplay layers.
- Preserved the previous block renderer as an automatic compatibility fallback.
- Added an environment toggle for renderer A/B testing.
- Kept the supported Windows release build on the stable non-LTO `-O3` path.
- See [detailed release notes](docs/releases/v1.4.0.md).

## 1.3.0

- Promoted the lightweight HD priority compositor.
- Preserved local-candidate rendering and provenance-linked additions.
- Fixed Mini Mario foot flashing and stale transformation artwork.
- See [detailed release notes](docs/releases/v1.3.0.md).

## 1.0.28-hd-parallax-v111

- Added seamless background wrapping and gameplay-based scrolling.
- Preserved gameplay-scroll continuity through RetroArch fast-forward.
- See [detailed release notes](docs/releases/v1.0.28-hd-parallax-v111.md).

## 1.0.27

- Added fork-only semantic HD-pack tags and lowering to classic rules.
- See [detailed release notes](docs/releases/v1.0.27.md).

## 1.0.26

- Fixed RetroArch cheat support.
- Added expanded System, Video, Audio, Input, Enhancements, and Advanced core options.
- Added clear descriptions and recommended values for each option.

## 1.0.25

- Replaced the universal 15 ms decoded-frame polling delay with immediate event-driven synchronization.
- Restored full-speed normal gameplay for NES and FDS content with and without HD packs.
- Added clean fast-forward exit synchronization before the first normal-speed frame.
- Preserved partially accepted Libretro audio batches in a bounded pending buffer.
- Trimmed excessive fast-forward audio latency with a short resume ramp.
- Added frontend capability handling for duplicate video frames.
- Updated the reported core version to 1.0.25.

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
