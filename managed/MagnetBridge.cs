using System;
using System.Collections.Generic;
using UnityEngine;

namespace Zb2Menu {
    public static class MagnetBridge {
        static int cursor;
        static ZombieLoader loader;
        static PlayerMain owner;
        static bool previous;
        static float nextPass;
        public static string Status="";
        public static bool Active {get{return previous || Status.Length>0;}}
        public static void Reset(){cursor=0;previous=false;owner=null;loader=null;Status="";nextPass=0;}
        public static void Apply(PlayerMain player,bool enabled,bool inputAllowed,float radius) {
            if(!enabled || player==null || player.healthFast<=0 || !player.HasLocalControl){Reset();return;}
            var multiplayer=MultiplayerController.instance;
            if(multiplayer==null || !multiplayer.IsServer()){Status="Magnet requer autoridade do host";cursor=0;previous=false;return;}
            var current=ZombieLoader.Instance;
            if(current==null || player.cam==null || player.cam.CameraTransform==null)return;
            if(!previous || owner!=player || loader!=current){cursor=0;owner=player;loader=current;previous=true;nextPass=0;}
            if(!inputAllowed || !Application.isFocused || GlobalTexting.IsTexting || DeveloperConsole.Opened || Time.timeScale<=0)return;
            if(Time.unscaledTime<nextPass)return;nextPass=Time.unscaledTime+.1f;
            if(float.IsNaN(radius)||float.IsInfinity(radius))radius=50;
            radius=Mathf.Clamp(radius,10,300);
            Vector3 origin=player.transform.position;
            Vector3 forward=player.cam.CameraTransform.forward;forward.y=0;
            if(forward.sqrMagnitude<.01f)return;forward.Normalize();
            Vector3 center=origin+forward*4;
            int count=0;
            int total=current.zombies.Count;
            for(int scanned=0;scanned<total;++scanned) {
                if(count>=4)break;
                int index=cursor%total;cursor=(index+1)%total;
                var zombie=current.zombies[index];
                if(zombie==null || zombie.obj==null || zombie.health==null || !zombie.health.isAlive || zombie.health.amount<=0)continue;
                if(Vector3.Distance(origin,zombie.obj.transform.position)>radius)continue;
                // A compact moving anchor follows the player. Small stable offsets
                // prevent rigidbodies from sharing exactly the same destination.
                int slot=(zombie.identity.id&int.MaxValue)%9;
                var proposed=center+new Vector3((slot%3-1)*.85f,3,(slot/3-1)*.85f);
                RaycastHit ground;
                if(!Physics.Raycast(proposed,Vector3.down,out ground,7,~0,QueryTriggerInteraction.Ignore) || ground.normal.y<.6f)continue;
                Vector3 target=ground.point+Vector3.up*.15f;
                if(Vector3.Distance(zombie.obj.transform.position,target)<1.25f)continue;
                if(Vector3.Distance(origin,target)>radius || Physics.CheckCapsule(target+Vector3.up*.4f,target+Vector3.up*1.6f,.35f,~0,QueryTriggerInteraction.Ignore))continue;
                zombie.TeleportTo(target,zombie.obj.transform.rotation);
                ++count;
            }
            Status="";
        }
    }
}
