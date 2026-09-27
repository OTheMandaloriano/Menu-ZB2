using System;
using System.Collections.Generic;
using UnityEngine;

namespace Zb2Menu {
    public static class MagnetBridge {
        static readonly HashSet<int> moved=new HashSet<int>();
        static ZombieLoader loader;
        static PlayerMain owner;
        static bool previous;
        static float nextPass;
        public static string Status="";
        public static bool Active {get{return previous || Status.Length>0;}}
        public static void Reset(){moved.Clear();previous=false;owner=null;loader=null;Status="";}
        public static void Apply(PlayerMain player,bool enabled,bool inputAllowed,float radius) {
            if(!enabled || player==null || player.healthFast<=0 || !player.HasLocalControl){Reset();return;}
            var multiplayer=MultiplayerController.instance;
            if(multiplayer==null || !multiplayer.IsServer()){Status="Magnet requer solo ou host";moved.Clear();previous=false;return;}
            var current=ZombieLoader.Instance;
            if(current==null || player.cam==null || player.cam.CameraTransform==null)return;
            if(!previous || owner!=player || loader!=current){moved.Clear();owner=player;loader=current;previous=true;}
            if(!inputAllowed || !Application.isFocused || GlobalTexting.IsTexting || DeveloperConsole.Opened || Time.timeScale<=0)return;
            if(Time.unscaledTime<nextPass)return;nextPass=Time.unscaledTime+.1f;
            if(float.IsNaN(radius)||float.IsInfinity(radius))radius=50;
            radius=Mathf.Clamp(radius,10,300);
            Vector3 origin=player.transform.position;
            Vector3 forward=player.cam.CameraTransform.forward;forward.y=0;
            if(forward.sqrMagnitude<.01f)return;forward.Normalize();
            Vector3 center=origin+forward*6;
            int count=0;
            foreach(var zombie in current.zombies) {
                if(count>=4)break;
                if(zombie==null || zombie.obj==null || zombie.health==null || !zombie.health.isAlive || zombie.health.amount<=0 || moved.Contains(zombie.identity.id))continue;
                if(Vector3.Distance(origin,zombie.obj.transform.position)>radius)continue;
                // Separate landing points avoid stacking rigidbodies on a single coordinate.
                float angle=(moved.Count%16)*Mathf.PI/8;
                float ring=2+(moved.Count/16)*1.5f;
                var proposed=center+new Vector3(Mathf.Cos(angle)*ring,3,Mathf.Sin(angle)*ring);
                RaycastHit ground;
                if(!Physics.Raycast(proposed,Vector3.down,out ground,7,~0,QueryTriggerInteraction.Ignore) || ground.normal.y<.6f)continue;
                Vector3 target=ground.point+Vector3.up*.15f;
                if(Vector3.Distance(origin,target)>radius || Physics.CheckCapsule(target+Vector3.up*.4f,target+Vector3.up*1.6f,.35f,~0,QueryTriggerInteraction.Ignore))continue;
                zombie.TeleportTo(target,zombie.obj.transform.rotation);
                moved.Add(zombie.identity.id);++count;
            }
            Status="Magnet: "+moved.Count+" zumbis reunidos (solo/host)";
        }
    }
}
