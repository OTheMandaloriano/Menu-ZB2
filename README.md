<div align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="apps/shared/brand/deadblock-menu.svg" />
    <img src="apps/shared/brand/deadblock-menu-light.svg" alt="DEADBLOCK" width="340" />
  </picture>
  <p>Uma interface para jogar. Um painel para gerenciar o acesso.</p>
  <p>Menu nativo para Zumbi Blocks 2, com loader Windows e gerenciamento de clientes e equipe.</p>
  <p><a href="docs/COMECE-AQUI.md">Comece aqui</a> · <a href="docs/DESENVOLVIMENTO.md">Desenvolvimento</a> · <a href="docs/README.md">Documentação</a> · <a href="docs/idiomas/README.en.md">English</a></p>
  <p><a href="README.md"><img src="https://api.iconify.design/flag/br-4x3.svg?width=26" alt="Português" title="Português" /></a>
  &nbsp;
  <a href="docs/idiomas/README.en.md"><img src="https://api.iconify.design/flag/us-4x3.svg?width=26" alt="English" title="English" /></a></p>
</div>


[![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)](docs/DESENVOLVIMENTO.md)
[![Windows](https://img.shields.io/badge/Windows-x64-0078D4?style=for-the-badge)](docs/INSTALACAO.md)
[![DirectX](https://img.shields.io/badge/DirectX-11-107C10?style=for-the-badge)](docs/ARQUITETURA.md)

[![Canal](https://img.shields.io/badge/canal-pr%C3%A9--release-2563EB?style=flat-square)](https://github.com/OTheMandaloriano/Menu-ZB2/releases)
[![Licenciamento](https://img.shields.io/badge/licenciamento-ver_NOTICE-EAB308?style=flat-square)](NOTICE.md)
[![Guias](https://img.shields.io/badge/guias-instala%C3%A7%C3%A3o_e_atualiza%C3%A7%C3%A3o-10B981?style=flat-square)](docs/COMECE-AQUI.md)

<h2><img src="https://api.iconify.design/solar/info-circle-bold.svg?color=%232563EB&width=24" alt="" align="top" /> O projeto</h2>

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

## Prévias

Interfaces renderizadas pelo código do produto com dados fictícios. O estado de
carregamento nas prévias é simulado; não representa homologação em partida.

![Menu DEADBLOCK](docs/previews/menu.png)

| Painel Admin | Cliente Windows |
|---|---|
| ![Admin](docs/previews/admin-clientes.png) | ![Loader](docs/previews/loader-carregado.png) |

![Biblioteca do cliente](docs/previews/loader-biblioteca.png)

A biblioteca tem navegação lateral e retorno entre telas. A capa é uma ilustração
original, não uma captura do jogo. [Guia de navegação](docs/LOADER-NAVEGACAO.md).

[Ver todas as telas e como atualizar as imagens](docs/PREVIAS.md).

## Downloads

A entrega de validação está em [Releases](https://github.com/OTheMandaloriano/Menu-ZB2/releases).
Use `01-CLIENTE-INICIAL.zip` para o primeiro envio ao jogador e `01-EQUIPE-INICIAL.zip`
para o integrante que emitirá licenças. Ambos começam sem ativação pessoal.

O repositório é privado: releases só ficam acessíveis a contas autorizadas. Envie
os ZIPs gerados pelo Admin a usuários sem acesso ao repositório. Não há atualização
automática pelo GitHub nesta versão.

## Instalar pela primeira vez

| Quem vai usar | Precisa de |
|---|---|
| Jogador | Windows x64 compatível, driver DirectX 11, jogo compatível e licença emitida para seu PC |
| Equipe emissora | Windows x64, driver DirectX 11, .NET Framework e autorização do proprietário; não precisa instalar o jogo para emitir |
| Desenvolvedor | Ferramentas C++/C#, Python e dependências descritas no guia de desenvolvimento |

[Passo a passo de instalação e dependências](docs/INSTALACAO.md).

> [!IMPORTANT]
> Baixar o Admin em outro PC não recupera o acesso de proprietário. A chave atual é
> protegida pelo usuário Windows. Exporte antes o backup protegido por senha no Admin. Ele não inclui o histórico. Leia [troca de computador e recuperação](docs/RECUPERACAO.md) antes de formatar.

## Usar e distribuir

No Admin, a aba **Como enviar** gera os programas iniciais, sem exigir ID ou solicitação.
O cliente devolve seu ID; o integrante devolve a solicitação da estação. Depois disso,
o proprietário gera e envia o pacote ativado ou autorizado.

- [Primeiro envio para clientes e equipe](docs/ADMIN.md)
- [Atualizar quem já utiliza o programa](docs/ATUALIZAR-USUARIOS.md)
- [Dados pessoais, resíduos e limpeza segura](docs/RETENCAO-E-LIMPEZA.md)

**Não envie o repositório ou a pasta `build`.** Os programas e ZIPs ficam em `dist`.
Chaves, perfis e histórico não fazem parte dos pacotes.

<h2><img src="https://api.iconify.design/solar/danger-triangle-bold.svg?color=%23F97316&width=24" alt="" align="top" /> Estado atual e limitações</h2>

| Área | Situação |
|---|---|
| Cliente | 1.16-local; ativação e carregamento implementados |
| Admin | Interface 1.16, com o cliente 1.16 incorporado |
| Atualizações | Envio manual de ZIPs; atualizador GitHub ainda é uma proposta |
| Limpeza | Serviços antigos do Admin, após análise; runtime antigo do cliente não é removido automaticamente |
| Validação | Testes automatizados e observações locais documentados; não equivalem a homologação completa de todas as funções em partida |
| Recuperação do proprietário | Backup de acesso por senha disponível; histórico não incluído |
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

O arquivo `Deadblock.Menu.vcxproj` permanece na raiz como entrada do Visual Studio.
As pastas `.agents` e `.opencode` pertencem às ferramentas de desenvolvimento.
Veja a [estrutura detalhada](docs/ESTRUTURA.md) e o [mapa do código](docs/ARQUITETURA.md).

## Desenvolver

Leia [AGENTS.md](AGENTS.md) e o [guia de desenvolvimento](docs/DESENVOLVIMENTO.md).
Requisitos: Windows x64, MSVC v145, Windows SDK, Python e as ferramentas indicadas no guia.
As assemblies proprietárias do jogo são usadas da instalação local; não estão neste repositório.

```powershell
msbuild Deadblock.Menu.vcxproj /p:Configuration=Release /p:Platform=x64
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
