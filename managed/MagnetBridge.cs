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
        static readonly Dictionary<int,int> slots=new Dictionary<int,int>();
        static readonly Dictionary<int,int> bossSlots=new Dictionary<int,int>();
        static readonly Dictionary<int,Vector3> placed=new Dictionary<int,Vector3>();
        static readonly HashSet<int> requestedBosses=new HashSet<int>();
        static Vector3 anchor;
        static bool anchorSet;
        public static string Status="";
        public static bool Active {get{return previous || Status.Length>0;}}
        public static void Reset(){MagnetFreeze.Clear();cursor=0;previous=false;owner=null;loader=null;Status="";nextPass=0;slots.Clear();bossSlots.Clear();placed.Clear();requestedBosses.Clear();anchorSet=false;}
        public static void Apply(PlayerMain player,bool enabled,bool inputAllowed,float radius,int targetMode=0,float frontDistance=2.5f,float bossDistance=8,bool freeze=false) {
            if(!enabled || player==null || player.healthFast<=0 || !player.HasLocalControl){Reset();return;}
            var multiplayer=MultiplayerController.instance;
            if(multiplayer==null || !multiplayer.IsServer()){Reset();Status="Magnet requer autoridade do host";return;}
            var current=ZombieLoader.Instance;
            if(current==null || player.cam==null || player.cam.CameraTransform==null)return;
            if(!previous || owner!=player || loader!=current){Reset();owner=player;loader=current;previous=true;}
            if(!freeze)MagnetFreeze.Clear();
            else MagnetFreeze.Pulse();
            if(!inputAllowed || !Application.isFocused || GlobalTexting.IsTexting || DeveloperConsole.Opened || Time.timeScale<=0)return;
            if(Time.unscaledTime<nextPass)return;nextPass=Time.unscaledTime+.1f;
            if(float.IsNaN(radius)||float.IsInfinity(radius))radius=50;
            radius=Mathf.Clamp(radius,10,300);
            if(targetMode<0 || targetMode>2)targetMode=0;
            if(targetMode!=1)LoadOneBoss(current);
            Vector3 origin=player.transform.position;
            Vector3 forward=player.cam.CameraTransform.forward;forward.y=0;
            if(forward.sqrMagnitude<.01f)return;forward.Normalize();
            frontDistance=FiniteDistance(frontDistance,2.5f,1.5f,10);bossDistance=FiniteDistance(bossDistance,8,6,20);
            Vector3 center=origin+forward*frontDistance;
            bool relocated=!anchorSet || Vector3.Distance(anchor,center)>2;
            if(relocated){anchor=center;anchorSet=true;placed.Clear();}
            int count=0;
            int total=current.zombies.Count;
            for(int scanned=0;scanned<total;++scanned) {
                if(count>=4)break;
                int index=cursor%total;cursor=(index+1)%total;
                var zombie=current.zombies[index];
                if(zombie==null || zombie.obj==null || zombie.health==null || !zombie.health.isAlive || zombie.health.amount<=0)continue;
                bool boss=zombie.identity.IsBoss;
                if((targetMode==1 && boss) || (targetMode==2 && !boss)){MagnetFreeze.Release(zombie);continue;}
                if(!boss && Vector3.Distance(origin,zombie.obj.transform.position)>radius)continue;
                // One placement per stationary anchor. AI movement does not trigger
                // another teleport every tick; only a new anchor does.
                if(placed.ContainsKey(zombie.identity.id)){if(freeze)MagnetFreeze.Hold(zombie);continue;}
                var allocation=boss?bossSlots:slots;
                int slot;if(!allocation.TryGetValue(zombie.identity.id,out slot)){slot=allocation.Count;allocation.Add(zombie.identity.id,slot);}
                float spacing=boss?4:1.2f;
                var destination=boss?origin+forward*bossDistance:anchor;
                int columns=boss?3:5;
                var proposed=destination+new Vector3((slot%columns-columns/2)*spacing,3,(slot/columns)*spacing);
                RaycastHit ground;
                if(!Physics.Raycast(proposed,Vector3.down,out ground,7,~0,QueryTriggerInteraction.Ignore) || ground.normal.y<.6f)continue;
                Vector3 target=ground.point+Vector3.up*.15f;
                if(Vector3.Distance(zombie.obj.transform.position,target)<1.25f){placed[zombie.identity.id]=target;if(freeze)MagnetFreeze.Hold(zombie);continue;}
                float bodyRadius=boss?1.5f:.4f,bodyHeight=boss?4:1.6f;
                if((!boss && Vector3.Distance(origin,target)>radius) || Physics.CheckCapsule(target+Vector3.up*bodyRadius,target+Vector3.up*bodyHeight,bodyRadius,~0,QueryTriggerInteraction.Ignore))continue;
                zombie.TeleportTo(target,zombie.obj.transform.rotation);
                placed[zombie.identity.id]=target;
                if(freeze)MagnetFreeze.Hold(zombie);
                ++count;
            }
            Status="";
        }
        static float FiniteDistance(float value,float fallback,float min,float max){return float.IsNaN(value)||float.IsInfinity(value)?fallback:Mathf.Clamp(value,min,max);}
        static void LoadOneBoss(ZombieLoader current) {
            int id=-1;
            foreach(var zombie in current.unloadedZombies)if(zombie!=null && zombie.identity.IsBoss && !requestedBosses.Contains(zombie.identity.id)){id=zombie.identity.id;break;}
            if(id<0)foreach(var zombie in current.zombieProps)if(zombie!=null && zombie.identity.IsBoss && !requestedBosses.Contains(zombie.identity.id)){id=zombie.identity.id;break;}
            // The game owns conversion and network state. Never mutate the list being enumerated.
            if(id>=0){requestedBosses.Add(id);current.ForceLoadRealZombie(id);}
        }
    }
}
