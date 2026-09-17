# AUDIT-LOG.md — Reconciliação de sessões (item 14b, freeze em horda)

## Rodada CE MCP 16/09 ~01:00 (PID 4064, jogo em partida, ESP on)

Comandos (só leitura, nenhum invoke/escrita no combate):
- `ping` → bridge v12.0.0 vivo; `open_process 4064` OK.
- `enum_modules` OK: `mono-2.0-bdwgc.dll`, `UnityPlayer.dll`, `d3d11.dll`,
  `kiero-dx11-base.dll size 688128` (= `SizeOfImage` do crash log).
- `get_thread_list` → 54 threads (mapeadas Present/worker/game, sem breakpoint).
- `evaluate_lua LaunchMonoDataCollector()` → `LAUNCH_OK`;
  `mono_enumAssemblies()` → `ASM_COUNT=142`;
  `Assembly-CSharp` + módulos Unity localizados.
- `mono_image_enumClasses` → `CLASSES=1358` (= dump 12/09), mas nomes
  indisponíveis nesta build do CE (limitação do bridge, não do jogo).
- Leitura direta no zumbi do log: `readBytes(z)=nil` com `UnityPlayer`
  legível = **página do objeto sumiu (GC/despawn)**. Janela kill confirmada
  por leitura: objeto morre e o slot invalida — invoke ali = AV. Evidência
  a favor do fix kill-window (`7650f54`).

## Rodada 1 — controle ESP-off NÃO TRAVOU (16/09 00:35→01:17, ~40min)

- Config: `bZombieEsp=false` + `bDebugOverlay=false` (únicas mudanças,
  `.bak-controle` guardado).
- Combate real: `localHP 95→0 (morreu) →46`, coop 4–5 players, picos
  `zombies=78–95`. Zero hang, zero crash, sem handler.
- Tail: `Mono live` contínuo, `[BONE]/[JOINT]/[SKEL]` 00:36:24, sem
  `[PI-CALL]/[PERF]` (worker sem ESP não publica).
- Correção metodológica: `bDebugOverlay=false` NÃO tira `ReadAll`/`AuditBones`
  do `hkPresent` (`Mono live` a cada 5s prova que continuaram rodando).
- Conclusão: `ReadAll`/`AuditBones` sozinhos não travam; worker sem ESP é sã.
- Fix correspondente: snapshot double-buffer sem lock no Present
  (commit `8faf6d3`).

## Hang 01:39–01:43 (PID 18704) + fix kill-window melee

- Tail: 12k unwinds/5s → worker viva invocando, Present travou (delta CPU
  ~0,5s, 50 threads em Wait). Snapshot sem lock inocentado como causa.
- Diagnóstico: kill-window do `7650f54` estreita demais — cobre topo do ciclo,
  mas o zumbi morre ENTRE `CollectJoints` (18 GetPos) e os invokes seguintes.
- Fix (commit `acff2c8`, 611840 bytes): revalidação HP/isAlive entre skeleton
  e o resto do ciclo no caminho 3D; morrendo = publica box+skeleton.
- `injector_state.txt` = DLL do fix. Jogo PID 18904 + injetor disparado;
  pendente: 10 kills multidão 40+.

## Teste 01:50 INVÁLIDO + rebuild limpo (16/09)

- Crash 01:53:09 com handler (`Crash_2026-09-16_045310418`, `Created #216`)
  carregou DLL com `SizeOfImage 741376` — a do fix tem 611840. `SizeOfImage`
  não mente: era OUTRA build (link incremental com `.obj` velhos).
- Ação: `build/intermediates/Release_x64` apagado, rebuild limpo full,
  sem IPDB reaproveitado (commit `f2606b8`). Tail da morte válido como
  sintoma (kill melee), não como validação.
- Sessão 02:00: `injector_state.txt` correto = DLL do rebuild. Init limpo.
- Pendente: 10 kills multidão 40+ com telemetria no log.

## Item 0 SizeOfImage fantasma — causa raiz: git corrompia a DLL

- Crash logs 01:53 e 02:00 reportavam `size: 741376`; disco = 611840.
- Prova: blob do git maior que o arquivo, com `core.autocrlf=true` e sem
  `.gitattributes` o git aplicava conversão CRLF no binário: `SizeOfImage`
  611840 virava 741376 no módulo carregado.
- Fix (commit `63027ef`): DLL e injector.exe REMOVIDOS do git
  (`git rm --cached`) + `.gitignore` com `build/ *.obj *.ipdb *.tlog
  *.dll.old *.iobj *.pdb`. Fingerprint passa a ser SHA256 +
  `injector_state.txt` (size-mtime), nunca git.
- Injetor agora loga PATH COMPLETO + `SizeOfImage` lido do PE por inject.
- `SceneAlive` real: além do loader, checa `MainCamera.instance` +
  `cam+32 != null` + lista de zumbis com size 0..512. Transição (kill =
  Destroy, loading = troca de cena) = worker dorme 500ms.
- Terceiro modo mapeado: hang-kill, AV-kill, AV-loading — comum: invoke sem
  vivacidade em transição.

## Crash 02:17 modo novo + gate geração de mapa (16/09)

- Stack managed NOVO: objetos LOD → `GenerateBuildingFurniture` →
  `GenerateHashCellBuildings` → `UpdateLOD` → AV `0xc0000005`. `Created #224` →
  `Crash!!!` com câmera no céu (loading).
- Quarto modo: hang-kill, AV-kill, AV-loading, AV-geração-de-mapa. Comum:
  jogo destrói/recria objetos enquanto a worker invoca.
- `SizeOfImage 741376` persiste com DLL 612352 no disco e `injector_state`
  correto. `SizeOfImage` do crash log é o mapeado (seções alinhadas), não o
  tamanho do arquivo.
- Fix (commit `066590f`): `MapSettling()` — lista de zumbis oscilando >= 8
  em 2 ciclos seguidos = mapa assentando: worker dorme 500ms, zero invoke.
- Pendente: 10 kills + tail.

## Crash no inject 02:29 (PID 18308) — hook no meio do loading

- Sintoma: crash ~20s após o inject, tela de loading (céu, `6:10`, sem zumbi).
- Log da DLL PARA em `[BUDGET] 51/200 entidades=9` — sem `MainThread concluída`,
  sem `Mono bind OK`. A MainThread morreu antes de terminar o init.
- Crash log: `Created #221` → `Crash!!!`, stack nativo SEM frame managed.
- Causa: `MainThread` fazia `kiero::init` + bind + WndProc assim que achava
  QUALQUER janela — incluindo splash/loading com o mapa gerando células.
  Hook no meio da remontagem do LOD = AV.
- Fix (commit `7356597`): MainThread exige 4 leituras iguais da janela
  (~200ms estável, ~10s max); sem janela, aborta com log em vez de hookar
  no escuro.
- REGRA NOVA: injetar SOMENTE em partida (nunca no loading/menu).

## Auditoria hang 02:44 (86 zumbis) — causa raiz tripla

Evidência: ciclo 1438ms + `Mono live zombies=86` + stack com `SetActive` +
print com ~40 boxes (metade fora da tela, atrás da câmera).

1. **Budget sem gate on-screen**: 96 ent/ciclo mesmo fora da viewport. Fix:
   centroide dos 8 cantos antes dos invokes (fora = skip).
2. **Kill-window skippa dano normal** (60→40→20): entidade em combate some por
   1 ciclo. Fix: só skippa se MORREU (`!alive3 || hp3 <= 0`).
- Fix (commit `2471ffb`).

## Hang 03:05 (multidão) — causa: ciclo de 1438ms

- Telemetria: ciclo 1438ms com 86 zumbis. O ritmo adaptativo só ia até 66ms —
  sem freio pro ciclo de 1.4s.
- Item 0 FECHADO: `SizeOfImage 0xB5000` lido do PE no disco = 741376, igual
  ao crash log. A regra "injetar em partida" continua (hook no loading = AV).
- Fix (commit `6ffb88a`): teto 200ms no ritmo adaptativo.

## Crash 03:15 no tiro em multidão — stack novo (16/09)

- Stack managed NOVO (7º dump, outro modo): `PlayerArms:ShootGun` →
  `PhysicalGun:Shoot` → `ShootSingleProjectile` → `PlayerShot` →
  `OnPlayerShot` → AV (mesma região LOD, outro gatilho). O TIRO liga objetos
  em massa; nossos invokes testavam os mesmos objetos na mesma janela.
- Telemetria: ciclo 1438ms. Invokes sob carga do tiro = janela de corrida.
- Fix (commit `727e105`): budget 96→64 invokes/ciclo.
- Item 0 segue fechado: crash log `size 741376` = `SizeOfImage 0xB5000` do PE.

## Hang 03:24 sem crash handler (21 zumbis)

- Telemetria limpa (0,1% fail), `zombies=21-26`, sem pasta nova = hang puro
  (Present travou, worker parou), não AV. Com 21 zumbis o volume não explica.

## Crash 03:33 no tiro parado (16/09)

- Stack: `ShootGun→Shoot→ShootSingleProjectile→PlayerShot→OnPlayerShot` → AV
  (8º dump, mesma região LOD). Com 3-5 zumbis o volume não explica: o gatilho
  é o TIRO ligando objetos na mesma janela dos invokes.
- Fix (commit `aa68d8b`).

## Hang 03:47 sem crash handler — deadlock em lock

- Telemetria limpa (0,03%), `zombies=6`, 50 threads em Wait, delta CPU
  0,17s/5s. Hang puro com jogo leve = deadlock, não overload, não AV.
- Auditoria de locks: `Enter` bloqueante em `BuildEsp` (virada double-buffer),
  em `Sample` (contadores) e `GetEsp` antigo. Se o Present suspender
  (resize/foco/GPU), a worker trava dentro do CS = tela congelada com ESP.
- Fix (commit `2a0146a`): `TryEnter` em TODA espera de lock (virada pula pro
  próximo ciclo); contadores via `InterlockedIncrement` (sem CS).
- CE no hang: bridge vivo, 52 threads, `ASM_COUNT=142`, `CLASSES=1358`.

## Crash 03:59 no tiro (16 zumbis) — Present invocava

- Stack NOVO (9º dump): tiro → AV. Com 16 zumbis o volume não explica: o
  gatilho é o TIRO ligando objetos na mesma janela em que o PRESENT invocava
  (`Tick` no `hkPresent`: `InvokeBool hasLocal` + 19x go+name do `AuditBones`).
- Telemetria limpa (0,7%). O crash não veio da worker.
- Fix (commit `87cd69c`): `Tick` no Present virou bind+init (zero invoke);
  `ReadAll`/`AuditBones` migraram pra worker 1x/2s. Present agora: copia
  snapshot + desenha, zero invoke.
- Padrão UC aplicado: Present/render thread nunca toca em API do jogo;
  tudo que invoca roda em worker dedicada com orçamento.

## Crash 04:20 com 21 zumbis (16/09)

- Telemetria limpa (0,07%), sem pasta nova = hang, não AV. Com 21 zumbis e
  telemetria limpa, o volume não explica.
- Stack do dump: SEM frame managed, offset `0x19f43c7`. Sem managed no stack,
  o AV veio de fora do Mono: candidato é o próprio tiro do player + LOD.
- Fix (commit `b51af9d`): 1 invoke por entidade. Custo cai 5x.
- Circuit breaker: invoke com dt médio alto = motor saturado = pula o ponto.

## Anomalias para REVALIDAR (não corrigir neste ciclo)

- `stam=-20` fixo e `eyeY/footY` congelados no `Mono live` — cheiro de campo
  com offset defasado, mas não trava nada; revalidar via CE MCP (`readFloat`
  nos offsets `OFFSETS.md`) antes de mexer.
- `ext=(0,0,0)` no caminho 2D — **esperado** (`mono.cpp` zera de propósito).
- `gui.cpp` diff só-BOM + `nSeg` (cosmético, sem efeito em freeze).
