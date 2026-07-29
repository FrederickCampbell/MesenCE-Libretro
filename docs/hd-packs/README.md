# HD packs

[Documentation index](../README.md)

## Location

Place packs under:

```text
RetroArch\system\HdPacks\<content filename without extension>\
```

The pack entry file is `hires.txt`.

## Compatibility

The fork supports classic Mesen/MesenCE HD-pack syntax. It also adds semantic
authoring tags for large packs:

- `<page>`
- `<watch>`
- `<paletteSet>`
- `<rect>`
- `<context>`
- `<contextTemplate>`
- `<route>`
- `<replace>`

Semantic declarations are resolved during pack loading into the runtime
condition/tile structures used by the renderer.

## CHR identity

CHR-ROM content can be identified through mapped tile indexes. CHR-RAM content
such as FDS games uses the complete 16-byte pattern data plus palette identity.
This allows visually identical pattern layouts with different palettes to route
to different replacement art.

Rule candidates are evaluated in stored order; the first candidate whose
conditions pass is selected.

## Additions

`<addition>` creates synthetic replacement pieces attached to a source tile.
Synthetic pieces inherit the source logical object's root and lifetime priority.
They are useful for artwork extending beyond native hardware layouts, though
ordinary full-sprite authoring may eventually move to an explicit object layer.

## Backgrounds and parallax

Fork background extensions support:

- `RepeatX`
- `RepeatY`
- `RepeatXY`
- `Gameplay` scroll mode

Gameplay scrolling samples the active playfield rather than a fixed HUD split
and unwraps the NES horizontal scroll cycle into continuous movement. Eligible
lowest-priority repeating layers use the bulk strip compositor; unsupported
cases automatically use the compatibility path.

Set `MESENCE_HD_BULK_PARALLAX=0` for diagnostic A/B testing.

See the [syntax reference](syntax-reference.md) and
[semantic rules guide](semantic-rules.md).