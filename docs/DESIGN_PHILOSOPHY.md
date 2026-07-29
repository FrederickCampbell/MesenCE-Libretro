# Design philosophy

[Documentation index](README.md)

MesenCE Libretro is built around a strict separation of responsibilities.

## Preserve the emulator

MesenCE remains the authority for NES, Famicom, and FDS behavior:

- CPU execution and interrupts
- PPU timing, scrolling, OAM, palettes, and native priority
- APU and expansion-audio generation
- memory, mappers, FDS hardware, DMA, and save states

Libretro integration and HD rendering must not quietly rewrite game state or
change hardware timing to solve presentation problems.

## Let RetroArch host the core

RetroArch owns host-facing concerns:

- frame pacing and throttling
- fast-forward and slow motion
- frontend audio synchronization
- input polling
- save, system, firmware, and content paths
- RetroAchievements integration

The core supplies complete emulated frames and audio batches without adding a
second desktop-style timing controller.

## Treat HD rendering as derived state

HD-pack watches observe emulated memory. They do not patch it. Logical-object
identity, lifetime priority, semantic routes, parallax state, and replacement
caches are renderer-only data derived from the current emulated frame.

Renderer state is kept out of NES RAM, RetroAchievements-visible memory, and
save-state serialization unless a feature genuinely requires otherwise.

## Compose objects, fall back to tiles

NES hardware exposes tiles and OAM entries, but games use them to construct
logical objects. Enlarged HD artwork therefore needs object-level ownership
above the authentic tile-level PPU.

The renderer should:

1. preserve original primary-OAM identity
2. infer logical sprite groups from native geometry
3. track those groups deterministically across frames
4. give every member and synthetic child one stable object priority
5. use classic tile rendering whenever an object cannot be identified safely

Object-first rendering must extend compatibility, not replace the universal
tile fallback.

## Prefer deterministic total orders

Visual priority must remain transitive, reproducible, and independent of
temporary scanline or OAM-evaluation order. Temporal systems assign immutable
numeric tokens before sorting; comparators never mutate caches or create
pairwise priority graphs.

## Spend work at frame and object scale

NES object counts are small. A bounded once-per-frame prepass is acceptable.
Repeated temporal matching, allocation, or global searching inside every
upscaled output pixel is not.

Performance work should be guided by measured frame, decoder, renderer, and
audio-queue timings. A visual enhancement is not successful if it introduces
audio starvation, unstable fast-forward, or frame-pacing regressions.

## Keep experiments isolated

Renderer and frontend experiments belong in disposable worktrees and separate
test-core directories. The active core and main repository are updated only
after the regression matrix passes.

Each release should preserve:

- a reproducible source commit
- an exact binary checksum
- documented architecture and behavior
- a rollback path
- honest maturity labels

## Version honestly

The 0.9.x line is intentionally alpha. Earlier 1.x labels represented rapid
experimental checkpoints, not a finished compatibility contract.

`1.0.0` is reserved for a core whose non-HD behavior, HD renderer, audio/video
delivery, options, documentation, and release process are all demonstrably
stable.
