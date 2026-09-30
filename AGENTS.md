# ZB2 Menu — Zumbi Blocks 2 (Unity 6 Mono x64, DLL interna D3D11)

Use o modelo e as instruções da sessão atual; este arquivo não define um modelo fixo.
Regra: 1 skill dominante por mensagem. Não misturar ESP + offset + perf.

## Skills (todas em `.opencode/skills/`, escopo deste projeto)

| skill | gatilho — carregar QUANDO |
|---|---|
| `zb2-mono-safety` | mexer em `mono.cpp`/`main.cpp`, invoke, entity list, hook, crash |
| `zb2-esp-patterns` | desenhar/mudar ESP em `gui.cpp` |
| `zb2-aimbot` | mexer no aimbot (`WohaxAim`, `WohaxLookAt`, `WohaxDistPx`, aba PLAYER) |
| `zb2-horda-perf` | ESP lento/travando com horda |
| `zb2-ce-mcp` | ler/mudar valor vivo no jogo via CE MCP |
| `zb2-dnspy-mcp` | dado estático do `Assembly-CSharp.dll` via MCP local (classe, campo, IL, enum, callers) |
| `zb2-dnspy-auto` | LEGADO (dumpasm sumiu do Temp) — usar `zb2-dnspy-mcp`; só se o MCP cair |
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

Máquina de estados wohax portada: eleição por px do crosshair →
sticky 500ms → mira vetor 3D (`bone3d − camPos`) com firing=1.
Pipeline: `WohaxAim` (mono.cpp) → snapshot `EspEntry` → `PlayerCamera`
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
