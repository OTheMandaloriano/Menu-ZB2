<div align="center">
  <picture><source media="(prefers-color-scheme: dark)" srcset="../../apps/shared/brand/deadblock-menu.svg" /><img src="../../apps/shared/brand/deadblock-menu-light.svg" alt="DEADBLOCK" width="340" /></picture>
  <p><a href="../../README.md"><img src="https://api.iconify.design/flag/br-4x3.svg?width=26" alt="Português" title="Português" /></a>
  &nbsp;
  <a href="README.en.md"><img src="https://api.iconify.design/flag/us-4x3.svg?width=26" alt="English" title="English" /></a></p>
</div>

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

## Preview and downloads

![Admin preview](../previews/admin-clientes.png)

Previews use the production UI with synthetic data, not live gameplay captures.
[Gallery](../PREVIAS.md) · [Releases](https://github.com/OTheMandaloriano/Menu-ZB2/releases)

Releases are private and this first package is a prerelease. Initial client/team
ZIPs do not include personal licenses or owner keys. GitHub Packages is not used.

## Installation and recovery

End users need Windows x64 and a DirectX 11-compatible graphics driver. Players
also need a compatible game and a license. Admin operators need .NET Framework
(4.8 or a later supported 4.x version is recommended) and authorization; they do
not need the game to issue licenses. Build tools are for developers only.

> [!IMPORTANT]
> Downloading the Admin on another PC does not restore owner access. Keys are
> protected by Windows DPAPI CurrentUser. Portable password-protected backup is
> not implemented. Preserve the original Windows profile and plan a tested transfer
> before wiping the original PC. See the [recovery guide](../RECUPERACAO.md).
