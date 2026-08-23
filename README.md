# SDHLT – Fork with Optimization Focus

This is a **fork** of the original [SDHLT](https://github.com/seedee/SDHLT) by the SDLHLT team, which itself is based on Vluzacn's ZHLT v34 and Valve's original Half‑Life map compile tools.

The original project is licensed under the **GNU General Public License** (see `LICENSE` file in this repository).  
This fork retains that license and adds **performance optimizations** and minor internal refactoring, while keeping all original functionality intact.

---

## Original Work Attribution

- **Original authors**: SDLHLT team, Vluzacn, and various contributors.
- **Based on**: Valve's Half‑Life engine tools, used with permission.
- **Original repository**: [SDHLT](https://github.com/seedee/SDHLT)

All features, entities, tool textures, and compile parameters listed below are inherited from the original SDHLT, except where noted as *new in this fork*.

---

## How to Install *(unchanged from original)*

1. Open the configuration dialog of your map editor or batch compiler.
2. Set CSG, BSP, VIS, RAD tool paths to `sdHLCSG.exe`, `sdHLBSP.exe`, `sdHLVIS.exe`, `sdHLRAD.exe` (use `_x64.exe` on 64‑bit systems).
3. Add `sdhlt.wad` to your WAD list (required).
4. Add `sdhlt.fgd` to your FGD list.

> The 64‑bit versions avoid memory allocation failures by accessing more than 2GB of RAM.

---

## Features *(all from original SDHLT)*

### Studiomodel Shadows
Entities with a `model` keyvalue (`env_sprite`, `cycler_sprite`, etc.) support:
- `zhlt_studioshadow 1` – flag model as opaque to lighting.
- `zhlt_shadowmode n` – shadow tracing mode:
  - `1` (default) – traces each triangle with transparent‑texture support.
  - `2` – no transparency, adds thickness to fill seams.
  - `0` – traces a bbox around each triangle (approximates whole‑model bbox).

For SmartEdit FGD integration, use the template at the top of `sdhlt.fgd`.  
If shadows darken the origin, set a custom `light_origin` or move the mesh origin.

### Entities
- `info_portal` + `info_leaf` – force visibility between two leaves.
- `info_minlights` – set per‑texture minimum light levels (works on world geometry).

### Textures
- `%` flag – sets minlight (e.g., `%texname` = `_minlight 1.0`; `%#texname` with `#` = 0–255).
- `BEVELHINT` – combines SOLIDHINT and BEVEL, reduces subdivision and clips nodes.
- `SPLITFACE` – subdivides faces along brush edges (like `zhlt_chopdown`).
- `cur_tool` – acts as CONTENTWATER + func_pushable (2048 units/s in -Y), always fullbright.

### Compile Parameters *(original)*
- `-pre25` – sets light clipping threshold to 188 (for pre‑25th‑anniversary engines).
- `-extra` – now implies `-bounce 12` for higher‑quality lighting.
- `-worldextent n` – extends map limits beyond ±32768.
- Portal file reformatting for J.A.C.K. (use `-nofixprt` to disable).
- `-nowadautodetect` – wadautodetect is now on by default.
- `-nostudioshadow` – ignore `zhlt_studioshadow`.

---

## What’s New in This Fork (Optimization Focus)

- **Performance improvements** in:
  - Distance based light cull

All optimizations are additive – they do not alter input/output compatibility with original SDHLT.

---

## License & Compliance

This fork is released under the **same license** as the original SDHLT (GPL).  
All original copyright notices, license headers, and attribution requirements remain intact in the source code.

Modifications are clearly marked in commit history and code comments.  
For full license terms, see the `LICENSE` file in this repository.

---

## Credits

- **Original SDHLT** – SDLHLT team, Vluzacn, and all contributors.
- **Valve** – for the original Half‑Life tools.
- **This fork** – [Bruhgogogo] (optimization work).

---

*For original documentation and feature details, please refer to the original SDHLT repository.*