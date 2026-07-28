# Release process

[Documentation index](../README.md)

## Preflight

```powershell
git status --short
git fetch origin --tags --prune
git diff --check
```

Require a clean working tree and reconcile remote changes before
building.

## Build

```powershell
.\BUILD_WINDOWS_X64.ps1 -Clean
```

The supported Windows x64 build uses `LTO=false`.

Expected public artifacts:

```text
dist\mesen_ce_nes_libretro.dll
dist\mesen_ce_nes_libretro.info
dist\mesen_ce_nes_libretro.sha256.txt
```

## Verify

- Confirm mandatory Libretro exports.
- Confirm the DLL has no unexpected runtime dependency.
- Hash the DLL.
- Install only after closing RetroArch.
- Verify the installed DLL hash matches the release DLL.
- Test non-HD, FDS, classic-HD, and semantic-HD content.

## Publish

1. Update `CHANGELOG.md`.
2. Add detailed notes under `docs/releases/`.
3. Commit the exact source used for the build.
4. Create an annotated version tag.
5. Push the branch and tag without rewriting history.
6. Create the GitHub release and attach the DLL, `.info`, and checksum.
7. Verify the published asset hash.

Documentation-only repository cleanup does not require a new binary
release.
