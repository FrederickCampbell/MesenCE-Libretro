# HD-pack documentation

[Documentation index](../README.md)

## Pack location

Enable HD packs in the core options and place each pack at:

```text
RetroArch\system\HdPacks\<ROM filename without extension>\hires.txt
```

HD packs and copyrighted game assets are not included in this repository.

## Supported syntax

Classic Mesen/MesenCE tags include:

```text
<ver>, <scale>, <img>, <tile>, <condition>, <background>,
<overscan>, <patch>, <options>, <bgm>, <sfx>, <addition>
```

Fork-only semantic tags include:

```text
<page>, <watch>, <paletteSet>, <rect>, <context>,
<contextTemplate>, <route>, <replace>
```

Semantic rules are lowered into classic renderer rules during pack load.
Existing classic packs continue through the compatibility path.

## Background extensions

Background syntax supports seamless wrapping:

```text
None
RepeatX
RepeatY
RepeatXY
```

It also supports two scroll modes:

```text
Scanline
Gameplay
```

`Gameplay` follows accumulated game-world scrolling instead of a static
HUD split. Version 1.4.0 retains fractional scroll through HD sampling
and bulk-composites eligible lowest-priority repeating layers.

Set the following environment variable before launching RetroArch to
force the previous compatibility renderer for diagnostics:

```text
MESENCE_HD_BULK_PARALLAX=0
```

## Authoring references

- [Full syntax reference](syntax-reference.md)
- [Semantic smarter rules](semantic-rules.md)
- [Smoke-test pack](examples/hires.native-smarter-smoke-test.txt)

## Compatibility rules

- Keep classic `<tile>` fallbacks when adding semantic routes.
- Avoid per-frame image loading.
- Test level entry, death, restart, warp, and transitions.
- Test normal speed and fast-forward.
- Keep repeating artwork seamless at both horizontal edges.
- Do not commit game ROMs, BIOS files, or copyrighted pack assets.
