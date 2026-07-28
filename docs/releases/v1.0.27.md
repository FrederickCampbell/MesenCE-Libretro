# MesenCE Libretro v1.0.27

Experimental Windows x64 release.

## Highlights

- Adds fork-only semantic HD-pack support:
  - `<page>`
  - `<watch>`
  - `<paletteSet>`
  - `<rect>`
  - `<context>`
  - `<contextTemplate>`
  - `<route>`
  - `<replace>`
- Keeps classic Mesen/MesenCE HD-pack behavior intact.
- Verified with the SMB2J semantic v2b HD pack:
  - 2,749,621 bytes
  - 33,681 lines
  - active semantic route/contextTemplate/paletteSet syntax
- README now documents the semantic HD-pack layer and links the full syntax guide.

## Asset

This release uploads only:

```text
mesen_ce_nes_libretro.dll
```

Keep the stock Mesen core installed alongside this experimental core while testing.
