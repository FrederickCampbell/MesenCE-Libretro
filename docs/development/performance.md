# Performance and renderer testing

[Documentation index](../README.md)

## Current renderer

Version 1.4.0 adds a bulk strip compositor for eligible lowest-priority
`Gameplay` plus `RepeatX` background layers. Other backgrounds retain
the previous block renderer automatically.

The renderer remains CPU-based. Cost depends on:

- HD scale
- output dimensions
- number of active layers
- image width and wrap behavior
- transparent versus opaque coverage
- blend mode
- foreground and sprite overlap

No formal minimum CPU has been established. The confirmed reference
system is an Intel Core i5-13600K.

## Required test matrix

Test each renderer change with:

1. a non-HD NES game
2. an FDS game
3. a classic HD pack
4. a semantic HD pack
5. normal speed
6. uncapped speed
7. RetroArch fast-forward
8. level entry, death, restart, warp, and transition
9. repeating-layer seams
10. the fallback path

Force fallback before launching the frontend:

```powershell
$env:MESENCE_HD_BULK_PARALLAX = "0"
```

Enable bulk rendering explicitly:

```powershell
$env:MESENCE_HD_BULK_PARALLAX = "1"
```

## Build optimization

The supported Windows release build uses GCC `-O3` with `LTO=false`.
Experimental LTO currently exposes project-wide upstream ODR/type
mismatches and is not part of the release workflow.

Performance regressions in HD hot paths should block release even when
rendering output is visually correct.
