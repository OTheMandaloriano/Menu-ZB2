---
name: zb2-mono-safety
description: Use ONLY when touching Unity Mono memory access in ZB2 Menu mono.cpp/main.cpp (invokes, field reads, entity lists, hooks). Safety rules learned from real crashes. Read-only reference.
---

# Segurança de memória Unity Mono (ZB2 Menu)

Aprendida em 10+ dumps reais (kill-window, LOD destroy, loading AV).

## Regras duras

1. **Present nunca invoca.** `hkPresent` só copia snapshot + desenha.
   Toda leitura/invoke roda na worker (`EspThread`).
2. **Worker lê com SEH/probe.** Objeto Mono morre por GC/destroy a
   qualquer momento; todo acesso a ponteiro precisa de guarda.
3. **Kill-window:** entidade pode morrer ENTRE duas leituras do mesmo
   ciclo — revalidar HP/isAlive antes da segunda leva de invokes.
4. **Sem invoke no loading/menu/mapa assentando.** Checar vivacidade
   da cena (`SceneAlive`, lista oscilando) antes de ler; dormir em vez
   de insistir.
5. **Offsets só de `memory/OFFSETS.md`.** Novo offset = auditoria +
   CE MCP em partida, nunca chute, nunca hardcode de endereço de
   sessão (heap `0x22...` morre no restart).
6. **Injetar SOMENTE em partida**, nunca no loading (hook no meio da
   remontagem do LOD = AV). Confirmado também por trainers públicos
   do jogo ("activate mods after entering the game world").
7. **Snapshot double-buffer, Present sem lock.** `TryEnter` em toda
   espera; contadores lock-free.

## Limites

- Não fala de desenho (ver `zb2-esp-patterns`) nem de orçamento
  (ver `zb2-horda-perf`).

## Atualização da DLL nesta sessão autorizada

O usuário autorizou encerrar e reabrir o jogo pelo terminal para atualizar o menu.
Não pedir essa mesma permissão novamente. Nunca descarregar/reinjetar uma DLL já
carregada: confirmar processo e artefatos, fechar o jogo, verificar que encerrou,
copiar os binários compilados e conferir hashes, reabrir o jogo com seu diretório
de trabalho e aguardar confirmação de entrada no mapa. Só então executar o
injetor e confirmar módulo/log. Não controlar mouse, teclado nem telas do jogo.
Uma compilação não equivale a implantação ou validação em partida.
