using System;
using System.Runtime.CompilerServices;
using HarmonyLib;
using UnityEngine;
namespace Zb2Menu {
    public static class CombatExtrasBridge {
        static readonly OriginalValues originals=new OriginalValues();
        sealed class Launch {public Vector3 Velocity;public float Time;}
        static ConditionalWeakTable<ZombieObject,Launch> launches=new ConditionalWeakTable<ZombieObject,Launch>();
        static bool saitama;
        static float updated;
        static int lastRequest;
        static ItemsBase previousItems;
        static ExplosionController previousExplosions;
        static bool previousEnabled,previousContact;
        static float previousFuse,previousRadius,previousDamage;
        static bool ApplyChanged(bool enabled,bool contact,float fuse,float radius,float damage){
            bool changed=previousItems!=ItemsBase.instance || previousExplosions!=ExplosionController.instance || previousEnabled!=enabled || previousContact!=contact || previousFuse!=fuse || previousRadius!=radius || previousDamage!=damage;
            previousItems=ItemsBase.instance;previousExplosions=ExplosionController.instance;previousEnabled=enabled;previousContact=contact;previousFuse=fuse;previousRadius=radius;previousDamage=damage;
            return changed;
        }
        public static void Install(Harmony harmony) {
            harmony.Patch(AccessTools.Method(typeof(Damage),"ProcessDamage"),prefix:new HarmonyMethod(typeof(CombatExtrasBridge),"BeforeDamage"));
            harmony.Patch(AccessTools.Method(typeof(ZombieDoll),"CopyArmature"),postfix:new HarmonyMethod(typeof(CombatExtrasBridge),"LaunchDoll"));
        }
        static bool Host(){return MultiplayerController.instance!=null && MultiplayerController.instance.IsServer();}
        static float Limit(float v,float low,float high){return float.IsNaN(v)||float.IsInfinity(v)?low:Math.Max(low,Math.Min(high,v));}
        public static string Apply(int ready,int punch,int explosives,int contact,float fuse,float radius,float damage,int itemId,int amount,int request) {
            bool add=request!=lastRequest;lastRequest=request;
            var player=PlayersController.instance==null?null:PlayersController.instance.MyPlayer();
            bool allowed=ready!=0 && Host() && player!=null && player.HasLocalControl && player.healthFast>0;
            saitama=allowed && punch!=0;updated=Time.unscaledTime;
            UpdateExplosives(allowed && explosives!=0,contact,fuse,radius,damage);
            if(!allowed){if(punch!=0 || explosives!=0 || add)return "Extras: entre no mapa em solo/host";return "";}
            if(!add)return "";
            return Grant(player,itemId,amount);
        }
        static void UpdateExplosives(bool enabled,int contact,float fuse,float radius,float damage) {
            bool update=ApplyChanged(enabled,contact!=0,fuse,radius,damage);
            if(update){
            originals.Begin();
            try {
                if(enabled && ItemsBase.instance!=null){
                    for(int id=0;id<(int)ItemsBase.ItemCount;++id){
                        var item=ItemsBase.instance.GetItem((InventoryItem.ID)id) as DatabaseThrowable;
                        if(item==null || (item.explosionID!=Explosion.ID.Frag && item.explosionID!=Explosion.ID.Dynamite))continue;
                        originals.Apply(item,"timeOut",()=>item.timeOut,v=>item.timeOut=v,v=>Limit(fuse,.1f,10));
                        if(contact!=0)originals.Apply(item,"contactDestroy",()=>item.contactDestroy,v=>item.contactDestroy=v,v=>true);
                    }
                    var owner=ExplosionController.instance;
                    if(owner!=null)foreach(var kind in new[]{Explosion.ID.Frag,Explosion.ID.Dynamite}){
                        var exp=owner.GetExplosion(kind);if(exp==null)continue;
                        originals.Apply(exp,"rangeMeters",()=>exp.rangeMeters,v=>exp.rangeMeters=v,v=>Limit(radius,1,30));
                        float top=Limit(damage,10,10000);
                        float originalMax=originals.ReadOriginal(exp,"dmgMax",()=>exp.dmgMax);
                        originals.Apply(exp,"dmgMax",()=>exp.dmgMax,v=>exp.dmgMax=v,v=>top);
                        originals.Apply(exp,"dmgMin",()=>exp.dmgMin,v=>exp.dmgMin=v,v=>originalMax>0?v*top/originalMax:v);
                        originals.Apply(exp,"maxTotalDamage",()=>exp.maxTotalDamage,v=>exp.maxTotalDamage=v,v=>originalMax>0?v*top/originalMax:v);
                    }
                }
            }catch{previousItems=null;throw;}finally{originals.End();}
            }
        }
        static string Grant(PlayerMain player,int itemId,int amount) {
            if(ItemsBase.instance==null || player.inventory==null)return "Inventario indisponivel";
            if(itemId<=0 || itemId>=(int)ItemsBase.ItemCount)return "Selecione um item valido";
            var db=ItemsBase.instance.GetItem((InventoryItem.ID)itemId);
            if(!ItemEligibility.Allowed(db))return "Item bloqueado ou reservado";
            int count=Math.Max(1,Math.Min(999,amount));
            var loot=new InventoryItem((InventoryItem.ID)itemId);
            loot.SetGenericNumericValue(db.stackMax>1?Math.Min(count,db.stackMax):1);
            var place=player.inventory.FindPlaceFor(loot.id,loot.stackCount,LootPlacingFilter.Both);
            if(place==null)return "Inventario sem espaco: nada adicionado";
            player.inventory.PutLootIntoPosition(loot,place);
            return "Item adicionado; acao nao e revertida ao desligar";
        }
        static void BeforeDamage(IDamageTarget target,ref Damage damage,bool localCall){
            if(!saitama || !Host() || Time.unscaledTime-updated>.5f || !localCall || damage==null || damage.damageType!=DamageType.BluntMelee)return;
            var zombie=target as Zombie;var player=damage.zombieDamage.sourcePlayer;
            if(zombie==null || zombie.obj==null || zombie.health==null || !zombie.health.isAlive || player==null || !player.HasLocalControl)return;
            if(player.movement==null || player.MyAnimator==null)return;
            var attack=player.movement.CurrentMeleeAttack;
            if(attack==null || !IsPunch(attack.ID) || !player.MyAnimator.GetCurrentAnimatorStateInfo(0).IsName(attack.Animation.Name))return;
            damage=damage.Clone();damage.amount=4000000;
            MagnetFreeze.Release(zombie);
            var velocity=player.transform.forward*25+Vector3.up*8;
            launches.Remove(zombie.obj);launches.Add(zombie.obj,new Launch{Velocity=velocity,Time=Time.unscaledTime});
            var body=zombie.obj.body;if(body!=null && !body.isKinematic)body.linearVelocity=velocity;
        }
        static bool IsPunch(PlayerMeleeAttackID id){return id==PlayerMeleeAttackID.LeftJab || id==PlayerMeleeAttackID.RightJab || id==PlayerMeleeAttackID.RightOverhandPunch || id==PlayerMeleeAttackID.SpinningBackfistLeft;}
        static void LaunchDoll(ZombieDoll __instance,ZombieObject original){
            Launch launch;if(original==null || !launches.TryGetValue(original,out launch))return;
            launches.Remove(original);
            if(Time.unscaledTime-launch.Time>15)return;
            if(__instance.armaturePhysics==null)return;
            foreach(var body in __instance.armaturePhysics)if(body!=null && !body.isKinematic)body.linearVelocity=launch.Velocity;
        }
    }
}
