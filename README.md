# seedee's Half-Life Compilation Tools

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
<sub>Based on code modifications by Sean 'Zoner' Cavanaugh and Vluzacn</sub>  
<sup>Based on Valve's version, modified with permission.</sup>

Map compile tools for the Half-Life engine, based on Vluzacn's ZHLT v34. New features include additional tool textures, ability to extend map size limits, and portal file optimisation for the J.A.C.K. map editor.

## How to install

1. Open the configuration dialog of your map editor or batch compiler.
2. Set CSG, BSP, VIS, RAD tool pathes to 'sdHLCSG.exe', 'sdHLBSP.exe', 'sdHLVIS.exe', 'sdHLRAD.exe' in 'tools' folder.
3. Add 'sdhlt.wad' into your wad list.
4. Add 'sdhlt.fgd' into your fgd list.

If you are running 64-bit Windows, use 'sdHLCSG_x64.exe', 'sdHLBSP_x64.exe', 'sdHLVIS_x64.exe' and 'sdHLRAD_x64.exe'.  
The main benefit of the 64-bit version is no memory allocation failures, because the 64-bit tools have access to more than 2GB of system memory.

## Documentation
- [Tool textures](https://gamebanana.com/tuts/13211)