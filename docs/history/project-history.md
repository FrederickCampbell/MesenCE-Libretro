# Project history

[Documentation index](../README.md)

This document consolidates the development milestones that were previously split
across release pages, tags, handoff notes, and renderer experiments.

## Initial port

The project began as an NES/FDS-focused Libretro frontend based on the hunterk
Mesen2 prototype and updated to MesenCE 2.2.1. Early work established:

- Windows x64 builds
- RetroAchievements memory descriptors
- RetroArch system/save/firmware paths
- FDS `disksys.rom` lookup
- persistent decoded-frame delivery
- frontend-owned pacing and fast-forward

## Frontend smoothness and audio transport

The frontend moved from fixed decoded-frame polling delays to event-driven
synchronization. It added duplicate-frame capability handling, bounded
preservation of partially accepted audio batches, and fast-forward backlog
trimming.

## Semantic HD-pack rules

Large HD packs gained pages, watches, palette sets, reusable rectangles,
contexts, templates, routes, and replacements. These authoring constructs lower
to the classic Mesen runtime rule model.

## HD priority compositor

The lightweight compositor kept rendering local to the native candidates
available at each location and attached synthetic additions to source
provenance. It fixed multiple detached-piece and stale-cache problems without
the major cost of a discarded full-scene compositor.

## Gameplay parallax

Background wrapping and gameplay-based scrolling enabled seamless long-level
parallax. A bulk strip compositor provided smooth HD-pixel motion for eligible
layers with compatibility fallback.

## Stable native metasprites

Renderer debugging established that native PPU priority cannot fully define
ownership for enlarged artwork outside original opaque pixels. Stable
primary-OAM identity fixed scanline ownership. Geometry-based 2x2/2x3 grouping
made native metasprites atomic regardless of OAM permutation. Global temporal
matching and immutable lifetime priority tokens removed whole-object flashing
and midpoint priority reversals.

This work became the foundation of the 0.9.1 alpha line.

## Version reset

The earlier public 1.x tags represented rapid experimental milestones rather
than a finished stable API. The project reset to a pre-1.0 alpha line so future
changes communicate maturity honestly:

- `0.9.1-alpha.N` for compatible hardening patches
- `0.9.2-alpha.N` for the planned object-level HD-pack architecture
- `1.0.0` only after documented stability criteria are met