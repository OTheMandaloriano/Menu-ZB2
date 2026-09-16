# AUDIT-LOG.md — Reconciliação de sessões (item 14b, freeze em horda)

> Sessão válida em disco: **15/09/2026 20:16:36, PID 10672** (TIDs 11092/17020/7284),
> DLL `build/Release_x64/kiero-dx11-base.dll` de **15/09 17:56** (commit `3f09298`).
> O relato de `PID 22432 23:52 [ESP-GLITCH]` e AV 6000.3.21f1 **não existe no disco**:
> é de outra sessão/operador — tratado como referência externa, não como evidência.

## Linha de código por sintoma (tudo em `mono.cpp`, salvo nota)

| Sintoma | Linha | Evidência em disco |
|---|---|---|
| Freeze após minutos com spawn alto | `EspThread` 1615-1642 + `Tick` 1743 (`%30`) | `[PERF] ciclo 41.6ms -> intervalo 66ms` 20:27:16 + rampa `zombies=26→64→95→108→114` (log 20:16) |
| Custo escala com spawn | `BuildEsp` 1170+ / `CollectJoints` 1085 (18 `GetPos`) / `LosPointV` 690 (5 raycasts) | `Player.log: Created #221 zombies` + `PhysX Threading Mode: Multi-Threaded` |
| Depth morto mas custando Map | `main.cpp` `DepthVis::Sample` 250-301 + `Publish` no `hkPresent` 124-212 | `[DEPTH-ROW] sample=0 u/v fora [0,1]` + `[DEPTH-STAT] dsvFmt=19` |
| LOS funcional (não é o crash) | `LosPointV` + histerese `LosStable` 599 | `[SIG] Raycast(V3,V3,Hit&,f,i) CONFIRMADO` + `[LOS-GATE] 2D 1/1/1` + `[LOS-RESULT] losVis=1 losHits=2` |

## Anomalias para REVALIDAR (não corrigir neste ciclo)

- `stam=-20` fixo e `eyeY/footY` congelados no `Mono live` — cheiro de campo com offset
  defasado, mas não trava nada; revalidar via CE MCP (`readFloat` nos offsets
  `OFFSETS.md`) antes de mexer.
- `ext=(0,0,0)` no caminho 2D — **esperado** (`mono.cpp:1543` zera de propósito).
- `gui.cpp` diff só-BOM + `nSeg` (cosmético, sem efeito em freeze).
