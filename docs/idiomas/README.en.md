<div align="center">

# Menu ZB2

A development menu for Zumbi Blocks 2 with a D3D11 interface and Unity Mono integration.

[![C++](https://img.shields.io/badge/C++-17-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)](../../kiero-dx11-base.vcxproj)
[![Status](https://img.shields.io/badge/status-in_development-orange?style=flat-square)](../../CHANGELOG.md)
[![Licensing](https://img.shields.io/badge/licensing-see_NOTICE-yellow?style=flat-square)](../../NOTICE.md)

<a href="../../README.md"><img src="https://api.iconify.design/flag/br-4x3.svg?width=26" alt="Português" title="Português" /></a>
&nbsp;
<a href="README.en.md"><img src="https://api.iconify.design/flag/us-4x3.svg?width=26" alt="English" title="English" /></a>

</div>

The native C++ module provides the interface and rendering. Managed C# adapters run inside the game's Mono environment. Features include aiming controls, weapon and movement parameters, entity/item/world overlays, presets and diagnostic logs.

> [!IMPORTANT]
> This private repository contains development sources, tests and documentation. A single-file launcher and automatic updates are planned, not implemented. There is no stable public release. Automated tests do not establish live compatibility in solo, host and client modes.

## Build

Use Windows x64, Visual Studio C++ toolset v145, Windows SDK and Python 3. The managed build requires the exact game assembly hash recorded in `scripts/build_managed_aim.py`. Game binaries are not included.

```powershell
msbuild kiero-dx11-base.vcxproj /p:Configuration=Release /p:Platform=x64
msbuild injector/injector.vcxproj /p:Configuration=Release /p:Platform=x64
python scripts/build_managed_aim.py --managed "GAME_PATH/ZumbiBlocks2_Data/Managed"
```

Build output is in `build/Release_x64`. Keep the injector, configuration, native DLL, managed adapter, Harmony dependency and its license together. Preserve matching PDB/MAP files for crash diagnosis. Enter a map before running the injector; Insert toggles the menu. Current logs and presets live in `Documents/kiero-dx11-base`. The planned launcher directory is `Documents/ZB2Menu`.

## Tests

```powershell
python tests/check_runtime_ownership.py
python tests/run_aim_validation.py
python tests/run_managed_aim_validation.py
python tests/run_modifier_bridge_validation.py
python tests/run_range_validation.py
```

Managed tests require Harmony from the preceding build. UI tests use simulated data without controlling the game screen.

## Known limitations

- Multiplayer effects remain subject to server behavior and per-feature live validation.
- Unimplemented controls, including teleport and chams, are marked pending.
- Distant unloaded entities expose known positions, not fabricated health or bones.
- Legacy inventory, currency and unlock operations are not equivalent to reversible parameter modifiers.
- Launcher, update delivery and access management are not implemented.
- Licensing provenance must be consolidated before public redistribution.

See the maintained Portuguese references for [structure](../ESTRUTURA.md), [distribution planning](../DISTRIBUICAO.md), [modifier audit](../AUDITORIA_MODIFICADORES_ESP.md), [render stability](../ESTABILIDADE_RENDER.md), [changelog](../../CHANGELOG.md), [contributing](../../.github/CONTRIBUTING.md) and [licensing](../../NOTICE.md).
