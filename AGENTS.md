# ZB2 Menu: Zumbi Blocks 2 (Unity 6 Mono x64, DLL interna D3D11)

Use o modelo e as instruções da sessão atual; este arquivo não define um modelo fixo.
Regra: 1 skill dominante por mensagem. Não misturar ESP + offset + perf.

## Skills (todas em `.opencode/skills/`, escopo deste projeto)

| skill | gatilho: carregar QUANDO |
|---|---|
| `zb2-mono-safety` | mexer em `mono.cpp`/`main.cpp`, invoke, entity list, hook, crash |
| `zb2-esp-patterns` | desenhar/mudar ESP em `gui.cpp` |
| `zb2-aimbot` | mexer no aimbot (`DeadblockAim`, aba PLAYER) |
| `zb2-horda-perf` | ESP lento/travando com horda |
| `zb2-ce-mcp` | ler/mudar valor vivo no jogo via CE MCP |
| `zb2-dnspy-mcp` | dado estático do `Assembly-CSharp.dll` via MCP local (classe, campo, IL, enum, callers) |
| `zb2-dnspy-auto` | LEGADO (dumpasm sumiu do Temp): usar `zb2-dnspy-mcp`; só se o MCP cair |
| `zb2-pesquisar-antes` | dúvida de padrão ANTES de codar (1 rodada websearch, sem loop) |
| `graphics-api-hooking` | fundo: hook Present slot 8, RTV, WndProc, W2S guard |
| `game-engine-resources` | fundo: Unity Mono, metadata ≠ offset |

## Hierarquia (sem exceção)

1. `memory/OFFSETS.md` + medição do projeto (log `[AIM]`/`[ESP+]`/`[BUDGET]`/`[SKEL]`, print) vencem tudo.
2. `zb2-*` vencem as 2 genéricas.
3. Genérica é fundo, nunca receita.

## Proibido (não sugerir nunca)

- Overlay externo/transparente, DWM, hijack Steam/NVIDIA (somos DLL interna).
- IL2CPPDumper/Dumper-7 (build é Mono), chute de offset, hardcode de heap `0x22…`.
- Invoke Mono dentro do `hkPresent`; `enumClasses` em imagem inteira; clique em CE/dnSpy.
- Arquitetura external/CS2/EAC/DMA para este projeto.
- Editar código a partir de `zb2-pesquisar-antes` sem fonte.

## Aimbot (resumo; detalhe em `zb2-aimbot`)

Máquina de estados da mira: eleição por px do crosshair →
sticky 500ms → mira vetor 3D (`bone3d − camPos`) com firing=1.
Pipeline: `DeadblockAim` (mono.cpp) → snapshot `EspEntry` → `PlayerCamera`
pitch `set_Angle` + yaw `Transform.Rotate`. Sem `IsVisible` ainda
(raycast é o próximo ciclo). Flags de vitrine (`bSilentAim`,
`bAutoFire`, `bTriggerbot`, `bPrediction`, `fLagComp`) NÃO executam.

## Manutenção do Admin e loader

Leia docs/ARQUITETURA.md para o mapa do código e a diferença entre build, dist e Documentos.
Siga docs/ATUALIZACOES.md para compilar e distribuir. O projeto canônico está em D:/Projeto/ZB2 Menu.
Não use cópias antigas de trabalho como fonte da verdade. Preserve alterações locais e o índice Git.
A limpeza de cache pertence a apps/admin/native/settings.cpp: somente serviços antigos verificados, nunca perfis ou runtime do jogo.
Diagnóstico de antivírus: docs/ANTIVIRUS.md. Não desative proteção nem prometa ausência de detecção.
A publicação em main depende de autorização explícita. Guias de cliente/equipe: docs/ADMIN.md.

## Continuidade obrigatória entre sessões

Para manutenção do menu, loader, Admin ou distribuição, comece por docs/COMECE-AQUI.md e docs/CONTINUIDADE.md.
Confira o estado real do Git e do disco; não trate recursos planejados como implementados.
A política atual e a proposta de retenção estão em docs/RETENCAO-E-LIMPEZA.md.
Atualize o guia de continuidade e as evidências ao encerrar uma entrega.

Organização dos fontes: menu nativo em `src/menu/`; detalhes em [docs/DESENVOLVIMENTO.md](docs/DESENVOLVIMENTO.md).

## Identidade pública e caminhos portáveis

- Marca do produto: DEADBLOCK. Créditos públicos: OTheMandaloriano (The Mandalorian).
- Não inserir nomes pessoais ou caminhos C:/Users/<conta> nos fontes, exemplos ou fixtures.
- Usar Known Folder API para Documentos. Nunca presumir nome de conta, disco ou idioma.
- Não confundir crédito público com nome local da estação; preservar perfis e licenças existentes.
- Rodar `python tests/check_portable_identity.py` antes de publicar.
- Troca do nome da DLL exige validar em conjunto projeto, DllImport gerenciado, helper,
  manifesto, leitura de módulos e testes. Não renomear o executável isoladamente.
- Preservar avisos de licença e procedência dos componentes de terceiros.
- Leia docs/PORTABILIDADE.md. A migração Deadblock.Menu está aplicada nos fontes. Validar uma nova sessão do jogo antes de declarar a entrega homologada.

## Instalação sem identificação pessoal fixa

- Nunca pedir nome da conta Windows para montar o caminho de instalação: consultar Known Folders.
- Os guias também entram na verificação check_portable_identity.py.
- Não distribuir atalhos --data do ambiente de desenvolvimento, relatórios brutos ou backups pessoais.
- Não usar uma cópia antiga de D:/ZB2-Retomada como base de atualização do projeto oficial.
- Conferir Unicode, permissões e diretórios redirecionados antes de afirmar suporte a qualquer perfil.

## Prévias e publicação de entrega

Após alterações na interface, recompilar os testes e executar scripts/export_previews.py.
Revisar as imagens com dados fictícios, executar --check e versionar PNGs e manifesto.
Ler docs/PREVIAS.md. Não usar capturas com nomes, IDs, licenças ou notificações pessoais.
Distribuir executáveis Windows em GitHub Releases; GitHub Packages não é necessário
para ZIPs desse produto. Marcar como pré-release enquanto a nova DLL não tiver sido
validada em partida. Não publicar chaves, perfis, backups, logs ou símbolos nos ZIPs.

## Recuperação e publicação

Antes de orientar troca de PC, ler docs/RECUPERACAO.md. Não prometer que o GitHub ou
um arquivo DPAPI isolado restaura o proprietário. Backup portátil por senha não está implementado.
Guias para novos usuários: docs/INSTALACAO.md. Separar requisitos de execução dos de build.
Em README e publicação, aplicar as skills readme-profissional e versionamento-git
quando estiverem disponíveis no ambiente, respeitando a convenção do repositório.
