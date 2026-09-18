---
name: zb2-dnspy-auto
description: Use ONLY when you need static data from the ZB2 game code (class fields, method IL, enum values like InventoryItem/ID) and the answer is not in memory/OFFSETS.md. Drives dnlib via dumpasm automatically, never asks the operator to click dnSpy GUI.
---

# dnSpy automatico (dnlib via dumpasm, sem GUI)

O dnSpy GUI nao e operavel por agente. Esta skill usa o motor dele
(dnlib.dll) via linha de comando. Nunca peca ao operador para clicar
no dnSpy.

## Ferramentas (ja prontas)

- `C:\Tools\dnSpy\bin\dnlib.dll` — engine.
- `dumpasm` (projeto dotnet):
  `C:\Users\WeFagundes\AppData\Local\Temp\opencode\dumpasm\dumpasm.csproj`
- Binario:
  `C:\Users\WeFagundes\AppData\Local\Temp\opencode\dumpasm\bin\Release\net10.0\dumpasm.dll`
- Assembly do jogo:
  `D:\Program Files (x86)\Steam\steamapps\common\Zumbi Blocks 2 Open Alpha\ZumbiBlocks2_Data\Managed\Assembly-CSharp.dll`
  (SHA `C41A2989…`; se o jogo atualizar, revalida o hash antes).

## Uso

```powershell
dotnet $dumpdll $asm <FiltroTipo> <saida.txt>
```

- `<FiltroTipo>` = substring do nome do tipo (ex. `PlayerArms`,
  `InventoryItem`, `Currency`, `Vendor`).
- Saida: `### TYPE` + `FIELD nome : Tipo` + `METHOD nome + assinatura`.
- IL e impresso so p/ metodos na lista do `Program.cs`
  (adiciona o nome la + rebuild se precisar de outro metodo).

## Adicionar IL de metodo novo

1. Edita `dumpasm/Program.cs` (condicao do `m.Name == ...`).
2. `dotnet build dumpasm.csproj -c Release -v q --nologo`.
3. Roda de novo com o filtro do tipo dono.

## Descobertas ja extraidas (nao re-extrair)

- `InventoryItem`: `id/pos/rotated/stackCount/ammo`; regra
  `stackMax==1 -> ammo, senao stackCount` (IL Get/SetGenericNumericValue).
- `DatabaseGun`: `ammoID/maxAmmo/ammoConsumption/gunClass/...`.
- `DatabaseItem`: `stackMax/itemID/tier/...`.
- `InventoryItem/ID` (TypeDef 1152): 117 valores (ex. 76 ArmoryKey,
  55 Grenade, 90 Dynamite, 25 Bandage, 20 SodaCan, 10-13 balas,
  58 Wood, 59/60 sucatas). Lista em `Temp/opencode/itemids.txt`.
- `ZombieType` (0-8): 0-4 tiers, 5 factory, 6 BossRiot, 7 BossQueen,
  8 BossReaper.
- `PlayerArms.ShootGun`: `ammo -= ammoConsumption`, clamp 0.
- `ReloadGun`: `need=maxAmmo-ammo; pulled=PullStoredItems(ammoID,need)`.
- `AddItem`: sem lugar -> `DropLoot` + retorna false.
- `LootPlacingFilter` (byte): 0 Inventory, 1 Equipment, 2 Both.

## Proibido

- Pedir ao operador para abrir/clicar no dnSpy GUI.
- Re-dumpar o que ja esta acima (le o .txt em Temp/opencode).
- Chutar offset a partir do metadata (layout Mono e runtime;
  offset real = `mono_field_get_offset` via DLL).
