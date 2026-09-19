# CLASSES_UTEIS.md - Auditoria Assembly-CSharp.dll (CE Mono Dissector, 2026-09-12)

> 1358 classes enumeradas via mono_image_enumClasses. Abaixo as uteis p/ o cheat.
> Assembly SHA c41a298975d35f0dad0a05531bce6e0b6e274d0ddf265217d65ce3ac5cbc84e1

## Entidades
- Zombie (obj+16 ZombieObject, ai+144, health+168 ZombieHealth, identity+192, state+220)
  - Metodos-chave: TakeDamage, DirectDamage, TeleportTo, SetSpeed, SetTargetPlayer, get_IsBoss, get_Transform
- ZombieHealth (max+16 float, isAlive+28 bool, amount+32 float HP atual)
- ZombieAI (targeting+16, zombieObj+24, State+40) | ZombieTargeting, ZombieAIState
- ZombieObject : MonoBehaviour (body+32 Rigidbody, meshRenderer+56 SkinnedMeshRenderer CORPO, armatureBone+72 Transform[] SKELETON, zombieEyeRef+88 HEAD, zombieFootRef+96 FEET, zombie+112)
- ZombieIdentity (id+16 int, type+20 ZombieType, meshIndex/materialIndex)
- ZombieType, ZombieTypes, ZombieController, ZombieProperties, ZombieDamage, ZombiePositionSync, ZombieNetSyncs, ZombieLoader (Instance static, zombies+88 List<Zombie>, unloadedZombies+72, zombieHolder+40)
- BossBehaviour (zombie+16, healthStage+48) | RiotBossBehaviour/RiotBossObject, QueenBossBehaviour/QueenBossObject, ReaperBehaviour/ReaperBossObject
- AllPurposeZombie, UnloadedZombie, ZombieDoll, ZombieObject, CustomZombieObject

## Player (aliado = mesmo tipo, azul)
- PlayerMain : MonoBehaviour (movement+40, inventory+64, noClip+104 NoClip, reviveInteraction+120, posSync+144, entityLocation+168, healthFast+204, healthSlow+208, maxStamina+224, staminaFast+228, staminaSlow+232)
  - Metodos: TakeDamage, DirectDamage, Revive, RespawnAt, FillHealthAndStamina, get_HasLocalControl (1=local), get_ForeignPlayer
  - dnlib 19/09 (`dump/sessao-2026-09-19_12-00.txt`): confirma arms : PlayerArms, cam : PlayerCamera, SpawnedSkin : CharacterSkin
- PlayerMovement (body+48 Rigidbody, walkSpeed+252, jumpSpeed+264, rollSpeed+272, speedCoef+300, state+284)
- PlayerInventory (storage+40 ItemContainer, equippedItems+48, AddItem, SetEquipment, TryAddEquipment)
- PlayerEquippedItems, PlayerCamera (aim), PlayerMeleeAttack, PlayerStatusEffects, PlayerInteraction, PlayerReviveInteraction, PlayerPositionSynchronizer (targetTransform+32, lastPos+104, ReceivePosition/SendUpdate), PlayerInputReader
- PlayersController (instance static, players+48 List<PlayerMain>, MyPlayer(), GetPlayer)
- NoClip : MonoBehaviour (targetBody+32, speed+40, SwitchNoClip(), Update)

## Armas / Tiro (dnlib 19/09 — pendente validacao viva, ver OFFSETS.md)
- PlayerArms (playerMain, selectedItem : EquipmentIndex, hands : PlayerHandItems)
  - ShootGun(item): ammo -= ammoConsumption (clamp 0) → ShotPath → PhysicalGun.Shoot → PlayerCamera.Recoil(recoilSpeedMultiplier×rof, recoil, recoilRandomness)
  - GetGunSpread() = DatabaseGun.spread × WeaponBase.precisionMultiplier; GetDudChance() (melee); GetChokeMultiplier()
  - ReloadGun(qtd): need=maxAmmo-ammo; PullStoredItems(ammoID, need); TryStartReload/CanReload/GunNotFilled(ammo<maxAmmo)
- DatabaseGun (recoil : Vector2, recoilRandomness, recoilSpeedMultiplier, spread, rof, pelletCount, fullAuto, burstCount, burstSpace, choke, maxAmmo, ammoConsumption, ammoID, dmg, pierce, hsMultiplier)
- PhysicalGun (barrel, DbReference : DatabaseGun, Cooldown, BufferedShot, BufferedBurst, meleeRange; BaseCooldownTime=1/rof; ResetCooldown grava -0.001; ShouldFire)
  - ShootSingleProjectile: ShotPool → GetShotDirectionWithSpread (sorteia Random.Range(0,spread)) → SetupShotDamage → PlayerShot
- WeaponBase (singleton instance; precisionMultiplier, gunSway, noiseMultiplier, chokeMultiplier[], guns : DatabaseGun[])
- PlayerCamera (recoilMagnitude : Vector2, recoilSpeed, recoilTimer, swayTimer, verticalRecoilCurve, horizontalRecoilCurve, recoilMultiplier; Recoil(); RecoilAndSwayVector() so soma sway em ADS sem DisableSway)
- DebugGeneralModifiers (singletons DebugBoolean: GodMode, DisableSway, DisableAdsCameraMovement, CountReloadTime, DrawMeleeRaycasts...; DebugBoolean.value : Boolean)

## Melee / Faca rapida (dnlib 19/09 — pendente validacao viva)
- PhysicalMelee (MoveSet, reach, IsThrowable, ThrowableID, databaseReference : DatabaseMelee)
- PlayerMeleeAttack (Duration, AverageHitTime, SpeedCurve, MovementSpeed, StaminaDrain, HitStopStrength, Damage, RaycastSource/Direction)
  - Faca: KnifeStab, KnifeRestab, KnifeCrossStab, KnifeReverseStab (+50 IDs: Katana*, Kukri*, Cleaver*, 6 Reaper*)
- MeleeMoveSet (entryWeakAttack, entryStrongAttack, nodes : MeleeMoveSetNode[]; transitionTimeMinimum+Maximum por no)
- Executor: PlayerMovement.MeleeAttack(ID, MoveSet) / DealEntryMeleeAttack; PlayerArms.TryMeleeAttacks/GetMeleeReach

## Camera / W2S
- MainCamera (instance static, cam+32 UnityEngine.Camera, fovController+48)
- FOVController (UserDefinedFOV+4 static, CurrentFOV+64, cam+32)
- PlayerCamera, PlayerCameraOffset, CameraPivotHandling

## Itens / Loot (Item magnet, giveaway)
- DatabaseItem (itemID+76 InventoryItem.ID, stackMax+84), DatabaseGun/DatabasePrimaryGun/DatabaseSecondaryGun, DatabaseMelee, DatabaseThrowable, DatabaseConsumable, CraftingDatabase/CraftingRecipe
- InventoryItem (id, stackCount, ammo; regra IL: stackMax==1 → ammo, senao stackCount), ItemContainer (items), DroppedLoot, DroppedLootSack, LootController, LootSpawner, LootSpawnPoint, LootSackItemSet, LootClusterGroup
- PhysicalGun/PhysicalMelee/PhysicalThrowable/PhysicalConsumable
- Currency (singleton Instance; Dollar/Silver/Gold → CurrencyData.amount; AddCurrency(ID,int)); CurrencyID (Dollars=0?)
- LoadoutSelector (instance; UnlockAll())

## Combate / Dano (Saitama, weapon mods, explosion)
- Damage, PlayerDamage, ZombieDamage, IDamageTaker/IDamageTarget, DamageType
- Explosion (rangeMeters+20 HOST, dmgMin+24, dmgMax+28, maxTotalDamage+32), ExplosionController, ExplosionResult
- ThrowableController/ThrowableInstance, Projectile (Shot, ShotPool, ShotPath), HitDetector
- Stagger/StaggerPack/StaggerDatabase

## Mundo / Host controls
- DaytimeController (instance static, dayDurationInMinutes+72, curTime+172 float HORAS, PassTime, SyncDaytimeOnline)
- WavesController (instance static, WaveSpawner+40, StackZombies*, RestockZombie, SkipToBossWave, ProcessWavesServerSide=server)
- WaveSpawner (SpawnZombiesAt, StackZombies, maximumSpawnedZombies+60)
- MatchController, GameMode/WaveMode/Difficulty, Helicopter (+HelicopterState/Landing/Takeoff), InterestPoint/InterestPointController (POI ESP), MapGraph/MapConstructor (teleports), RespawnPoints
- ServerController/ClientController, ServerMatchmaking/ClientMatchmaking (host x client), NetMessageType, P2PConnection

## HUD (referencia de layout)
- PlayerHUD, EquipmentHUD, DmgIndicator, BossHealthBar, WaveBarDisplay, MainTimerHUD, LootNotifier

## Box 3D (auditoria complementar)
- ZombieObject.meshRenderer+56 = SkinnedMeshRenderer do corpo; Renderer.bounds (AABB de mundo) cobre corpo todo.
- Ossos: zombieEyeRef+88 (olhos), zombieFootRef+96 (tornozelo aprox.), armatureBone+72 Transform[] (skeleton, item 12).
- Box 3D usa AABB (center/extents, 8 cantos projetados); fallback eye/foot se bounds falhar.

