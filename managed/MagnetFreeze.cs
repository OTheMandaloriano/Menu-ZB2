using System;
using System.Collections.Generic;
using HarmonyLib;
using UnityEngine;
namespace Zb2Menu {
    public static class MagnetFreeze {
        sealed class Saved {public bool Kinematic,RootMotion;public Vector3 Anchor;public Quaternion Rotation;public float Animation;public Rigidbody Body;public Animator Animator;}
        static readonly Dictionary<Zombie,Saved> owned=new Dictionary<Zombie,Saved>();
        static int heartbeatFrame;
        public static void Pulse(){
            heartbeatFrame=Time.frameCount;
            foreach(var zombie in new List<Zombie>(owned.Keys)) {
                if(zombie.obj==null || zombie.health==null || !zombie.health.isAlive || zombie.health.amount<=0 || IsSpawning(zombie)){Release(zombie);continue;}
                var saved=owned[zombie];
                if(saved.Body!=zombie.obj.body || saved.Animator!=zombie.obj.animator){var point=saved.Anchor;Release(zombie);Hold(zombie,point);continue;}
                Pin(zombie,saved);
            }
        }
        public static void Install(Harmony harmony) {
            foreach(string method in new[]{"UpdateStateMachine","UpdatePhysicsAndAnimation","UpdateBossBehaviour"}) {
                var target=AccessTools.Method(typeof(Zombie),method);
                if(target==null)throw new MissingMethodException("Zombie."+method);
                harmony.Patch(target,prefix:new HarmonyMethod(typeof(MagnetFreeze),"AllowUpdate"));
            }
        }
        static bool AllowUpdate(Zombie __instance) {
            if(!owned.ContainsKey(__instance))return true;
            if(IsSpawning(__instance)){Release(__instance);return true;}
            if(Time.frameCount-heartbeatFrame>60){Clear();return true;}
            if(__instance.obj==null || __instance.health==null || !__instance.health.isAlive || __instance.health.amount<=0){Release(__instance);return true;}
            if(MultiplayerController.instance==null || !MultiplayerController.instance.IsServer()){Clear();return true;}
            Pin(__instance,owned[__instance]);return false;
        }
        public static void Hold(Zombie zombie) {if(zombie!=null && zombie.obj!=null)Hold(zombie,zombie.obj.transform.position);}
        public static void Hold(Zombie zombie,Vector3 point) {
            heartbeatFrame=Time.frameCount;
            if(zombie==null || zombie.obj==null || IsSpawning(zombie))return;
            Saved saved;
            if(!owned.TryGetValue(zombie,out saved)){
                var body=zombie.obj.body;var animator=zombie.obj.animator;
                saved=new Saved{Body=body,Animator=animator,Kinematic=body!=null && body.isKinematic,Animation=animator==null?1:animator.speed,RootMotion=animator!=null && animator.applyRootMotion,Rotation=zombie.obj.transform.rotation};
                owned.Add(zombie,saved);
            }
            saved.Anchor=point;Pin(zombie,saved);
        }
        static void Pin(Zombie zombie,Saved saved) {
            if(saved.Body!=null){if(!saved.Body.isKinematic)saved.Body.linearVelocity=Vector3.zero;saved.Body.isKinematic=true;}
            if(saved.Animator!=null){saved.Animator.applyRootMotion=false;saved.Animator.speed=0;}
            zombie.obj.transform.position=saved.Anchor;zombie.obj.transform.rotation=saved.Rotation;
        }
        public static bool IsSpawning(Zombie zombie){return zombie!=null && (zombie.state==ZombieState.Spawning || (zombie.state==ZombieState.Transition && zombie.targetState==ZombieState.Spawning));}
        public static void Release(Zombie zombie) {
            Saved saved;if(!owned.TryGetValue(zombie,out saved))return;
            if(saved.Body!=null)saved.Body.isKinematic=saved.Kinematic;
            if(saved.Animator!=null){saved.Animator.speed=saved.Animation;saved.Animator.applyRootMotion=saved.RootMotion;}
            owned.Remove(zombie);
        }
        public static void Clear(){foreach(var zombie in new List<Zombie>(owned.Keys))Release(zombie);}
    }
}
