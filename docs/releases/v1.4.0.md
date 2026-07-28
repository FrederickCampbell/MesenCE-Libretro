# MesenCE-Libretro v1.4.0

This release adds smooth, optimized HD background parallax for Windows x64.

## Smooth HD parallax

- Retains fractional NES scroll movement until HD-image sampling.
- Allows one-HD-pixel background movement at 8x instead of whole NES-pixel
  movement becoming eight-pixel jumps.
- Preserves seamless RepeatX, RepeatY, and RepeatXY wrapping.
- Preserves Gameplay scrolling behind fixed HUD splits.
- Preserves fast-forward continuity across skipped frontend frame numbers.

## Bulk background compositor

- Bulk-renders eligible lowest-priority Gameplay and RepeatX layers as
  full-width HD scanline strips.
- Replaces hundreds of tiny per-layer calls per scanline with long contiguous
  source spans.
- Uses cached image dimensions, source pointers, wrap state, and opacity.
- Uses direct copies for fully opaque, full-brightness Alpha layers.
- Keeps the previous block renderer as an automatic fallback when a background
  is not eligible for bulk rendering.
- Set MESENCE_HD_BULK_PARALLAX=0 before launch to force the fallback path.

## Compatibility

- No changes to NES RAM, game logic, PPU timing, memory watches, semantic
  conditions, the Libretro ABI, presenter timing, or save-state serialization.
- Non-HD games continue through the normal emulator path.
- Legacy, non-repeating, non-Gameplay, partial-width, and unusual-priority
  backgrounds retain the compatibility renderer.
- Windows x64 is the primary tested target.
- No formal minimum CPU is claimed. Performance remains dependent on HD scale,
  layer count, image size, and alpha coverage.
- Confirmed at full speed on an Intel Core i5-13600K with the SMB2J 8x HD pack.

## Build

The release uses the established non-LTO GCC build with `-O3`. Experimental LTO
was rejected because project-wide upstream ODR mismatches caused the Windows
link to fail.

## Assets

- `mesen_ce_nes_libretro.dll`
- `mesen_ce_nes_libretro.info`
- `mesen_ce_nes_libretro.sha256.txt`