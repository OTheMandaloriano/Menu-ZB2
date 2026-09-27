using System;
using System.Collections.Generic;
using HarmonyLib;
using UnityEngine;
namespace Zb2Menu {
    public static class MagnetFreeze {
        sealed class Saved {public bool Kinematic;public float Animation;public Rigidbody Body;public Animator Animator;}
        static readonly Dictionary<Zombie,Saved> owned=new Dictionary<Zombie,Saved>();
        static float heartbeat;
        public static void Pulse(){heartbeat=Time.unscaledTime;}
        public static void Install(Harmony harmony) {
            foreach(string method in new[]{"UpdateStateMachine","UpdatePhysicsAndAnimation","UpdateBossBehaviour"}) {
                var target=AccessTools.Method(typeof(Zombie),method);
                if(target==null)throw new MissingMethodException("Zombie."+method);
                harmony.Patch(target,prefix:new HarmonyMethod(typeof(MagnetFreeze),"AllowUpdate"));
            }
        }
        static bool AllowUpdate(Zombie __instance) {
            if(!owned.ContainsKey(__instance))return true;
            if(Time.unscaledTime-heartbeat>.5f){Clear();return true;}
            if(__instance.obj==null || __instance.health==null || !__instance.health.isAlive || __instance.health.amount<=0){Release(__instance);return true;}
            if(MultiplayerController.instance==null || !MultiplayerController.instance.IsServer()){Clear();return true;}
            return false;
        }
        public static void Hold(Zombie zombie) {
            Pulse();
            if(zombie==null || zombie.obj==null || owned.ContainsKey(zombie))return;
            var body=zombie.obj.body;var animator=zombie.obj.animator;
            var saved=new Saved{Body=body,Animator=animator,Kinematic=body!=null && body.isKinematic,Animation=animator==null?1:animator.speed};
            owned.Add(zombie,saved);
            if(body!=null){if(!body.isKinematic)body.linearVelocity=Vector3.zero;body.isKinematic=true;}
            if(animator!=null)animator.speed=0;
        }
        public static void Release(Zombie zombie) {
            Saved saved;if(!owned.TryGetValue(zombie,out saved))return;
            if(saved.Body!=null)saved.Body.isKinematic=saved.Kinematic;
            if(saved.Animator!=null)saved.Animator.speed=saved.Animation;
            owned.Remove(zombie);
        }
        public static void Clear(){foreach(var zombie in new List<Zombie>(owned.Keys))Release(zombie);}
    }
}
