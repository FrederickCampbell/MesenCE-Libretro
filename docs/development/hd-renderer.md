# HD renderer and logical-object priority

[Documentation index](../README.md)

## Why tile-only rendering becomes unstable

NES hardware exposes background tiles and OAM sprites, not game-level entities
such as a Koopa or mushroom. An enlarged HD replacement can extend far beyond
the opaque footprint of its original 8x8 tile. Several enlarged replacements
can therefore overlap in regions where the native PPU never had to choose a
winner.

Native OAM priority alone is not enough for those additional HD pixels.

## Stable original-OAM ownership

The fork preserves the primary hardware OAM index while sprites are copied into
secondary OAM and loaded for a scanline. HD candidates and synthetic
`<addition>` children inherit that stable root rather than a temporary scanline
slot.

This prevents an object from changing identity as scanline evaluation changes.

## Native metasprite detection

The 0.9.1 alpha performs a once-per-frame native-resolution prepass. It gathers
one representative tile origin for each visible primary-OAM entry and detects
common 2x2 and 2x3 layouts by geometry.

Detection is independent of OAM member ordering:

- members are validated on an exact 8-pixel grid
- the group anchor is the minimum screen X/Y
- signatures are canonicalized by screen-space member order
- TL/TR/BL/BR, column-major, reversed, and rotated OAM allocation all resolve
  to the same logical layout

Unrecognized sprites remain valid one-member objects and use normal tile
fallback behavior.

## Temporal tracking

Current-frame objects are matched globally against previous-frame tracks using:

- group size and member count
- canonical tile/palette/flip signatures
- predicted position
- velocity consistency
- deterministic track and spatial tie-breaks

A continuing object keeps an immutable priority token for its visible lifetime.
All native members and inherited additions use that token. Sorting therefore
remains a strict total order and does not mutate tracking state.

This fixes the former midpoint swap where two complete objects reversed priority
as their X anchors crossed.

## Reset behavior

Renderer tracks reset when:

- frame progression moves backward
- frames become discontinuous
- an object disappears
- a large teleport or warp invalidates continuity
- a new emulator/pack session begins

The tracker is renderer-only and allocation-free in the frame hot path.

## Pack compatibility

No new pack syntax is required for the priority fix. Classic tile rules,
semantic rules, backgrounds, and additions continue working.

A future object-replacement layer may allow pack authors to define and dump
whole logical sprites. That should be object-first with tile fallback, not a
replacement for background tile rendering.