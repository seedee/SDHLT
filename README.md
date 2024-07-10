```
  █████████  ██████████   █████   █████ █████    ███████████
 ███░░░░░███░░███░░░░███ ░░███   ░░███ ░░███    ░█░░░███░░░█
░███    ░░░  ░███   ░░███ ░███    ░███  ░███    ░   ░███  ░ 
░░█████████  ░███    ░███ ░███████████  ░███        ░███    
 ░░░░░░░░███ ░███    ░███ ░███░░░░░███  ░███        ░███    
 ███    ░███ ░███    ███  ░███    ░███  ░███      █ ░███    
░░█████████  ██████████   █████   █████ ███████████ █████   
 ░░░░░░░░░  ░░░░░░░░░░   ░░░░░   ░░░░░ ░░░░░░░░░░░ ░░░░░    
```
<sub>Half-Life engine map compile tools, based on Vluzacn's ZHLT v34 with code contributions from various contributors. Based on Valve's version, modified with permission.</sup>

New features include new entities, additional tool textures, ability to extend world size limits, portal file optimisation for J.A.C.K. map editor and minor algorithm optimization.

## How to install

1. Open the configuration dialog of your map editor or batch compiler.
2. Set CSG, BSP, VIS, RAD tool paths to *sdHLCSG.exe*, *sdHLBSP.exe*, *sdHLVIS.exe*, *sdHLRAD.exe*, use the *_x64.exe* editions if running on 64-bit.  
3. Add 'sdhlt.wad' into your wad list. This is required to compile maps.
4. Add 'sdhlt.fgd' into your fgd list.

The main benefit of the 64-bit version is no memory allocation failures, because the 64-bit tools have access to more than 2GB of system memory.

## Features

- *info_portal* and *info_leaf* used to create a portal from the leaf the *info_portal* is inside, to selected leaf the *info_leaf* is inside. Forces target leaf to be visible from the current one, making all entities visible.
- **BEVELHINT** tool texture, which acts like **SOLIDHINT** and **BEVEL**. Eliminates unnecessary face subdivision and bevels clipnodes at the same time. Useful on complex shapes such as terrain, spiral staircase clipping, etc.
- **SPLITFACE** tool texture. Brushes with this texture will subdivide faces they touch along their edges, similarly to `zhlt_chopdown`.
- `-pre25` RAD paremeter overrides light clipping threshhold limiter to `188`. Use this when creating maps for the legacy pre-25th anniversary engine without worrying about other parameters.
- `-extra` RAD parameter now sets `-bounce 12` for a higher quality of lighting simulation.
- `-worldextent n` CSG parameter. Extends map geometry limits beyond `+/-32768`.
- Portal file optimization for J.A.C.K. map editor, allows for importing the prt file into the editor directly after VIS. Use `-nofixprt` VIS parameter to disable.
- **cur_tool** textures, which act like **CONTENTWATER** and *func_pushable* with a speed of `2048 units/s` in -Y. This texture is always fullbright.
- `-nowadautodetect` CSG parameter. Wadautodetect is now true by default regardless of settings.

## Planned
- **BLOCKLIGHT** tool texture, cast shadows without generating faces or cliphulls.
- Optimization for BuildFacelights and LeafThread
- Shadow casting for studiomodels, potentially adapting code from other codebases.
- Res file creation for servers

## Documentation
- [Tool textures](https://gamebanana.com/tuts/13211)