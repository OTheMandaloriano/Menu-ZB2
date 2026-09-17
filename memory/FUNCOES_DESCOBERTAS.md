# FUNCOES_DESCOBERTAS.md

> Sistemas novos alem da lista (teleporte, coleta em massa, spawn controlado etc).

## REMOVIDO 17/09 — Visible Check (item 14)
Decisão: 1 cor por elemento; sem teste de oclusão o par de cores não tem
sentido. Código removido por completo (nunca existiu): flags, UI, JSON,
cores em par, engine de oclusão, estabilização, leitura de profundidade
D3D11, telemetria, logs. Detalhe histórico preservado abaixo.

- **Metodo (removido):** teste multi-ponto em 5 ossos, 1 invoke por ponto.
- **Regra proporcional (removida):** maioria dos pontos OU cabeça exposta.
- **Fallback (removido):** leitura de profundidade D3D11 (1x/Present + worker).
