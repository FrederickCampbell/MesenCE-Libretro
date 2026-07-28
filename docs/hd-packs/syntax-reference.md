# HD Pack Syntax Guide

Current fork-approved guide for the MesenCE Libretro branch
`feature/native-smarter-hd-rules-v1`.

This guide documents syntax that should be treated as valid for this fork right
now. It intentionally does not list future dream syntax as supported.

## Compatibility

Classic Mesen/MesenCE syntax remains supported.

Fork-only semantic syntax requires this fork:

```text
<page>
<watch>
<paletteSet>
<rect>
<context>
<contextTemplate>
<route>
<replace>
```

The semantic tags are parsed before normal HD-pack loading and are lowered into
classic in-memory rules. Existing classic packs should continue working.

Rule order still matters: first matching HD rule wins.

## Core classic tags

### `<ver>`

```text
<ver>109
```

Declares the HD-pack format version.

### `<scale>`

```text
<scale>8
```

Declares the scale factor for replacement graphics.

### `<img>`

```text
<img>characters/Mario/mario.png
```

Adds a PNG to the image list. Classic `<tile>` rules use numeric image indexes
based on this order.

### `<tile>`

```text
<tile>imageIndex,tileData,paletteData,x,y,brightness,defaultTile
```

Example:

```text
<tile>9,000001070F0F0E120000000000000F1F,ff063727,1024,128,1,N
```

Fields:

```text
imageIndex      PNG image index from <img>
tileData        original tile index or CHR RAM tile hash
paletteData     original palette bytes
x               replacement x coordinate
y               replacement y coordinate
brightness      usually 1
defaultTile     Y or N
```

CHR RAM tile data is normally a 32-character hex string.

### `<condition>`

```text
<condition>name,type,args...
```

Use conditions by prefixing a rule:

```text
[name]<tile>...
[nameA&nameB&!nameC]<tile>...
```

Built-in condition names:

```text
hmirror
vmirror
bgpriority
```

### Nearby and position conditions

```text
<condition>name,tileNearby,xOffset,yOffset,tileData,paletteData
<condition>name,spriteNearby,xOffset,yOffset,tileData,paletteData
<condition>name,tileAtPosition,x,y,tileData,paletteData
<condition>name,spriteAtPosition,x,y,tileData,paletteData
```

### Memory conditions

```text
<condition>name,memoryCheckConstant,address,operator,value
<condition>name,memoryCheckConstant,address,operator,value,mask
<condition>name,ppuMemoryCheckConstant,address,operator,value
<condition>name,ppuMemoryCheckConstant,address,operator,value,mask
```

Operators:

```text
== != > < >= <=
```

Example:

```text
<condition>isLuigi,memoryCheckConstant,0753,==,01
<condition>starPhase0,memoryCheckConstant,03C4,==,00,03
```

### `frameRange`

```text
<condition>name,frameRange,divisor,compare
```

Expression:

```text
currentFrame % divisor >= compare
```

### `<background>`

```text
[condition]<background>filename,brightness,hScrollRatio,vScrollRatio,priority
```

Longer background forms may include offsets and MesenCE-family blend options.
Verify exact extended grammar in `Core/NES/HdPacks/HdPackLoader.cpp` before
authoring new extended background rules.

### `<addition>`

Used for injecting extra HD tiles around a matched tile/sprite.

Practical form used by current SMB2J work:

```text
<addition>triggerTile,triggerPalette,xOffset,yOffset,addedTile,addedPalette
```

The added tile normally needs a matching `<tile>` rule.

### Other classic tags

```text
<overscan>top,right,bottom,left
<patch>filename,sha1
<options>option1,option2
<bgm>album,track,filename.ogg
<sfx>album,track,filename.ogg
<supportedRom>sha1
```

## Fork semantic tags

### `<page>`

```text
<page>name = imagePath
```

Example:

```text
<page>mario.normal = characters/Mario/mario.png
<page>luigi.fire.star2 = characters/Luigi/fireluigi_2.png
```

Defines a named image page.

### `<watch>`

Readable memory-check shortcut.

```text
<watch>name = mem(address)==value
<watch>name = mem(address)>value
<watch>name = mem(address)==value mask maskValue
```

Example:

```text
<watch>isLuigi = mem(0753)==01
<watch>isFire = mem(0756)==02
<watch>isStar = mem(079F)>00
<watch>starPhase0 = mem(03C4)==00 mask 03
```

Lowered into `memoryCheckConstant` conditions.

### `<paletteSet>`

```text
<paletteSet>name = palette1,palette2,palette3
```

Example:

```text
<paletteSet>playerPalettes = ff063727,ff0c3c1c,ff0f3617
```

Used by `<replace>` to generate one rule per palette.

### `<rect>`

```text
<rect>name = x,y
<rect>name = x,y,w,h
```

Example:

```text
<rect>super.swim1 = 1408,128,128,256
```

Current `<replace>` uses only x/y. Width/height are allowed for readability.

### `<context>`

```text
<context>name = condA&condB&!condC
```

Example:

```text
<context>rightFacing = !hmirror
```

### `<contextTemplate>`

Reusable context that changes per palette.

Pattern mode:

```text
<contextTemplate>name = conditionUsing{PAL}
```

Branch-map mode:

```text
<contextTemplate>name = paletteA:conditionsA; paletteB:conditionsB; default:conditionsDefault
```

Example:

```text
<contextTemplate>ct00001 = ff063727:zgfm&hmirror; ff0c3c1c:zgfn&hmirror
```

Used by:

```text
<replace>...,paletteSet=psCT0001,contextTemplate=ct00001
```

### `<route>`

Defines dynamic page routing.

```text
<route>routeName = when condExpr -> pageName; when condExpr2 -> pageName2; default -> pageName
```

Example:

```text
<route>playerPage = when isLuigi&isFire -> luigi.fire; when isLuigi -> luigi.normal; default -> mario.normal
```

Use with:

```text
page=@playerPage
```

Current limitation: this is load-time lowering, not true runtime routing.

### `<replace>`

Semantic replacement rule.

```text
<replace>tile=TILE,palette=PAL,page=PAGE,target=RECT
<replace>tile=TILE,paletteSet=SET,page=@ROUTE,target=RECT,context=CTX,contextTemplate=TEMPLATE
```

Common fields:

```text
tile              required
palette           one palette value
paletteSet        named palette set
page              named <page> or @route
target            named <rect>
context           optional <context>
contextTemplate   optional <contextTemplate>
brightness        optional, default 1
default           optional, default N
conditions        optional raw condition expression
```

Example:

```text
<replace>tile=000001070F0F0E120000000000000F1F,paletteSet=psCT0001,page=@playerPage,target=r1024_128,contextTemplate=ct00091
```

## Verified SMB2J semantic v2b profile

```text
SHA-256:           9192adb59bb1c0255153cf6a915fab5398a7d3f2f83944d90714ceddddb9c1b0
Bytes:             2,749,621
Lines:             33,681
<route>:           1
<replace>:         2,821
<context>:         1,245
<contextTemplate>: 380
<paletteSet>:      2
classic <tile>:    9,825
page=@playerPage:  2,820
```

## Not currently supported syntax

These are future ideas only:

```text
<include>
<define>
<let>
<atlas>
<tileDef>
<hash>
<conditionPack>
<contextPack>
<tilePack>
<poseContext>
<matrixContext>
<nearbyGrid>
```
