# Audio, video, pacing, and hitch behavior

[Documentation index](../README.md)

## Audio generation

MesenCE emulates the NES pulse, triangle, noise, DMC, and expansion channels at
CPU-cycle-accurate event boundaries. Channel amplitude changes feed band-limited
resamplers and the NES channels are combined with nonlinear mixing behavior.

The Libretro target outputs 48 kHz audio to RetroArch.

## Libretro audio adapter

The adapter converts mono output to interleaved stereo when required and keeps a
bounded pending queue. If the frontend accepts only part of a batch, the
remaining samples are preserved instead of dropped. Excess fast-forward backlog
is trimmed before normal playback resumes.

Current counters track short writes, zero writes, and dropped queued frames.

## Video delivery

MesenCE builds a native or HD `RenderedFrame` and the asynchronous video decoder
produces the frontend image. The Libretro layer retains the latest completed
decoded frame and supports duplicate-frame submission when the frontend permits
it.

## Current synchronization policy

RetroArch owns pacing, throttling, fast-forward, slow motion, and host audio
synchronization. The core should generate one complete emulated frame per
`retro_run()` and must not run an independent desktop frame limiter.

## Known hitch path

A rare expensive HD frame can delay normal-speed return from `retro_run()`.
That can make a visual frame drop and an audio hitch occur together even when
the APU itself is correct.

Planned alpha work:

1. Add measured decoder/renderer/audio queue diagnostics.
2. Test a smooth delivery mode that duplicates a late video frame instead of
   blocking the next emulation/audio frame.
3. Replace abrupt queue trimming with a short splice from the last submitted
   sample to the new queue head.
4. Compare the current 96 kHz internal path and final downsampling against a
   direct 48 kHz band-limited path before changing audio quality.

Audio changes must not introduce pitch drift, duplicate synchronization loops,
or save-state differences.