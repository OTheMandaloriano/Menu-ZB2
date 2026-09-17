---
name: zb2-horda-perf
description: Use ONLY when ESP slows down or hangs with many zombies in ZB2 Menu (crowd, high invoke count, long cycle). Budget and culling rules measured in-game. Read-only reference.
---

# Performance em horda (ZB2 Menu)

Nascida de medição real: ciclo de 1438ms com 86 zumbis (16/09).
Jogo gera horda via `WaveSpawner` (até ~95 vivos).

## Regras (nesta ordem, parar quando resolver)

1. **Orçamento de invokes/ciclo** (atual: 64). Estourou = pula resto.
2. **Gate on-screen antes de qualquer invoke.** Fora da viewport =
   skip, sem tocar no jogo.
3. **Cull por distância** (`fMaxDistance`) antes de invocar.
4. **Teto no ritmo do ciclo** (atual: 200ms). Ciclo estourou = dorme,
   não acumula.
5. **Elemento caro com limite próprio:** skeleton só < 50m.
6. **Régua pública:** impacto < 5% FPS, < 5% de um core (cs2Menuss);
   aceite do projeto: 10 kills em multidão 40+ sem hang, p95 < 15ms.

## Proibido

- Aumentar orçamento/teto para "ver se melhora" sem medir antes
  e depois no log (`[BUDGET]`, `[PERF]`, `[PI-CALL]`).
- Aplicar culling que muda o visual sem avisar o operador.
