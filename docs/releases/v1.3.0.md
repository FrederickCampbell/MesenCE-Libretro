# MesenCE-Libretro v1.3.0

This is the current recommended NES HD-pack renderer.

## HD priority compositor

- Orders only the HD sprite candidates already present at the current NES pixel.
- Uses inherited NES sprite priority for stable back-to-front overlap.
- Keeps synthetic `<addition>` artwork attached to its parent through provenance.
- Prevents synthetic additions from contaminating native `spriteNearby` and
  `spriteAtPosition` checks.
- Fixes the SMB2J Mini Mario foot disappearance/flashing issue.
- Fixes transformation artwork becoming stuck after Mini Mario grows into
  Super Mario by making synthetic rule-match caching frame-local.
- Avoids the discarded full-scene compositor's severe performance regression.
- Requires no `hires.txt` or HD-pack syntax changes.

## Compatibility

Based on MesenCE-Libretro upstream `v1.0.27`. Changes are isolated to the NES
HD-pack renderer.
