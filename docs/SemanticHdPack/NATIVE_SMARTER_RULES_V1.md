# Native smarter HD rules v1

This patch makes the fork read smarter rules directly inside the active `hires.txt`.

ELI5: before MesenCE parses `hires.txt`, this turns the new clean tags into normal old Mesen tags in memory.

## Supported tags

```txt
<page>mario.normal = characters/Mario/mario.png
<watch>swimming = mem(0704)>00
<paletteSet>playerPalettes = ff063727,ff0c3c1c
<rect>player.super.swimFoot = 1792,320,128,64
<context>swimNoFire = swimming&!firethrow
<route>playerPage = when isLuigi -> luigi.normal; default -> mario.normal
<replace>tile=31313131313131313131313131313131,paletteSet=playerPalettes,page=@playerPage,target=player.super.swimFoot,context=swimNoFire
```

## What this is

Safe native parser support for cleaner `hires.txt` authoring.

## What this is not yet

This is not the final runtime route engine. It still lowers smarter rules into classic tile/condition rules in memory.

Next later: true runtime `@playerPage` routing without expanding duplicate tile rules.