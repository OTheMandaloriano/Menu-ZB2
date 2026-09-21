# OFFSETS.md - Registro oficial de offsets validados (ZB2 / Unity 6 Mono x64)

> Origem: dnSpy-conceitual via CE Mono Dissector (mono_image_enumClasses) + validacao
> runtime via CE MCP Bridge v12 (mono_class_findInstancesOfClassListOnly + readFloat).
> Jogo: ZumbiBlocks2.exe SHA-256 66c3ed6829349aac8b5cb5fdb2a85ef62d17c1bded4334352af7349ca608ea0a
> Assembly-CSharp.dll SHA-256 c41a298975d35f0dad0a05531bce6e0b6e274d0ddf265217d65ce3ac5cbc84e1
> Data: 2026-09-12 ~00:05 BRT. Partida ao vivo (2x PlayerMain, 25x Zombie).
>
> ## Tabela de limites por arma (validada viva 21/09 via CE MCP, ItemsBase 116 slots)
>
> Todas as 61 armas: `stackMax da arma = 1` (pente em `ammo`), consumo 1/tiro
> (excecao: DoubleSgx1911 = 2). Balas: 9/10/11/12/64 = pilha 200; 107 He40mm = 20.
> Reserva = soma `stackCount` no storage (sem teto por tipo; teto = grid 16x20).
> Recarga puxa `Min(need, reserva)` via `PullStoredItems` (só storage).
>
> | ID | Arma | maxAmmo | ammoID | pellet | fullAuto | rof | spread | recoil |
> |---|---|---|---|---|---|---|---|---|
> | 1 | HiPoint | 9 | 12 | 1 | 0 | 8.0 | 4.0 | 1.50,0.80 |
> | 50 | RiotShotgun | 20 | 64 | 10 | 0 | 2.0 | 30.0 | 1.00,3.00 |
> | 46 | Barrett | 10 | 10 | 1 | 0 | 4.0 | 1.0 | 1.20,2.00 |
> | 68 | Spas | 9 | 9 | 10 | 0 | 5.0 | 20.0 | 1.00,1.50 |
> | 105 | Negev | 150 | 11 | 1 | 1 | 11.0 | 15.0 | 0.40,0.30 |
> | 47 | Ultimax100 | 100 | 11 | 1 | — | — | — | — |
> | 103 | Am640Launcher | 1 | 107 | 1 | 0 | — | — | — (explode: ShootThrowable) |
> | 94 | DoubleSgx1911 | 36 | 12 | 2 | — | — | — | consumo 2/tiro |
>
> Tabela completa das 61 armas no dump da sessao 21/09.
> Armas brancas: Shovel, Pickaxe, Pipe, WoodenClub, KnifeAk47, Cleaver, PipeWrench, Katana, Kukri, Nodachi, QueensHammer, ReaperScythe, Shuriken, ThrowingAxe.
> Regra: nome do campo (FieldOff em runtime) > numero fixo. Numero acima e
> cache desta build (SHA c41a2989…) — revalidar se o SHA mudar.

## Validados em runtime [OK]

| Classe.campo | Offset | Tipo | Como validado (MCP) | Valor vivo | Status |
|---|---|---|---|---|---|
| PlayerMain.healthFast | +204 (0xCC) | float | evaluate_lua readFloat(obj+204) | PM1=100.0, PM2=50.0 | OK |
| PlayerMain.healthSlow | +208 (0xD0) | float | evaluate_lua readFloat(obj+208) | 100.0 | OK |
| PlayerMain.maxStamina | +224 (0xE0) | float | read_integer float obj+224 | 100.0 | OK |
| PlayerMain.staminaFast | +228 (0xE4) | float | evaluate_lua readFloat(obj+228) | PM1~95.9, PM2=100.0 | OK |
| PlayerMain.staminaSlow | +232 (0xE8) | float | dump PlayerMain | - | OK-struct |
| PlayerMain.movement | +40 (0x28) | PlayerMovement* | read_memory obj+40 ptr valido | ptr ok | OK |
| PlayerMain.inventory | +64 (0x40) | PlayerInventory* | read_memory obj+64 ptr valido | ptr ok | OK |
| PlayerMain.noClip | +104 (0x68) | NoClip* | dump PlayerMain | - | OK-struct |
| PlayerMain.posSync | +144 (0x90) | PlayerPositionSynchronizer* | dump | - | OK-struct |
| PlayerMain.get_HasLocalControl | metodo | bool | mono_invoke: PM1=1 PM2=0 | local=PM1 | OK |
| Zombie.health | +168 (0xA8) | ZombieHealth* | readPointer(z+168)->obj | ptr ok | OK |
| ZombieHealth.amount (HP atual) | +32 (0x20) | float | readFloat(h+32): 80.0 vivos, 0.0 morto | 80/80, 0/0 | OK |
| ZombieHealth.max | +16 (0x10) | float | readFloat(h+16) | 80.0 | OK |
| ZombieHealth.isAlive | +28 (0x1C) | bool | dump ZombieHealth | - | OK-struct |
| Zombie.ai | +144 (0x90) | ZombieAI* | dump Zombie | - | OK-struct |
| Zombie.obj | +16 (0x10) | ZombieObject* | dump Zombie | - | OK-struct |
| Zombie.identity | +192 (0xC0) | ZombieIdentity | dump Zombie | - | OK-struct |
| Zombie.state | +220 (0xDC) | ZombieState | dump Zombie | - | OK-struct |
| ZombieObject.meshRenderer | +56 (0x38) | SkinnedMeshRenderer* (CORPO p/ chams) | dump ZombieObject | - | OK-struct |
| ZombieObject.armatureBone | +72 (0x48) | Transform[] (skeleton) | dump ZombieObject | - | OK-struct |
| armature[0] | - | null | [BONE] 14/09 | - | OK |
| armature[1]=hl hipL | - | Transform | [BONE] 14/09 | - | OK |
| armature[2,3,4] | - | l1l thighL, l2l shinL, fl footL | [BONE] 14/09 | - | OK |
| armature[5,6,7] | - | l1r thighR, l2r shinR, fr footR | [BONE] 14/09 | - | OK |
| armature[8,9,10] | - | sp1,sp2,sp3 spine | [BONE] 14/09 | - | OK |
| armature[11,12] | - | neck, head | [BONE] 14/09 | - | OK |
| armature[13,14,15] | - | sl shoulderL, a1l armL, a2l foreL | [BONE] 14/09 | - | OK |
| armature[16,17,18] | - | sr shoulderR, a1r armR, a2r foreR | [BONE] 14/09 | - | OK |
| ZombieObject.zombieEyeRef | +88 (0x58) | Transform (head) | dump ZombieObject | - | OK-struct |
| ZombieObject.zombieFootRef | +96 (0x60) | Transform (feet) | dump ZombieObject | - | OK-struct |
| PlayerMovement.walkSpeed | +252 (0xFC) | float | dump PlayerMovement | - | OK-struct |
| PlayerMovement.jumpSpeed | +264 (0x108) | float | dump PlayerMovement | - | OK-struct |
| PlayerMovement.rollSpeed | +272 (0x110) | float | dump PlayerMovement | - | OK-struct |
| NoClip.speed | +40 (0x28) | float | dump NoClip | - | OK-struct |
| DaytimeController.curTime | +172 (0xAC) | float | readFloat=9.68h ao vivo | 9.68 | OK |
| DaytimeController.dayDurationInMinutes | +72 (0x48) | float | dump DaytimeController | - | OK-struct |
| MainCamera.cam | +32 (0x20) | Camera* (W2S) | dump MainCamera | - | OK-struct |
| FOVController.UserDefinedFOV | +4 [static] | float | dump FOVController | - | OK-struct |
| PlayersController.players | +48 (0x30) | List<PlayerMain> | dump PlayersController | - | OK-struct |
| ZombieLoader.zombies | +88 (0x58) | List<Zombie> (ESP loop) | dump ZombieLoader | - | OK-struct |
| Explosion.rangeMeters | +20 (0x14) | float (HOST) | dump Explosion | - | OK-struct |
| Explosion.dmgMin/dmgMax | +24/+28 | float | dump Explosion | - | OK-struct |
| PlayerInventory.AddItem | metodo | giveaway itens | dump PlayerInventory | - | OK-struct |

## Cadeias resolvidas via FieldOff (runtime, nome do campo — sem hardcode)

> Implementado em `mono.cpp` (offsets via `mono_field_get_offset`, nunca numero fixo).
> Evidencia estatica: `dump/sessao-2026-09-19_12-00.txt` (dnlib). Validacao viva quando citada.

| Cadeia (nomes) | Uso | Status |
|---|---|---|
| PlayerMain.inventory → PlayerInventory.equippedItems → GetEquipment(selectedItem) → InventoryItem.ammo | Municao infinita (pente) | OK-uso |
| InventoryItem.GetDataBaseItem() → DatabaseGun.maxAmmo / ammoConsumption / ammoID | Teto do pente + tipo da reserva | OK-uso |
| PlayerInventory.storage → ItemContainer.items[] → InventoryItem.stackCount / id + DatabaseItem.stackMax | Pilhas no teto (reserva) | OK-uso |
| Currency.Instance → Dollar/Silver/Gold → CurrencyData.amount (=99999) | Dinheiro infinito | OK-uso |
| LoadoutSelector.instance → UnlockAll() | Desbloquear slots (1x) | OK-uso |
| InventoryItem.CreateInventoryItem(ID,int) + PlayerInventory.AddItem(item,filter) | Spawn (REMOVIDO 17/09; IDs no historico) | REMOVIDO |
| PlayerArms.TryStartReload | Recarga legitima (coop, pente zera) | OK-uso |
| ZombieIdentity.type+20 + get_IsBoss | Boss real (Assalto/Rainha/Ceifador), cor propria | OK-uso |

## PENDENTE (extraido do IL 19/09, sem validacao viva — nao usar em WriteF ainda)

| Campo (nome exato) | Funcao | Cadeia | Como validar (MCP) |
|---|---|---|---|
| DatabaseGun.recoil (Vector2) + recoilRandomness | No Recoil | PhysicalGun.DbReference → DatabaseGun | zerar na arma equipada, atirar, mira nao sobe |
| DatabaseGun.recoilSpeedMultiplier × rof | No Recoil (velocidade) | idem | idem |
| DatabaseGun.spread × WeaponBase.precisionMultiplier | No Spread | DbReference + WeaponBase.instance | spread=0 agrupa tiro num ponto |
| DatabaseGun.choke (enum) via WeaponBase.GetChoke() | Spread escopeta | idem | so escopeta |
| DebugGeneralModifiers.General.DisableSway.value (=true) | No Sway | singleton DebugModifiers.General | mirar ADS, flutuacao some |
| WeaponBase.gunSway | No Sway (global alternativo) | WeaponBase.instance | idem |
| PlayerCamera.swayTimer (congelar) | No Sway (alternativo) | PlayerMain.cam → PlayerCamera | idem |
| DatabaseGun.rof (BaseCooldownTime=1/rof) | Rapid Fire | DbReference da equipada | subir rof, rajada acelera |
| PhysicalGun.Cooldown (ResetCooldown grava -0.001) | Rapid Fire (alternativo) | PlayerArms → EquippedGun | forcar por ciclo |
| PlayerMeleeAttack.Duration (+SpeedCurve/MovementSpeed) | Fast Knife | arma equipada → MoveSet → nodes[] → attack | reduzir Duration do KnifeStab |
| MeleeMoveSetNode.transitionTimeMinimum+Maximum | Fast Knife (transicao) | idem | idem |
| PlayerMovement.jumpSpeed | Super Pulo | PlayerMain.movement+40 → jumpSpeed | subir valor, pular |
| PlayerMain.arms / PlayerMain.cam | Base das cadeias acima | FieldOff no local | ler ponteiro != null |
| WeaponBase.instance / DebugModifiers.General | Singletons das cadeias | FieldOff static | ler != null |
| PlayerHUD.instance → innerCrossHairTransform → localScale | Mira Fechada (visual) | FieldOff + singleton via vtable | escala trava na metade da base |
| MeleeAttackBase.Instance → AllAttacks (dict ID→attack) | Fast Knife global (toda arma branca) | FieldOff + singleton | Duration de todos / mult |
| PlayerMovement.fallDamageThreshold | Super Pulo (anti-podador LimitVerticalVelocity) | FieldOff, acompanha mult | threshold × mult |
| DatabaseGun.fullAuto (bool) = true | Rapid Fire: 1-tiro vira automatica (pistola 12/12, Riot 1/1) | FieldOff, asset compartilhado | segura gatilho = rajada |
| DatabaseGun.burstCount = 0 | Rapid Fire: rajada vira auto continuo | FieldOff | sem pausa de burst |
| PlayerArms.selectedItem (EquipmentIndex struct) | arma equipada real | arms+sel: SetType@+0, Value@+4 (rel. campo; +16/+20 absolutos no CE) | SetType==1 → weapons[Value] |
| PlayerEquippedItems.weapons / misc | listas de equipados | FieldOff | WalkList |
| PlayerArms.TryStartReload | recarga legitima (coop) | metodo, invoke 1x | pente 0 → recarrega |
| DatabaseGun.ammoID / maxAmmo | tipo + teto do pente | FieldOff no db da equipada | ammoID 10-116 |

## Alternativas conhecidas (rejeitadas ou secundarias)

| Duvida | Alternativas | Decisao |
|---|---|---|
| HP real | healthFast+204 (usado no dano) vs healthSlow+208 (regen/lento) | God trava os dois; leitura usa Fast |
| Stamina real | staminaFast+228 vs staminaSlow+232; maxStamina+224 | trava Fast no Max; Slow p/ sprint (GetSprintSpeed usa staminaSlow) |
| Pente vs reserva | InventoryItem.ammo (pente) vs pilhas storage (reserva por ammoID) | coop: so reserva (host valida dano); solo: os dois |
| Sway | DisableSway.value (legitimo) vs gunSway=0 (global) vs swayTimer (congelado) | preferir DisableSway; resto fallback |
| Cooldown | rof alto vs ResetCooldown por ciclo | ResetCooldown e mais estavel (nao muda DPS nominal) |
| Tipo do zumbi | get_IsBoss (metodo) vs ZombieIdentity.type+20 (campo) | campo direto + fallback metodo |
| Spawn de itens | AddItem direto (bania/flood) vs doses de 30 + backoff | REMOVIDO 17/09; se voltar, doses |

VTable D3D11 (spec, nao do jogo): Present=8, ResizeBuffers=13.

## Ponteiros base (via Mono, sem AOB - Unity Mono usa reflection)
- LocalPlayer: PlayersController.instance -> players[] + get_HasLocalControl==1 (PM1=0x2232CEAED00 nesta sessao; re-resolver por partida via reflection, nunca hardcodar).
- Zumbis: ZombieLoader.Instance -> zombies List (25 vivos nesta sessao).
- NOTA: enderecos 0x223... sao heap da sessao - NAO commitar como offset fixo. Offsets de campo acima sao estaveis (layout Mono).
