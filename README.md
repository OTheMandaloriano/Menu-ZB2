<div align="center">

# Menu ZB2

Menu de desenvolvimento para Zumbi Blocks 2, com interface D3D11, visualização de entidades e integração com Unity Mono.

[![C++](https://img.shields.io/badge/C++-17-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)](kiero-dx11-base.vcxproj)
[![Windows](https://img.shields.io/badge/Windows-x64-0078D4?style=for-the-badge)](docs/ESTRUTURA.md)
[![Estado](https://img.shields.io/badge/estado-em_desenvolvimento-orange?style=flat-square)](CHANGELOG.md)
[![Licenciamento](https://img.shields.io/badge/licenciamento-ver_NOTICE-yellow?style=flat-square)](NOTICE.md)

<a href="README.md"><img src="https://api.iconify.design/flag/br-4x3.svg?width=26" alt="Português" title="Português" /></a>
&nbsp;
<a href="docs/idiomas/README.en.md"><img src="https://api.iconify.design/flag/us-4x3.svg?width=26" alt="English" title="English" /></a>

</div>

<h2><img src="https://api.iconify.design/solar/info-circle-bold.svg?color=%233B82F6&width=24" alt="" /> Sobre o projeto</h2>

O projeto combina uma DLL nativa C++ para interface e renderização com adaptadores C# executados no ambiente Mono do jogo. Inclui controles de mira, modificadores de movimento e armas, ESP de zumbis, itens e pontos do mundo, presets e logs de diagnóstico.

Este é o repositório privado de **código-fonte, testes e documentação**. Distribuição para usuários e atualização do menu pertencem a um canal separado. Veja [distribuição e atualizações](docs/DISTRIBUICAO.md).

> [!IMPORTANT]
> O launcher de arquivo único e a atualização automática ainda não estão implementados. Não há uma versão pública estável. Testes simulados não substituem validação em partida solo, host e cliente.

## Requisitos de desenvolvimento

| Componente | Requisito |
|---|---|
| Sistema | Windows x64 |
| Compilador nativo | Visual Studio, ferramentas C++ v145 e Windows SDK |
| Scripts | Python 3 |
| Jogo | Build Mono compatível com o hash registrado em `scripts/build_managed_aim.py` |
| Dependência gerenciada | Harmony 2.3.3, obtida pelo script com hash verificado |

As DLLs proprietárias do jogo são referenciadas na instalação local e não são distribuídas neste repositório.

## Compilar

No terminal de desenvolvimento do Visual Studio:

```powershell
msbuild kiero-dx11-base.vcxproj /p:Configuration=Release /p:Platform=x64
msbuild injector/injector.vcxproj /p:Configuration=Release /p:Platform=x64
python scripts/build_managed_aim.py --managed "CAMINHO_DO_JOGO/ZumbiBlocks2_Data/Managed"
```

A saída fica em `build/Release_x64`. A distribuição de desenvolvimento contém o injetor, sua configuração, a DLL nativa, o adaptador gerenciado, Harmony e sua licença. Os símbolos `.pdb` e `.map` devem ser preservados por versão para investigar crashes.

## Executar e diagnosticar

1. Compile os componentes compatíveis com a instalação local.
2. Entre no mapa do jogo antes de executar `injector.exe`.
3. Use Insert para abrir ou fechar o menu.
4. Consulte configurações e logs em `Documentos/kiero-dx11-base`.

O futuro launcher usará `Documentos/ZB2Menu`. Essa migração ainda não faz parte da implementação atual.

## Testes

```powershell
python tests/check_runtime_ownership.py
python tests/run_aim_validation.py
python tests/run_managed_aim_validation.py
python tests/run_modifier_bridge_validation.py
python tests/run_range_validation.py
```

Os testes gerenciados exigem a dependência Harmony gerada pelo build anterior. Os testes de interface usam ImGui com dados simulados, sem controlar a tela do jogo.

<h2><img src="https://api.iconify.design/solar/danger-triangle-bold.svg?color=%23F97316&width=24" alt="" /> Limitações conhecidas</h2>

| Área | Estado |
|---|---|
| Multiplayer | Há caminhos para solo, host e cliente; aceitação de efeitos pelo servidor e estabilidade prolongada precisam de validação por função. |
| Controles pendentes | Teleporte, chams, ESP de aliados e outros controles sem execução estão identificados como pendentes. |
| ESP distante | Registros sem modelo carregado usam posição conhecida; não há vida ou ossos inventados. |
| Modificadores legados | Inventário, moedas e desbloqueios têm comportamento distinto de restaurar parâmetros de armas e movimento. Consulte a auditoria. |
| Distribuição | Launcher, autenticação de acesso e atualização automática são planejamento, não recursos entregues. |
| Licenciamento | Origem e avisos de terceiros precisam ser consolidados antes de distribuição pública. |

## Organização e documentação

- [Estrutura e arquivos gerados](docs/ESTRUTURA.md)
- [Auditoria de modificadores e ESP](docs/AUDITORIA_MODIFICADORES_ESP.md)
- [Estabilidade da renderização](docs/ESTABILIDADE_RENDER.md)
- [Alcances de entidades](docs/DISTANCIAS.md) e [raios independentes](docs/RAIOS_ESP.md)
- [Histórico](CHANGELOG.md), [contribuição](.github/CONTRIBUTING.md) e [licenciamento](NOTICE.md)
