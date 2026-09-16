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

## Rodada CE MCP 16/09 ~01:00 (PID 4064, jogo em partida, ESP on)

Comandos (só leitura, nenhum invoke/escrita no combate):
- `ping` → bridge v12.0.0 vivo; `open_process 4064` OK.
- `enum_modules` OK: `mono-2.0-bdwgc.dll @0x7FFA9C260000`, `UnityPlayer.dll`,
  `d3d11.dll`, `kiero-dx11-base.dll size 688128` (= `SizeOfImage` do crash log).
- `get_thread_list` → 54 threads (mapeadas Present/worker/game, sem breakpoint).
- `evaluate_lua LaunchMonoDataCollector()` → `LAUNCH_OK`;
  `mono_enumAssemblies()` → `ASM_COUNT=142`;
  `Assembly-CSharp` + `UnityEngine.PhysicsModule` + `UnityEngine.CoreModule` localizados.
- `mono_image_enumClasses` → `CLASSES=1358` (= dump 12/09), mas
  `mono_class_getName`/`getFullName` retornam vazio nesta build do CE
  (limitação do bridge, não do jogo) — `findInstancesOfClassListOnly`
  sem nomes fica para rodada com Mono ativado no CE GUI.
- Leitura direta validando `OFFSETS.md` no zumbi do log
  (`ent=0x21593CA5C80`, sessão anterior): `readBytes(z)=nil` com
  `UnityPlayer MZ=4D5A` legível = **página do objeto sumiu (GC/despawn)**.
  Janela kill por leitura confirmada: objeto morre e o slot invalida —
  invoke ali = AV. Evidência a favor do fix kill-window (`7650f54`).

## Rodada 1 — controle ESP-off NÃO TRAVOU (16/09 00:35→01:17, ~40min)

- Config: `bZombieEsp=false` + `bDebugOverlay=false` (únicas mudanças,
  `.bak-controle` guardado). DLL `309AA73C…` (`injector_state` `610816-...`).
- Combate real: `localHP 95→0 (morreu) →46`, coop 4–5 players, picos
  `zombies=78–95`. Zero hang, zero crash, sem handler.
- Tail: `Mono live` contínuo (TID 15536), `[BONE]/[JOINT]/[SKEL]` 00:36:24,
  sem `[PI-CALL]/[PERF]` (worker sem ESP não publica).
- Correção metodológica: `bDebugOverlay=false` NÃO tira `ReadAll`/`AuditBones`
  do `hkPresent` (`Mono live` a cada 5s prova que continuaram rodando).
  Testado = ESP-off + ReadAll-ON + depth-ON + watermark-ON, sem travar.
- Conclusão: `ReadAll`/`AuditBones` sozinhos não travam; worker sem ESP é sã.
  Crash exige o encontro: worker publicando + overlay desenhando + depth
  copiando — lock do snapshot + contexto imediato em iGPU Intel UHD.
- Fix correspondente: snapshot double-buffer sem lock no Present
  (commit `8faf6d3`, DLL `C128BC8F…`, item 1 do PROMPT P0-BISSECÇÃO §4).

## Anomalias para REVALIDAR (não corrigir neste ciclo)

- `stam=-20` fixo e `eyeY/footY` congelados no `Mono live` — cheiro de campo com offset
  defasado, mas não trava nada; revalidar via CE MCP (`readFloat` nos offsets
  `OFFSETS.md`) antes de mexer.
- `ext=(0,0,0)` no caminho 2D — **esperado** (`mono.cpp:1543` zera de propósito).
- `gui.cpp` diff só-BOM + `nSeg` (cosmético, sem efeito em freeze).
