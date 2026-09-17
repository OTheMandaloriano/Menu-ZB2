# FUNCOES_DESCOBERTAS.md

> Sistemas novos alem da lista (teleporte, coleta em massa, spawn controlado etc).

## REMOVIDO 17/09 — Visible Check (item 14, raycast LOS + depth buffer)
Decisão: 1 cor por elemento (padrão Gamesense `custom_esp.lua`); sem raycast
o par Visível/Oculto não tem sentido. Código removido por completo (nunca
existiu): flags, UI, JSON, cores Vis/Inv, engine LOS (Raycast/Linecast/
histerese/calibração), depth buffer D3D11 (staging/Map/Publish/Sample),
telemetria raycast, logs LOS/DEPTH. Detalhe histórico preservado abaixo.

- **Metodo (removido):** multi-bone raycast 5 pontos, `Physics.Raycast/5`.
- **Regra proporcional (removida):** hits >= 2 (40%) OU cabeça exposta.
- **Fallback (removido):** depth buffer D3D11 (staging 1x/Present + Map worker).
