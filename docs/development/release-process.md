# Release process

[Documentation index](../README.md)

## Versioning

The alpha line follows semantic prerelease identifiers:

- `0.9.1-alpha.1`
- `0.9.1-alpha.2`
- `0.9.2-alpha.1`

The Git tag adds a leading `v`.

Keep these synchronized:

- root `VERSION`
- `retro_get_system_info()` library version
- `CHANGELOG.md`
- `docs/releases/<tag without v>.md`

## Preflight

```powershell
git status --short
git fetch origin --tags --prune
git diff --check
gh auth status
```

Require a clean tree before applying source work. Do not publish from a
disposable renderer test worktree.

## Build

```powershell
.\BUILD_WINDOWS_X64.ps1 `
  -Clean `
  -MsysRoot 'C:\_Hub\_Dev\Tools\MSYS2' `
  -SkipPrerequisiteInstall
```

Expected artifacts:

```text
dist\mesen_ce_nes_libretro.dll
dist\mesen_ce_nes_libretro.info
dist\SHA256SUMS.txt
dist\DLL_IMPORTS.txt
```

## Verify

- mandatory Libretro exports are present
- forbidden MinGW runtime dependencies are absent
- DLL checksum is generated
- non-HD NES works
- FDS firmware/content works
- classic HD packs work
- semantic HD packs work
- pause, fast-forward, rewind, and state load are tested
- renderer overlap matrix passes
- published checksum matches the local DLL

## Publish

1. Commit the exact source and documentation used for the build.
2. Fast-forward the public `main` branch without rewriting history.
3. Create and push an annotated tag.
4. Create a GitHub prerelease with `--verify-tag`.
5. Upload DLL, `.info`, checksum, and import audit.
6. Verify the release assets and checksum.

The one-time 0.9.1 reset also archives old release metadata/assets and removes
the previous experimental releases/tags after the new alpha release exists.
Future releases must not routinely delete history.