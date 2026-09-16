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

## Hang 01:39–01:43 (PID 18704, DLL C128BC8F) + fix kill-window melee

- Tail: `[PI-CALL] raycast n=11495–12730 fail=100% seh dt_avg 39–66µs`
  (12k unwinds/5s) → worker viva invocando, Present travou (delta CPU ~0,5s,
  50 threads em Wait). Snapshot sem lock inocentado como causa.
- Diagnóstico: kill-window do `7650f54` estreita demais — cobre topo do ciclo,
  mas o zumbi morre ENTRE `CollectJoints` (18 GetPos) e os 5 raycasts do LOS.
  `hp2 != hp` ainda skippa dano normal (60→40→20), aceitável: entidade em
  transição sai do LOS por 1 ciclo e volta (histerese segura a cor).
- CE §2 no hang: bridge vivo, `enum_modules` OK (104, `kiero size 688128`),
  53 threads, `ASM_COUNT=142`, `CLASSES=1358`, nomes indisponíveis nesta
  build do CE (limitação registrada, sem invoke no hang).
- Fix (commit `acff2c8`, DLL `4B170D90…` 611840 bytes): revalidação
  HP/isAlive entre skeleton e LOS no caminho 3D; morrendo = publica
  box+skeleton e pula os 5 raycasts (cor anterior mantida).
- `injector_state.txt` = `611840-134340076975175614` = DLL do fix ✅.
- Jogo PID 18904 + injetor disparado; pendente: 10 kills multidão 40+.

## Teste 01:50 INVÁLIDO + rebuild limpo (16/09)

- Crash 01:53:09 com handler (`Crash_2026-09-16_045310418`, `Created #216`)
  carregou DLL com `SizeOfImage 741376` — a do fix tem 611840. Build/string
  tabelam diferente, mas `SizeOfImage` não mente: era OUTRA build (link
  incremental com `.obj` velhos: `main.obj` de 15/09 21:58 no meio).
  Prova adicional: zero `[PI-CALL]/[PERF]/[BUDGET]` na sessão.
- Ação: `build/intermediates/Release_x64` apagado, rebuild limpo full
  (2608 funções, sem IPDB reaproveitado) → DLL `9B2C07AC…` 611840 bytes
  (commit `f2606b8`). Tail da morte válido como sintoma (kill melee
  `...8F140 hp=56/80` → `...8F640`), não como validação.
- Sessão 02:00: `injector_state.txt` = `611840-134340083136094130` ✅ =
  DLL do rebuild. Init limpo (`LOS OK`, `[BUDGET] 0/200`). Jogo PID 21952.
- Pendente: 10 kills multidão 40+ com `[PI-CALL]` discriminado no log.

## Item 0 SizeOfImage fantasma — causa raiz: git corrompia a DLL

- Crash logs 01:53 e 02:00 reportavam `size: 741376`; disco = 611840.
- Prova: `git cat-file -p <blob DLL>` = 1233214 bytes (blob maior que o
  arquivo!) e `git cat-file -s` = 611840. Com `core.autocrlf=true` e sem
  `.gitattributes`, o git aplicava conversão CRLF no binário na ida/volta:
  `SizeOfImage` 611840 virava 741376 no módulo carregado. Não eram "4 cópias"
  nem injetor errado — `FindDll` só tem 1 DLL no disco (varredura completa:
  projeto + Documents + Desktop + Downloads).
- Fix (commit `63027ef`): DLL e injector.exe REMOVIDOS do git
  (`git rm --cached`) + `.gitignore` com `build/ *.obj *.ipdb *.tlog
  *.dll.old *.iobj *.pdb`. Fingerprint passa a ser SHA256 +
  `injector_state.txt` (size-mtime), nunca git.
- Injetor agora loga `DLL caminho[N]: <PATH> (SizeOfImage=0x...)` por inject
  (função `PeSizeOfImage`, lê do PE no disco antes do inject).
- `SceneAlive` real: além do loader, checa `MainCamera.instance` +
  `cam+32 != null` + lista de zumbis com size 0..512. Transição (kill =
  Destroy, loading = troca de cena) = worker dorme 500ms.
- DLL `CD3AF6E1…` 612352 bytes (rebuild pós-remoção) + injector rebuildado.
- Terceiro modo mapeado: hang-kill, AV-kill, AV-loading — comum: invoke sem
  vivacidade em transição. `Raycast n=360 fail=140 seh` no loading confirma.

## Crash 02:17 modo novo + gate geração de mapa (16/09)

- Stack managed NOVO (não é `UpdatePhysics`): `LODTarget:InitialForceCull` →
  `LODCollider:SetColliding` → `LotConstructor:AddFurnitureToSetsAndInteractables`
  → `GenerateBuildingFurniture` → `GenerateHashCellBuildings` →
  `MapHash:InternalGenerateCell` → `LODController:RequestGenerationFor` →
  `LODForCell` → `UpdateLOD` → AV `0xc0000005` offset `0x19f4924`.
  `Created #224` → `Crash!!!` com câmera no céu (loading).
- Quarto modo: hang-kill, AV-kill, AV-loading, AV-geração-de-mapa. Comum:
  jogo destrói/recria objetos enquanto a worker invoca.
- Item 0: `SizeOfImage 741376` persiste com DLL 612352 no disco e
  `injector_state` correto. Não é mais o git (DLL fora do versionamento).
  `SizeOfImage` do crash log é o mapeado (seções alinhadas), não o tamanho
  do arquivo — comparar com o `SizeOfImage` do PE no disco (log do injetor),
  não com bytes do arquivo.
- Fix (commit `066590f`): `MapSettling()` — lista de zumbis oscilando >= 8
  em 2 ciclos seguidos = mapa assentando: worker dorme 500ms, zero invoke.
  DLL `C75E8405…` 612352 bytes.
- Pendente: 10 kills + tail (`[PI-CALL]`, `[PERF]`, `[SCENE]`).

## Crash no inject 02:29 (PID 18308) — hook no meio do loading

- Sintoma: crash ~20s após o inject, tela de loading (céu, `6:10`, sem zumbi).
- Log da DLL PARA em `[BUDGET] 51/200 entidades=9` — sem `MainThread concluída`,
  sem `Mono bind OK`, sem `[SIG]`. A MainThread morreu antes de terminar o init.
- Nosso log recomeça 02:29:28 (nova MainThread?) e crasha 02:29:50
  (`[PI-CALL] raycast n=5670 fail=1 seh`, 22s de vida). Duas vidas no mesmo log.
- Crash log: `size 741376` (item 0 ainda aberto — confere `SizeOfImage` do PE
  no disco, não bytes do arquivo), `Created #221` → `Crash!!!`, stack nativo
  SEM frame managed, offset `0x19f248b` (mesma região dos LOD).
- Causa: `MainThread` fazia `kiero::init` + bind + WndProc assim que achava
  QUALQUER janela — incluindo splash/loading com o `MapHash` gerando células.
  Hook no meio da remontagem do LOD = AV. Padrão UC: nunca hookar D3D durante
  load; esperar a janela estabilizar (message loop rodando).
- Fix (commit `7356597`): MainThread exige 4 leituras iguais da janela
  (~200ms estável, ~10s max); sem janela, aborta com log em vez de hookar
  no escuro. DLL `150EFCAA…` 612864 bytes.
- REGRA NOVA: injetar SOMENTE em partida (nunca no loading/menu).
  O `SceneAlive`/`MapSettling` cobre a worker; este cobre a MainThread.

## Auditoria hang 02:44 (86 zumbis, AV 0x19eee86 LOD) — causa raiz tripla

Evidência: `[PI-CALL] raycast n=15431 fail=1604 (seh=1604, 10%) dt 5µs` +
`Mono live zombies=86` + stack `SetActive→SetColliding→UpdatePhysics` +
print com ~40 boxes (metade fora da tela, atrás da câmera).

1. **Budget 200 sem gate on-screen**: 96 ent/ciclo x 5 raycasts mesmo fora da
   viewport. O cull de tela existia só no caminho 2D; o 3D (o que roda em
   horda) não tinha. Fix: centroide dos 8 cantos antes dos 5 raycasts
   (fora = skip, mantém cor).
2. **Kill-window `hp2 != hp` skippa dano normal** (60→40→20): entidade em
   combate some do LOS por 1 ciclo e a histerese segura cor velha = "não
   atualiza". Fix: só skippa se MORREU (`!alive3 || hp3 <= 0`).
3. **SEH em massa = sintoma, não causa**: `hd` inválido porque o raycast leu
   o corpo errado (lista reciclada + sem gate on-screen). Com os gates, o
   volume de invokes cai e o SEH some junto.
- Fix (commit `2471ffb`, DLL `B28489A7…` 612864 bytes).
- Padrão UC aplicado: broadphase (distância + on-screen) antes de qualquer
  trace; trace só em quem desenha; decay/histerese segura o resto.
  (Sem link colado: padrão canônico de ESP, justificativa aqui.)

## Hang 03:05 (multidão, oscilação visível/invisível) — causa: ciclo de 1438ms

- `[PI-CALL] raycast n=6119 fail=700 seh dt_avg 2358µs` + `[PERF] ciclo 1438.6ms`.
  O raycast foi de 5µs para 2358µs: PhysX saturado com 86 zumbis invocando
  junto. O ritmo adaptativo só ia até 66ms — sem freio pro ciclo de 1.4s.
- Oscilação verde/vermelho: histerese de 3 ciclos com ciclo de 1.4s = a cor
  leva ~4s pra estabilizar; no meio, cada ciclo alterna. Com rodízio 1/3, a
  cor atualiza a cada ~100ms e estabiliza em ~300ms.
- Item 0 FECHADO: `SizeOfImage 0xB5000` lido do PE no disco = 741376, igual
  ao crash log. Era alinhamento de seção, nunca foi fantasma. A regra
  "injetar em partida" continua (hook no loading = AV).
- Fix (commit `6ffb88a`, DLL `F363FCE9…` 612864 bytes): rodízio LOS
  (perto todo ciclo, longe 1/3) + teto 200ms no ritmo adaptativo.
- Crash 03:05: mesmo stack LOD `UpdatePhysics` offset `0x19eee86` (6º dump).
  O LOD crasha sozinho sob carga; nosso trabalho é não estar invocando junto.

## Crash 03:15 no tiro em multidão — stack novo + oscilação (16/09)

- Stack managed NOVO (7º dump, outro modo): `PlayerArms:ShootGun` →
  `PhysicalGun:Shoot` → `ShootSingleProjectile` → `PlayerShot` →
  `OnPlayerShot` → `StartCollidersByLine` → `StartColliding` →
  `ZombieLimbColliders:SetColliding` → `Collider:set_enabled` → AV
  `0xc0000005` offset `0x19eee86` (mesma região LOD, outro gatilho).
  O TIRO liga colliders em massa; nosso raycast testa os mesmos colliders
  na mesma janela = corrida.
- `[PI-CALL] raycast n=6119 fail=700 seh dt_avg 2358µs` → `n=340 fail=340`
  (100% SEH no fim) + `[PERF] ciclo 1438ms`. PhysX saturado: raycast de
  5µs foi pra 2358µs.
- Oscilação verde/vermelho: histerese de 3 ciclos com ciclo de 200ms–1.4s
  nunca estabiliza. Fix: verde exige 5 ciclos, vermelho 3 (assimetria
  intencional), entidade nova nasce vermelha.
- Fix (commit `727e105`, DLL `670FF792…` 612864 bytes): histerese dura +
  budget 96→64 raycasts/ciclo.
- Item 0 segue fechado: crash log `size 741376` = `SizeOfImage 0xB5000` do PE.

## Hang 03:24 sem crash handler (21 zumbis, raycast limpo) + inversão de cor

- `[PI-CALL] raycast n=1780 fail=2 seh` (0,1%), `zombies=21-26`, sem pasta
  nova em `Crashes/`, sem Event 1000. Hang puro (Present travou, worker
  parou), não AV. Com 21 zumbis o volume não explica — o hang veio de outro
  lugar (a investigar: deadlock Present/worker fora do snapshot).
- Inversão verde/vermelho: entidade nova nascia VERMELHA direto
  (`s_hystShown=false`), antes de qualquer raycast. Quem estava visível
  nascia vermelho, quem estava oculto... também. Depois a histerese corrigia
  alguns e outros não = "uns verde quando visível, outros vermelho".
  Fix: nasce na cor CRUA do 1º ciclo (commit `ae2333a`).
- DLL `4B18B2A1…` 612864 bytes.

## Crash 03:33 no tiro parado + pisca-pisca com poucos zumbis (16/09)

- Stack: `ShootGun→Shoot→ShootSingleProjectile→PlayerShot→OnPlayerShot→
  StartCollidersByLine→StartColliding→SetColliding→set_enabled` → AV
  `0x19eee86` (8º dump, mesma região LOD). Com 3-5 zumbis o volume não
  explica: o gatilho é o TIRO ligando colliders na mesma janela do raycast.
- Pisca-pisca com poucos zumbis: o RODIZIO 1/3 alternava a cor entre ciclos
  mesmo sem pressão de budget. Rodízio removido dos 2 caminhos (3D e 2D);
  custo controlado pelo budget (64) + histerese dura (5 verde / 3 vermelho).
- Fix (commit `aa68d8b`, DLL `9F862392…` 612864 bytes).
- Padrão UC/Unity aplicado: 1 raycast por entidade por ciclo seria o ideal;
  aqui 5 pontos com budget global + tri-estado + decay. QueryTrigger usa o
  global do projeto (overload confirmado não tem o parâmetro).

## Hang 03:47 sem crash handler (raycast limpo) — deadlock em lock

- `[PI-CALL] raycast n=3795 fail=1` (0,03%), `zombies=6`, sem pasta nova,
  sem Event 1000, 50 threads em Wait, delta CPU 0,17s/5s. Hang puro com
  jogo leve = deadlock, não overload, não AV.
- Auditoria de locks: `Enter` bloqueante em `BuildEsp` (virada double-buffer,
  `mono.cpp`), em `Sample` (contadores `s_mapFail/s_mapOk`, `main.cpp`), e
  `Enter` no `else` da worker + `GetEsp` antigo. Se o Present suspender
  (resize/foco/GPU), a worker trava dentro do CS = tela congelada com ESP.
- Fix (commit `2a0146a`, DLL `885827F4…` 612352 bytes): `TryEnter` em TODA
  espera de lock (virada pula pro próximo ciclo); contadores via
  `InterlockedIncrement` (sem CS). Zero `Enter` bloqueante no frame/worker.
- CE no hang: bridge vivo, 52 threads, `ASM_COUNT=142`, `CLASSES=1358`
  (nomes indisponíveis nesta build do CE — limitação registrada).

## Anomalias para REVALIDAR (não corrigir neste ciclo)

- `stam=-20` fixo e `eyeY/footY` congelados no `Mono live` — cheiro de campo com offset
  defasado, mas não trava nada; revalidar via CE MCP (`readFloat` nos offsets
  `OFFSETS.md`) antes de mexer.
- `ext=(0,0,0)` no caminho 2D — **esperado** (`mono.cpp:1543` zera de propósito).
- `gui.cpp` diff só-BOM + `nSeg` (cosmético, sem efeito em freeze).
