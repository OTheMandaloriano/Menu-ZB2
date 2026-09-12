# OFFSETS.md - Registro oficial de offsets validados (ZB2 / Unity 6 Mono x64)

> Origem: dnSpy-conceitual via CE Mono Dissector (mono_image_enumClasses) + validacao
> runtime via CE MCP Bridge v12 (mono_class_findInstancesOfClassListOnly + readFloat).
> Jogo: ZumbiBlocks2.exe SHA-256 66c3ed6829349aac8b5cb5fdb2a85ef62d17c1bded4334352af7349ca608ea0a
> Assembly-CSharp.dll SHA-256 c41a298975d35f0dad0a05531bce6e0b6e274d0ddf265217d65ce3ac5cbc84e1
> Data: 2026-09-12 ~00:05 BRT. Partida ao vivo (2x PlayerMain, 25x Zombie).

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

VTable D3D11 (spec, nao do jogo): Present=8, ResizeBuffers=13.

## Ponteiros base (via Mono, sem AOB - Unity Mono usa reflection)
- LocalPlayer: PlayersController.instance -> players[] + get_HasLocalControl==1 (PM1=0x2232CEAED00 nesta sessao; re-resolver por partida via reflection, nunca hardcodar).
- Zumbis: ZombieLoader.Instance -> zombies List (25 vivos nesta sessao).
- NOTA: enderecos 0x223... sao heap da sessao - NAO commitar como offset fixo. Offsets de campo acima sao estaveis (layout Mono).
