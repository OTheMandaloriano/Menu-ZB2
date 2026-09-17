# VISUAL-PRO-SPEC.md — Reorganização profissional da aba VISUAL (ZB2 Menu)

> Padrão aplicado: EO (ESP Range Limit, Box 2D/Cornered/3D, thickness,
> Item/Player/Grenade Line) + Gamesense/Skeet (Player ESP: teammates, chams,
> bounding box, health, name, weapon, distance; Other ESP: radar, dropped
> weapons) + PhantomOverlay/Warzone (Filled box, Show team, Max distance, Loot
> ESP separado) + Battlelog (Radar, Removals, Warnings) + UC (Skeleton ESP
> Preview separado, thread-safety com array fixo).
> Regras herdadas: 1 função por ciclo, commit pt-BR Objetivo/Origem/Testes/
> Histórico, build Release|x64, `injector_state.txt` + tail por teste, sem stub,
> sem TODO, sem chute de offset. Nada de aimbot/weapon/movement/magnet/
> teleport/host — SOMENTE VISUAL.
> REMOVIDO 17/09: Visible Check + Somente visíveis.
> Motivo: 1 cor por elemento; sem teste de oclusão o par de cores não tem
> sentido. ESP sempre na cor do elemento.

## 1. Layout final da aba (espelho do menu — implementar 1:1)

```markdown
## ■ ZUMBIS                               [x] Enable ESP (bZombieEsp — existe)
Box       [x]  Tipo [Corners v]  [cor ##]              (iZombieBox — existe)
Nome      [x]                    [cor ##]              (bZombieName — existe)
Distância [x]                    [cor ##]              (bZombieDist — existe)
Vida      [x]  Barra [x]  % [x]  [cor ##]              (bZombieHp/Pct — existe)
Skeleton  [x]                    [cor ##]              (bZombieSkeleton — existe)
Linha     [x]  [Base v]          [cor ##]              (bZombieSnap — existe)
HeadDot   [x]                    [cor ##]              (bZombieHeadDot — existe)
Classe    [x]                    [cor ##]              (NOVA bZombieClass)
────────────────────────────────────────────────
Dist máx  [==================== 60m]                  (fMaxDistance — espelhar PLAYER)

## ■ ALIADOS (azuis, sempre visíveis)     [x] Enable (bAllyEsp — vira master)
Box       [x]  Tipo [2D v]       [cor ##]              (iAllyBox — existe)
Nome      [x]                    [cor ##]              (bAllyName — existe)
Dist      [x]                    [cor ##]              (bAllyDist — existe)
Vida      [x]                    [cor ##]              (bAllyHp — existe)
Skeleton  [ ]                    [cor ##]              (bAllySkeleton — sem controle hoje)
Linha     [ ]  [Base v]          [cor ##]              (bAllySnap — sem controle hoje)
HeadDot   [ ]                    [cor ##]              (bAllyHeadDot — sem controle hoje)

## ■ CHAMS                                (sem master — 1 item só)
Corpo     [x]  [Visível ##][Oculto ##]                (bChams/colChamsVis/Inv — existe)

## ■ ITENS                                [x] Enable (bItemEsp — vira master)
Estilo  [Nome+Dist+Linha v]                           (NOVO iItemStyle)
Box     [x]  Tipo [Mini 2D v]                         (NOVO bItemBox/iItemBox)
Armas        [x]  [cor ##]   Raros        [x]  [cor ##] (bItemWeapons/Rare + colItem*)
Munição      [ ]  [cor ##]   Suprimento   [x]  [cor ##] (bItemAmmo/Supply + colItem*)
Raio  [================ 150m]                         (fItemRadius — existe)

## ■ PONTOS / MUNDO                       [x] Enable (bPoiEsp — vira master)
Helicóptero   [x]  [cor ##]   (sempre)                (NOVO bPoiHeli/colPoiHeli)
Chefão        [x]  [cor ##]   (sempre)                (NOVO bPoiBoss/colPoiBoss)
Missão        [x]  [cor ##]   (sempre)                (NOVO bPoiMission/colPoiMission)
Onda/Wave     [x]  [cor ##]   (sempre)                (NOVO bPoiWave/colPoiWave)
Loot fixo     [x]  [cor ##]   (no raio)               (NOVO bPoiLootFix/colPoiLootFix)
Bancadas      [x]  [cor ##]   (no raio)               (NOVO bPoiBench/colPoiBench)
Fogueira      [x]  [cor ##]   (no raio)               (NOVO bPoiFire/colPoiFire)
Mercador      [x]  [cor ##]   (no raio)               (NOVO bPoiShop/colPoiShop)
Respawn       [x]  [cor ##]   (sempre)                (NOVO bPoiRespawn/colPoiRespawn)
Raio  [================ 150m]                         (fItemRadius — mesmo dos itens)

## ▸ Avançado (colapsado — editor atual, sem salvar)
```

## 2. Variáveis novas (únicas — resto já existe em `config.h`)

```cpp
// Zumbi — classe prevê loot (ZombieIdentity.type+20, get_IsBoss)
// Itens — estilo + box mini + 5 cores (colItem atual = Armas, compatível)
extern int   iItemStyle;          // 0=Completo 1=só Nome 2=só Linha 3=Nome+Dist
extern bool  bItemBox;            // default true
extern int   iItemBox;            // 0=Mini 2D 1=Mini Corners
extern float colItemRare[4];      // default roxo  (0.7,0.2,1,1)
extern float colItemAmmo[4];      // default cinza (0.7,0.7,0.7,1)
extern float colItemSupply[4];    // default verde (0.2,1,0.4,1)
extern float colItemPoi[4];       // fallback POI sem cor própria
// POI — 9 filtros + 9 cores (sempre = dinâmico; no raio = estático)
extern bool  bPoiHeli, bPoiBoss, bPoiMission, bPoiWave, bPoiLootFix,
             bPoiBench, bPoiFire, bPoiShop, bPoiRespawn;   // default true
extern float colPoiHeli[4], colPoiBoss[4], colPoiMission[4], colPoiWave[4],
             colPoiLootFix[4], colPoiBench[4], colPoiFire[4], colPoiShop[4],
             colPoiRespawn[4];
// Linha — origem configurável (destino fixo no pé; item sempre Base→item)
extern int   iSnapFrom;           // 0=Base 1=Topo 2=Centro (zumbi + aliado)
```

JSON: adicionar as chaves com default acima; `LoadConfig` com `Has()`/fallback
para presets antigos (chave ausente = default, nunca reset). `iCfgVer` 4→5.

## 3. Regras de render (lógica — 1 parágrafo por regra)

1. **1 linha = 1 checkbox + 1 cor.** Some dropdown "Elemento" e "Exibir elemento".
   Cada função tem a própria linha (padrão CS2_External `Checkbox+SameLine+ColorEdit4`).
2. **1 cor por elemento, sempre.** Sem par de cores (removido 17/09):
   elemento desenha com `sua_cor`, ponto final.
3. **Par global só no XQZ do Chams** (`colChamsVis/Inv`).
4. **Aliados sempre, 1 cor.** (decisão de design PvE coop).
5. **Dist máx (zumbi):** cull ANTES de qualquer invoke (`dist > fMaxDistance` =
   skip entidade). Chefão ignora (`get_IsBoss` = sempre).
6. **Classe (zumbi):** texto `ZombieIdentity.type+20` (Comum/Riot/Queen/Reaper),
   mesma linha do Nome. Zero invoke novo. Offset via auditoria + CE MCP,
   nunca chutar.
6. **Linha origem:** `Base=(w/2,h)` / `Topo=(w/2,0)` / `Centro=(cx,cy)` da tela;
   destino fixo no pé (player/aliado) ou no item. `iSnapFrom`, zero invoke novo.
7. **Itens:** `Estilo` master (Completo/Nome/Linha); `Box` Mini 2D proporcional
   ao pickup (sem skeleton/headdot); Nome+Dist em par. Nome+Dist+Linha por
   categoria com sua cor, dentro de `fItemRadius`.
8. **POI:** dinâmico (heli/chefão/missão/onda/respawn) sempre; estático (loot
    fixo/bancadas/fogueira/mercador) dentro de `fItemRadius`. Heli mostra
    estado, Onda mostra direção, Chefão mostra estágio de vida. Classes em
    `CLASSES_UTEIS.md`; instâncias via CE MCP em partida.
9. **Chams sem master:** 1 item só, o próprio checkbox é o enable (XQZ pode
    ficar ligado com ESP desligado).
10. **Sem Salvar/Cancelar:** tudo escreve direto na var + JSON silencioso
    (debounce ~500ms, fora do frame). Some `s_editor.dirty`, `SaveLayoutDraft`,
    botões. Preset = combo que aplica na hora. SETTINGS mantém Salvar/Carregar
    como backup/restore.
11. **Worker = render, mesma flag:** `CollectJoints` lê a MESMA var do menu
    (mata `Config::bZombieSkeleton` × `layout.skeleton`). Gate on-screen antes
    de qualquer invoke (fora da viewport = skip, mantém cor).
12. **`ReadAll`/`AuditBones` fora do `hkPresent`.** Present só copia snapshot +
    desenha. `[BONE]/[JOINT]` 1x/sessão. Log quente agregado 1x/5s.

## 4. Ordem de implementação (1 função = 1 ciclo = 1 commit pt-BR)

1. Masters (só `if` no topo de cada bloco: Itens, Chams-render-mínimo, Aliados).
2. Toggles por elemento (bindar `bAllySkeleton/Snap/HeadDot`, `bZombieClass`).
3. Espelhar `fMaxDistance` na VISUAL (mesma var).
4. Unificar flags worker=render + gate on-screen.
5. `iSnapFrom` (origem da linha) + `iItemStyle`/`bItemBox`/`iItemBox` + 5 cores item.
6. `Tipo/Classe` texto + 9 POIs (Chefão+Onda → Heli+Missão → estáticos).
7. Sem-salvar (JSON silencioso) + `.gitignore` + injetor com PATH + SizeOfImage.
8. CE MCP por item novo + link UC pra padrão (nunca pra offset) no AUDIT-LOG.

## 5. Aceite

- 10 kills em multidão 40+ com tudo ligado, sem hang + `DETACH` limpo.
- `[PI-CALL]` fail <1% por site em 10min. p95 `[PERF]` <15ms com 60+.
- Toggle isola em 4 cliques: B (só Box+texto) → C (+Skeleton) → D (+Linha/Dot).
- `SizeOfImage` do crash log = disco (item 0 — sem isso, rodada não conta).
