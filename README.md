<div align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="apps/shared/brand/deadblock-menu.svg" />
    <img src="apps/shared/brand/deadblock-menu-light.svg" alt="DEADBLOCK" width="340" />
  </picture>
  <p>Menu nativo, loader e painel de licenças para Zumbi Blocks 2.</p>
  <p><a href="docs/COMECE-AQUI.md">Comece aqui</a> · <a href="docs/DESENVOLVIMENTO.md">Desenvolvimento</a> · <a href="docs/README.md">Documentação</a> · <a href="docs/idiomas/README.en.md">English</a></p>
</div>

## O projeto

DEADBLOCK reúne três componentes: uma DLL nativa integrada ao jogo, um cliente
Windows que valida o acesso e carrega o menu, e um Admin para emitir licenças e
autorizar integrantes da equipe. Este repositório privado contém o código, os
testes, os recursos visuais e a documentação.

| Componente | Local | Função |
|---|---|---|
| Menu | `src/menu/` e `managed/` | Interface D3D11 e integração com Unity Mono |
| Cliente | `apps/loader/` | Ativação offline, verificação do pacote e carregamento |
| Admin | `apps/admin/` | Clientes, equipe, pacotes e limpeza limitada de cache |
| Interface comum | `apps/shared/` | Marca, fontes, controles e renderização |

## Usar e distribuir

No Admin, a aba **Como enviar** gera os programas iniciais, sem exigir ID ou solicitação.
O cliente devolve seu ID; o integrante devolve a solicitação da estação. Depois disso,
o proprietário gera e envia o pacote ativado ou autorizado.

- [Primeiro envio para clientes e equipe](docs/ADMIN.md)
- [Atualizar quem já utiliza o programa](docs/ATUALIZAR-USUARIOS.md)
- [Dados pessoais, resíduos e limpeza segura](docs/RETENCAO-E-LIMPEZA.md)

**Não envie o repositório ou a pasta `build`.** Os programas e ZIPs ficam em `dist`.
Chaves, perfis e histórico não fazem parte dos pacotes.

## Estado atual

| Área | Situação |
|---|---|
| Cliente | 1.14-local; ativação e carregamento implementados |
| Admin | Interface 1.13, com o cliente 1.14 incorporado |
| Atualizações | Envio manual de ZIPs; atualizador GitHub ainda é uma proposta |
| Limpeza | Serviços antigos do Admin, após análise; runtime antigo do cliente não é removido automaticamente |
| Validação | Testes automatizados e observações locais documentados; não equivalem a homologação completa de todas as funções em partida |
| Segurança | Sem Authenticode; detecções da amostra anterior estão documentadas, sem falso positivo confirmado |

Consulte [continuidade](docs/CONTINUIDADE.md), [estados do loader](docs/ESTADOS-LOADER.md)
e [análise de antivírus](docs/ANALISE-VIRUSTOTAL.md) antes de uma nova entrega.

## Estrutura

```text
apps/          Admin, cliente e componentes de interface compartilhados
src/menu/      Código nativo do menu
managed/       Integração C# com o Mono do jogo
injector/      Helper nativo de carregamento
imgui/         Dependência de interface e adaptador FreeType
kiero/         Dependência de hook gráfico
packaging/     Manifestos e chave pública
scripts/       Compilação e empacotamento
tests/         Regressões e testes sem controlar o jogo
docs/          Guias de uso, arquitetura e histórico técnico
memory/        Referências técnicas e medições do projeto
build/         Gerado localmente; fora do Git
dist/          Entrega local; fora do Git
.local/        Credenciais, recuperação e auditorias; fora do Git
```

O arquivo `kiero-dx11-base.vcxproj` permanece na raiz como entrada do Visual Studio.
As pastas `.agents` e `.opencode` pertencem às ferramentas de desenvolvimento.
Veja a [estrutura detalhada](docs/ESTRUTURA.md) e o [mapa do código](docs/ARQUITETURA.md).

## Desenvolver

Leia [AGENTS.md](AGENTS.md) e o [guia de desenvolvimento](docs/DESENVOLVIMENTO.md).
Requisitos: Windows x64, MSVC v145, Windows SDK, Python e as ferramentas indicadas no guia.
As assemblies proprietárias do jogo são usadas da instalação local; não estão neste repositório.

```powershell
msbuild kiero-dx11-base.vcxproj /p:Configuration=Release /p:Platform=x64
python tests/check_repository_layout.py
python tests/check_runtime_ownership.py
python tests/run_aim_validation.py
```

O build do menu não gera sozinho os pacotes finais. A ordem de entrega é
**menu → loader → Admin → ZIPs**. Os comandos e verificações estão em
[ATUALIZACOES.md](docs/ATUALIZACOES.md).

## Contribuição e licenciamento

- [Como contribuir](.github/CONTRIBUTING.md)
- [Histórico de mudanças](CHANGELOG.md)
- [Avisos e licenciamento](NOTICE.md)
- [Índice da documentação técnica](docs/README.md)

Não inclua credenciais, licenças reais, dados de clientes, binários do jogo ou logs
pessoais em commits. Alterações de caminhos devem atualizar projetos e testes juntos.
