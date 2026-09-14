# FUNCOES_DESCOBERTAS.md

> Sistemas novos alem da lista (teleporte, coleta em massa, spawn controlado etc).

## Visible Check profissional (item 14, 14/09)
- **Metodo:** multi-bone raycast 5 pontos (cabeca/peito/quadril/coxaL/coxaR), `Physics.Raycast/5`, maxDist = dist - 0.15m.
- **Regra proporcional:** visivel = hits >= 2 (40%) OU cabeca exposta (headshot viavel).
- **Threshold final:** 2/5 (padrao profissional; ajustar se sensivel demais).
- **RaycastHit.distance:** offset real via `mono_field_get_offset` (era chute +20).
- **Pontos:** 3D = fracoes da AABB; 2D = interpolacao olho->pe (0/25/50/75/100%).
- **GEOMETRY_MASK:** `-1` (tudo) ate auditoria de layers via CE MCP (Passo 2 pendente).
- **Layers ZB2:** [PENDENTE — preencher apos log [LOS-HIT]/CE MCP].
- **Fallback:** se raycast continuar 5/5 atras de parede (mesh sem colisor) -> depth buffer (Passo 5).
