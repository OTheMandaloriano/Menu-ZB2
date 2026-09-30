# DEADBLOCK

Native menu, Windows loader and license administration panel for Zumbi Blocks 2.
This private repository contains source code, tests, assets and documentation.

## Components

- `src/menu/` and `managed/`: native D3D11 menu and Unity Mono integration.
- `apps/loader/`: offline activation, package verification and loading.
- `apps/admin/`: customer licenses, team authorization and ZIP generation.
- `apps/shared/`: shared interface components and branding.

The loader is functional. GitHub auto-updates and automatic cleanup of old client
runtimes are not implemented. Code-signing and antivirus findings remain documented
limitations. See the [current Portuguese README](../../README.md) and
[handoff guide](../CONTINUIDADE.md) for the maintained release state.

## Development

Use Windows x64, MSVC v145 and Windows SDK. Packaging also needs Python 3.12+,
cryptography, CMake and Ninja. Game assemblies and private signing material are not
included in the repository.

```powershell
msbuild Deadblock.Menu.vcxproj /p:Configuration=Release /p:Platform=x64
python tests/check_repository_layout.py
python tests/check_runtime_ownership.py
python tests/run_aim_validation.py
```

Build order: menu, loader, Admin, delivery ZIPs. See [development](../DESENVOLVIMENTO.md),
[architecture](../ARQUITETURA.md), [documentation index](../README.md) and
[licensing](../../NOTICE.md). Never distribute the repository, private profiles or build tree.
