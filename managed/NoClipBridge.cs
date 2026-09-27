using System;
using HarmonyLib;
using UnityEngine;

namespace Zb2Menu {
    public static class NoClipBridge {
        static NoClip active;
        static PlayerMain player;
        static bool wasEnabled,wasKinematic,detected;
        static float originalSpeed;
        static bool inputAllowed;
        static float published;
        static Vector3 safePosition;
        static readonly Collider[] overlaps=new Collider[64];
        public static string Status="";
        public static bool Active {get{return active!=null;}}
        public static void Install(Harmony harmony) {
            harmony.Patch(AccessTools.Method(typeof(NoClip),"Update"),prefix:new HarmonyMethod(typeof(NoClipBridge),"BeforeUpdate"));
        }
        public static void Apply(PlayerMain owner,bool enabled,float multiplier,bool allowInput,float normalWalk=0) {
            if(active!=null && (owner!=player || !enabled))Restore();
            if(!enabled || owner==null || owner.noClip==null || owner.noClip.targetBody==null)return;
            if(active==null) {
                if(Blocked(owner)){Status="Ative NoClip fora de obstaculos";return;}
                active=owner.noClip;player=owner;wasEnabled=active.enabled;
                safePosition=active.transform.position;
                wasKinematic=active.targetBody.isKinematic;detected=active.targetBody.detectCollisions;
                originalSpeed=active.speed;
            }
            if(!(normalWalk>0) && owner.movement!=null)normalWalk=owner.movement.walkSpeed;
            if(!(normalWalk>0) || float.IsInfinity(normalWalk)){Restore();Status="Velocidade normal indisponivel";return;}
            active.speed=normalWalk*Mathf.Clamp(float.IsNaN(multiplier)||float.IsInfinity(multiplier)?1:multiplier,.5f,5);
            active.enabled=true;active.targetBody.isKinematic=true;active.targetBody.detectCollisions=false;
            inputAllowed=allowInput;published=Time.unscaledTime;Status="";
        }
        public static void Restore() {
            if(active!=null) {
                if(player!=null && Blocked(player))active.transform.position=safePosition;
                active.speed=originalSpeed;active.enabled=wasEnabled;
                if(active.targetBody!=null){active.targetBody.isKinematic=wasKinematic;active.targetBody.detectCollisions=detected;}
            }
            active=null;player=null;Status="";
        }
        static bool BeforeUpdate(NoClip __instance) {
            if(__instance!=active)return true;
            if(player==null || player.healthFast<=0 || Time.unscaledTime-published>.25f){Restore();return false;}
            if(!inputAllowed || !Application.isFocused || Time.timeScale<=0 || GlobalTexting.IsTexting || DeveloperConsole.Opened)return false;
            var direction=new Vector3((Input.GetKey(KeyCode.D)?1:0)-(Input.GetKey(KeyCode.A)?1:0),
                (Input.GetKey(KeyCode.Space)?1:0)-(Input.GetKey(KeyCode.LeftControl)?1:0),
                (Input.GetKey(KeyCode.W)?1:0)-(Input.GetKey(KeyCode.S)?1:0));
            if(direction.sqrMagnitude>1)direction.Normalize();
            var basis=player.cam!=null ? player.cam.CameraTransform : active.transform;
            active.transform.position+=basis.TransformDirection(direction)*active.speed*Time.deltaTime;
            if(!Blocked(player))safePosition=active.transform.position;
            return false;
        }
        static bool Blocked(PlayerMain owner) {
            if(owner.movement==null || owner.movement.hitbox==null)return false;
            var bounds=owner.movement.hitbox.bounds;
            int count=Physics.OverlapBoxNonAlloc(bounds.center,bounds.extents*.9f,overlaps,Quaternion.identity,~0,QueryTriggerInteraction.Ignore);
            if(count==overlaps.Length)return true;
            for(int i=0;i<count;++i)if(overlaps[i]!=null && !overlaps[i].transform.IsChildOf(owner.transform))return true;
            return false;
        }
    }
}
