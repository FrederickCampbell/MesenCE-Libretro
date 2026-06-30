# Port notes

This repository contains a patched MesenCE source tree rather than a script
that recreates the port during every build.

Important integration decisions:

1. The public Libretro target is NES/FDS-focused.
2. RetroArch is the sole frame-pacing authority.
3. MesenCE's decoded `RenderedFrame` is submitted directly to the frontend.
4. MesenCE's home and firmware folders are mapped to RetroArch's system
   directory.
5. FDS firmware is expected directly under that system directory.
6. NES internal RAM, writable mapper/FDS pages, and battery-backed memory are
   exposed through Libretro memory descriptors.
7. The core has a separate library and display identity from the stock Mesen
   core.

The original MesenCE README from the imported source revision is preserved as
`docs/UPSTREAM_MESENCE_README.md`.