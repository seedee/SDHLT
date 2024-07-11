# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.2.0] - Jul 11 2024
### Changed
- Add studiomodel shadows with 3 shadow modes and `-nostudioshadow`
- Add *info_portal* and *info_leaf*
- Add *info_minlights* and `%` texture flag
- Add `-pre25`, increase `-limiter` default to `255`
- Increase `-bounce` to min `12` if using `-expert`
- Enable `-wadautodetect` by default
- Reformatted texture-related logging to look like resgen
- Add CMake config and Makefile

### Fixed
- Potential buffer overrun in `PushWadPath`

## [1.1.2] - Sep 09 2022
### Changed
- Reasons for skipping portal file optimisation process are more detailed

### Fixed
- Fatal errors replaced with generic log messages when skipping optimisation of portal file

## [1.1.1] - Aug 27 2022
### Changed
- Portal file optimisation process more streamlined. Separate .prt file is no longer created, instead the same one is optimised after VIS compilation
- Automatic embedding of tool texture WAD file is hard-coded again due to lazy mappers
- -chart parameter now enabled by default

### Fixed
- Bug with -worldextent CSG parameter, where the default map size was +/-2048

## [1.1.0] - Jul 04 2020
### Added
- -worldextent CSG parameter. Extends map geometry limits beyond +/-32768
- Optimised portal file workflow for J.A.C.K, allowing import of .prt file into the editor directly after BSP compilation
- Higher resolution image textures to tool texture WAD file

## 1.0.0 - Mar 09 2020
### Added
- BEVELHINT tool texture, which acts like SOLIDHINT and BEVEL. Eliminates unnecessary face subdivision and bevels clipnodes at the same time
- SPLITFACE tool texture. Brushes with this texture will subdivide faces they touch along their edges
- !cur_ tool textures, which act like CONTENTWATER and func_pushable with a speed of 2048 units/s
### Changed
- Automatic embedding of tool texture WAD file can now be controlled in settings.txt

[1.1.2]: https://github.com/seedee/SDHLT/compare/v1.1.1...v1.1.2
[1.1.1]: https://github.com/seedee/SDHLT/compare/v1.1.0...v1.1.1
[1.1.0]: https://github.com/seedee/SDHLT/releases/tag/v1.1.0
