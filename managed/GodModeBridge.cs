using System;
using HarmonyLib;
using UnityEngine;

namespace Zb2Menu {
    public static class GodModeBridge {
        static PlayerMain owner;
        static float heartbeat;
        public static void Install(Harmony harmony) {
            var method=AccessTools.Method(typeof(PlayerMain),"TakeDamage",new[]{typeof(Damage)});
            if(method==null)throw new MissingMethodException("PlayerMain.TakeDamage");
            harmony.Patch(method,prefix:new HarmonyMethod(typeof(GodModeBridge),"AllowDamage"));
        }
        public static bool Active {get{return owner!=null;}}
        public static void Clear(){owner=null;}
        public static void Apply(PlayerMain player,bool enabled) {
            if(!enabled || player==null || !player.HasLocalControl || player.healthState!=PlayerMain.HealthState.Alive || player.healthFast<=0){Clear();return;}
            float maximum=player.MaxHealth;
            if(!(maximum>0) || float.IsInfinity(maximum)){Clear();return;}
            owner=player;heartbeat=Time.unscaledTime;
            player.healthFast=maximum;player.healthSlow=maximum;
        }
        static bool AllowDamage(PlayerMain __instance) {
            if(!ReferenceEquals(owner,__instance) || !__instance.HasLocalControl)return true;
            if(Time.unscaledTime-heartbeat>.5f || __instance.healthState!=PlayerMain.HealthState.Alive){Clear();return true;}
            return false;
        }
    }
}
