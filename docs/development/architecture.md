# Fork architecture

[Documentation index](../README.md)

## Layer model

The project has three relevant layers:

1. **MesenCE foundation**: CPU, PPU, APU, mappers, FDS, memory manager,
   save states, video decoder, and the original HD-pack implementation.
2. **MesenCE Libretro port**: RetroArch lifecycle, input, audio/video callbacks,
   core options, folders, firmware lookup, RetroAchievements memory maps, and
   frontend-owned pacing.
3. **Fork renderer extensions**: semantic HD rules, repeating gameplay
   backgrounds, bulk parallax composition, stable original-OAM ownership, and
   native metasprite lifetime tracking.

The fork does not replace NES emulation. It changes how MesenCE is hosted and
how HD replacement artwork is interpreted and composed.

## Frame execution

`retro_run()` polls input and advances one complete MesenCE frame. The CPU is
the execution driver; mapper, APU, DMA, interrupts, controllers, and PPU advance
at their normal emulated timing. The PPU finishes a `RenderedFrame`, the video
decoder produces the frontend image, and the Libretro layer submits video and
frame-sized audio batches to RetroArch.

RetroArch is the pacing authority. The core must not add an independent
standalone frame limiter.

## CPU memory

`NesMemoryManager` routes CPU reads and writes through handler tables. The
physical 2 KiB NES internal RAM is mirrored through `$0000-$1FFF`. PPU, APU,
controller, mapper, cartridge, and FDS devices register the ranges they own.

The Libretro port publishes:

- physical NES internal RAM and its CPU mirrors
- writable mapper and FDS pages
- battery-backed save RAM

These descriptors support RetroAchievements without changing emulated memory.

## PPU and HD capture

The PPU remains scanline/dot based. It evaluates primary OAM into secondary OAM,
fetches background and sprite patterns, applies palette, transparency, flipping,
and native priority, and produces the authentic NES result.

When an HD pack is loaded, `HdNesPpu` records additional source metadata:

- CHR tile index or full CHR-RAM pattern data
- palette identity
- tile offsets and mirroring
- background and sprite candidates
- native priority state
- frame number
- pack-requested RAM/PPU watch values

The native PPU decides authentic NES pixels. The HD renderer decides how larger
replacement artwork extending beyond those native pixels is composed.

## HD-pack loading

Classic Mesen/MesenCE rules are supported directly. Fork semantic tags are a
maintainability layer that lowers to classic conditions and tile mappings during
pack loading.

The current semantic family includes:

`<page>`, `<watch>`, `<paletteSet>`, `<rect>`, `<context>`,
`<contextTemplate>`, `<route>`, and `<replace>`.

Gameplay backgrounds add seamless wrapping and continuous scroll tracking beyond
the NES 512-pixel nametable cycle.

## Audio

The MesenCE NES APU emulates pulse, triangle, noise, DMC, and supported expansion
audio. Channel amplitude deltas are band-limited and mixed using NES nonlinear
mixing behavior. The shared mixer outputs to the Libretro audio adapter.

The adapter preserves short batch writes in a bounded stereo queue, trims stale
fast-forward backlog, and resubmits pending audio. RetroArch remains responsible
for host-device synchronization.

## Save states and renderer state

Emulated state is serialized through MesenCE. Renderer-only ownership and
lifetime tracking are derived metadata and are deliberately excluded from save
states and RetroAchievements-visible memory. The tracker resets on backward or
discontinuous frame progression and reconstructs itself from current PPU/OAM
data.

## Protected integration boundaries

Renderer work should not casually modify:

- `PublishRetroAchievementsMemoryMap`
- `retro_serialize_size`
- `retro_serialize`
- `retro_unserialize`
- `retro_get_memory_data`
- `retro_get_memory_size`
- NES RAM or mapper behavior
- CPU/PPU/APU timing
- frontend fast-forward semantics
- audio transport

Changes at those boundaries require their own focused regression pass.