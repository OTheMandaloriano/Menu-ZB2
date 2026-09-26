# ZB2 Menu

Menu interno em C++/D3D11 com adaptadores C# para Unity Mono.

## Estrutura

| Caminho | Finalidade |
|---|---|
| `main.cpp`, `gui.cpp`, `mono.cpp`, headers e `.inl` na raiz | Entrada, interface, integração nativa e módulos auxiliares |
| `kiero-dx11-base.vcxproj` | Projeto Visual Studio da DLL nativa |
| `managed/` | Adaptadores C#; fontes necessárias para gerar `Zb2.AimBridge.dll` |
| `injector/` | Código, projeto e configuração do injetor |
| `imgui/`, `kiero/` | Dependências incorporadas ao código-fonte |
| `tests/` | Testes e modelos simulados, sem controlar a tela do jogo |
| `scripts/` | Identificação do ambiente e compilação gerenciada |
| `docs/` | Auditorias, comportamento e limitações das funções |
| `memory/`, `dump/` | Evidências de engenharia reversa e campos auditados; não são caches de compilação |
| `.agents/`, `.opencode/`, `AGENTS.md`, `opencode.json` | Instruções e configuração das ferramentas locais |
| `.git/` | Histórico do repositório |
| `build/` | Saídas geradas, ignoradas pelo Git |

## Saídas de compilação

`build/Release_x64/` contém a distribuição de desenvolvimento atual:

- `kiero-dx11-base.dll`: módulo nativo.
- `Zb2.AimBridge.dll` e `0Harmony.dll`: adaptadores e dependência de runtime; não remover.
- `Harmony.LICENSE`: licença da dependência distribuída.
- `injector.exe` e `config.ini`: execução e configuração do injetor.
- `kiero-dx11-base.pdb` e `.map`: símbolos e mapa correspondentes à DLL, mantidos para diagnosticar crashes. Não são necessários para executar, mas são úteis no desenvolvimento.

`build/intermediates/` é cache gerado pelo MSBuild. Pode ser removido quando não houver compilação em andamento e será recriado na próxima compilação. Sua presença é normal, não código residual.

Builds candidatos, DLLs `.old` e autosaves vazios de ferramentas não pertencem à distribuição atual nem à raiz. Backups de atualização devem ficar fora da árvore do projeto, identificados por data/versão. Logs e presets do usuário ficam em `Documents/kiero-dx11-base`, não entre os fontes.

## Compilação e verificações

Compile `kiero-dx11-base.vcxproj` em `Release|x64` e o projeto de `injector/` quando necessário. Gere os adaptadores com:

```powershell
python scripts/build_managed_aim.py --managed "CAMINHO_DO_JOGO/ZumbiBlocks2_Data/Managed"
```

O script verifica a versão do assembly do jogo e a integridade da dependência. Execute os testes aplicáveis em `tests/`; o runner nativo é `python tests/run_aim_validation.py`. Compilação e testes simulados não comprovam funcionamento em partida.

Fontes permanecem na estrutura atual para preservar includes e projetos existentes. Uma migração para `src/` deve atualizar os projetos e testes em uma alteração própria, não ser misturada à limpeza de artefatos.
