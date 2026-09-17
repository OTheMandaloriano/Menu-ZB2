---
name: zb2-esp-patterns
description: Use ONLY when drawing or changing ESP elements (box, name, health, skeleton, snapline, distance) in ZB2 Menu gui.cpp. Canonical patterns with sources. Read-only reference, never edits code by itself.
---

# Padrões canônicos de ESP (referência, só-leitura)

Pipeline universal: entity list → validar → WorldToScreen → desenhar.
(Fontes: CS2-External, RCi tutorial, Universal-Unity-ESP,
codereversing.com W2S série, GuidedHacking.)

## Regras (não reinventar)

1. **W2S rejeita por `w`:** `if (w < 0.001f) return false` antes de
   dividir. Projeto usa `clip.w > 0.05` (mais conservador, correto).
2. **Box 2D:** projeta topo e base; `height × 1.25`, `width = height / 2`.
   Projeto usa `0.6` (zumbi largo) — se a box parecer larga, o ajuste
   correto é `0.5`, nunca outro chute.
3. **Skeleton:** tabela de conexões fixa; desenha a linha **só se os
   DOIS ossos projetarem** (`skV[a] && skV[b]`). Checar `isfinite`.
4. **Snapline:** origem base ou topo da tela, destino no pé. Projeto tem
   Base/Topo/Centro (`iSnapFrom`) — cobre o padrão.
5. **Elementos padrão:** box, nome, vida, distância, skeleton, snapline,
   filtro de time. Nada fora disso sem pedido do operador.
6. **Present hook D3D11 + ImGui + MinHook, menu no INSERT:** molde
   universal (Sh0ckFR, kiraa024). Não trocar de técnica.

## Limites desta skill

- Referência de desenho apenas. Segurança de memória e performance
  estão em `zb2-mono-safety` e `zb2-horda-perf`.
- Em caso de conflito com medição do projeto (print, log), a medição
  vence e esta skill é atualizada depois, não ignorada na hora.
