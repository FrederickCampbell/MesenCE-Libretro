# Contributing to MesenCE Libretro

Thanks for helping improve the project.

## Scope

The public Libretro target is intentionally focused on NES, Famicom,
and FDS behavior. Discuss large architectural changes before investing
significant work.

## Before opening a pull request

1. Work from the current default branch.
2. Keep emulator changes isolated and easy to review.
3. Run `git diff --check`.
4. Build with `BUILD_WINDOWS_X64.ps1 -Clean`.
5. Test non-HD content plus relevant classic and semantic HD packs.
6. Include reproduction steps, performance observations, and test ROM
   names without attaching copyrighted ROM data.

## Performance

Renderer and frame-pacing hot paths are performance-sensitive. Measure
uncapped FPS before and after a change. A visual fix that causes a
material performance regression is not release-ready.

## Style

Follow the surrounding source style and repository `.editorconfig`.
Use clang-format for C/C++ changes and preserve configured line endings.

## Licensing and source provenance

Contributions must be compatible with the repository license and must
not include ROMs, BIOS files, copyrighted HD-pack artwork, leaked source,
or code copied from an incompatible project.

You remain responsible for understanding and validating every submitted
line. Disclose substantial AI assistance in the pull-request description
and describe the tests used to verify the result.

Changes intended for upstream MesenCE must also satisfy upstream's
contribution policy, preserved under
[`docs/upstream/CONTRIBUTING.md`](../docs/upstream/CONTRIBUTING.md).
