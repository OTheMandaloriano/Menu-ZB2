using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using UnityEngine;

namespace Zb2Menu {
    public static class WorldEspBridge {
        [StructLayout(LayoutKind.Sequential, Pack=4)]
        struct Marker {
            public int Kind;
            public float X, Y, Distance;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst=96)] public byte[] Name;
            public float Left,Top,Right,Bottom,Health;
            public float MaxHealth;
            [MarshalAs(UnmanagedType.ByValArray,SizeConst=24)] public float[] Corners;
            [MarshalAs(UnmanagedType.ByValArray,SizeConst=51)] public float[] Bones;
        }
        static readonly List<Marker> result = new List<Marker>(256);
        static readonly float[] emptyCorners=new float[24],emptyBones=new float[51];
        sealed class LootEntry { public DroppedLoot Loot; public Renderer[] Renderers; public string Name; public int Kind; public float Distance; }
        static readonly List<LootEntry> lootCache = new List<LootEntry>(512);
        static MapHash previousMap;
        static float nextLootScan;
        static int lastLootMask;
        static float lastLootRadius;
        static Camera camera;
        static Vector3 origin;
        static int mask;
        static float radius, poiRadius;
        static readonly int[] itemFilters=new int[4];
        static int pointFilter;
        static int teamCount;
        static string error="";
        public static string LastError() { return error; }
        // Kind bits: weapons, rare, ammo, supply, heli, boss, mission, wave,
        // fixed loot, bench, fire, shop, respawn, allies.
        static void Add(int kind, string name, Vector3 position, bool limited) {
            if (kind < 0 || (mask & (1 << kind))==0) return;
            float distance=Vector3.Distance(origin,position);
            if (float.IsNaN(distance) || float.IsInfinity(distance) || (limited && distance>(kind<4 ? radius : poiRadius))) return;
            var screen=camera.WorldToViewportPoint(position);
            if (!(screen.z > .01f && screen.x>=0 && screen.x<=1 && screen.y>=0 && screen.y<=1)) return;
            var entry=new Marker {Kind=kind, X=screen.x, Y=1-screen.y, Distance=distance,Corners=emptyCorners,Bones=emptyBones};
            entry.Name=new byte[96];
            string label=name ?? "?";
            // Truncate characters before UTF8 encoding, avoiding partial sequences.
            if (label.Length>28) label=label.Substring(0,28);
            byte[] bytes=Encoding.UTF8.GetBytes(label);
            Array.Copy(bytes,entry.Name,Math.Min(bytes.Length,95));
            if (result.Count<256) result.Add(entry);
            else {
                int farthest=0;
                for (int i=1;i<result.Count;++i) if(result[i].Distance>result[farthest].Distance) farthest=i;
                if(distance<result[farthest].Distance) result[farthest]=entry;
            }
        }
        public static int Collect(IntPtr buffer, int capacity, int enabled, float range, float pointRange,
            int item0,int item1,int item2,int item3,int points) {
            result.Clear(); error="";
            if (buffer==IntPtr.Zero || capacity<=0 || enabled==0 || MainCamera.instance==null) return 0;
            camera=MainCamera.instance.cam;
            var player=PlayersController.instance == null ? null : PlayersController.instance.MyPlayer();
            if (camera==null || player==null || player.healthFast<=0) return 0;
            origin=player.transform.position; mask=enabled;
            radius=ClampRadius(range);
            poiRadius=ClampRadius(pointRange);
            if(itemFilters[0]!=item0 || itemFilters[1]!=item1 || itemFilters[2]!=item2 || itemFilters[3]!=item3)nextLootScan=0;
            itemFilters[0]=item0;itemFilters[1]=item1;itemFilters[2]=item2;itemFilters[3]=item3;pointFilter=points;
            try {
                if ((mask & 15)!=0) Items();
                if ((mask & ~15)!=0) Points();
                if ((mask & (1<<14))!=0) Team();
                if ((mask & (1<<7))!=0) Wave();
            } catch(Exception ex) { error=ex.ToString(); }
            int count=Math.Min(capacity,result.Count), stride=Marshal.SizeOf(typeof(Marker));
            for(int i=0;i<count;++i) Marshal.StructureToPtr(result[i],IntPtr.Add(buffer,i*stride),false);
            return count;
        }
        static float ClampRadius(float value) {
            return float.IsNaN(value) || float.IsInfinity(value) ? 150 : Math.Max(10,Math.Min(500,value));
        }
        static void Items() {
            var map=MapHash.instance;
            if(map==null || !map.IsCreated || !(map.cellSize>0)) { lootCache.Clear(); return; }
            if(map!=previousMap || (mask&15)!=lastLootMask || radius!=lastLootRadius || Time.unscaledTime>=nextLootScan) {
                lastLootMask=mask&15; lastLootRadius=radius;
                previousMap=map; nextLootScan=Time.unscaledTime+.5f;
                RefreshItems(map);
            }
            foreach(var entry in lootCache)
                if(entry.Loot!=null && entry.Loot.item!=null && ItemEligibility.Allowed(entry.Loot.item.GetDataBaseItem())) Add(entry.Kind,entry.Name,LootCenter(entry),true);
        }
        static Vector3 LootCenter(LootEntry entry) {
            Bounds bounds=new Bounds();bool found=false;
            foreach(var renderer in entry.Renderers)if(renderer!=null && renderer.enabled && renderer.gameObject.activeInHierarchy) {
                if(!found){bounds=renderer.bounds;found=true;}else bounds.Encapsulate(renderer.bounds);
            }
            return found?bounds.center:entry.Loot.transform.position;
        }
        static void RefreshItems(MapHash map) {
            lootCache.Clear();
            var first=map.GetHashCoord(origin-new Vector3(radius,0,radius));
            var last=map.GetHashCoord(origin+new Vector3(radius,0,radius));
            for(int x=first.x;x<=last.x;++x) for(int y=first.y;y<=last.y;++y) {
                var cell=map.GetCell(x,y);
                if(cell==null || cell.loot==null) continue;
                foreach(var loot in cell.loot) {
                    if(loot==null || loot.item==null) continue;
                    float distance=Vector3.Distance(origin,loot.transform.position);
                    if(!(distance<=radius)) continue;
                    var db=loot.item.GetDataBaseItem();
                    if(!ItemEligibility.Allowed(db)) continue;
                    var id=loot.item.id;
                    int index=(int)id;
                    if(index<0 || index>=128 || ((uint)itemFilters[index/32] & (1u<<(index%32)))==0)continue;
                    bool ammo=id==InventoryItem.ID.RifleAmmo || id==InventoryItem.ID.SniperAmmo ||
                        id==InventoryItem.ID.ShotgunAmmo || id.ToString().IndexOf("Ammo",StringComparison.Ordinal)>=0;
                    var subtype=db.GetSubType();
                    bool weapon=db is DatabaseGun || subtype==DatabaseItem.SubType.Melee;
                    int kind=ammo ? 2 : weapon ? 0 : 3;
                    // Rare is an additional filter, not a reason to hide an enabled category.
                    if((int)db.tier>=2 && (mask & 2)!=0) kind=1;
                    if((mask & (1<<kind))==0) continue;
                    var entry=new LootEntry {Loot=loot,Renderers=loot.GetComponentsInChildren<Renderer>(true),Name=db.GetName,Kind=kind,Distance=distance};
                    if(lootCache.Count<512) lootCache.Add(entry);
                    else {
                        int farthest=0;
                        for(int i=1;i<lootCache.Count;++i) if(lootCache[i].Distance>lootCache[farthest].Distance) farthest=i;
                        if(distance<lootCache[farthest].Distance) lootCache[farthest]=entry;
                    }
                }
            }
        }
        static void Points() {
            var owner=InterestPointController.instance;
            if(owner==null || owner.points==null) return;
            foreach(var point in owner.points) {
                if(point==null) continue;
                int index=(int)point.type;
                if(index<0 || index>=32 || ((uint)pointFilter & (1u<<index))==0)continue;
                int kind; string name;
                switch(point.type) {
                    case InterestPoint.Type.Gravestone: kind=13; name="Sepultura"; break;
                    case InterestPoint.Type.OtherPlayerDot: continue;
                    case InterestPoint.Type.Helicopter: kind=4; name="Helicoptero"; break;
                    case InterestPoint.Type.Bossfight: kind=5; name="Chefao"; break;
                    case InterestPoint.Type.QuestionMark: kind=6; name="Ponto desconhecido"; break;
                    case InterestPoint.Type.Bomb: kind=6; name="Bomba"; break;
                    case InterestPoint.Type.Ammo: kind=8; name="Area: municao"; break;
                    case InterestPoint.Type.Gun: kind=8; name="Area: armas"; break;
                    case InterestPoint.Type.Melee: kind=8; name="Area: armas brancas"; break;
                    case InterestPoint.Type.HealingItem: kind=8; name="Area: cura"; break;
                    case InterestPoint.Type.CraftingMaterial: kind=8; name="Area: materiais"; break;
                    case InterestPoint.Type.Food: kind=8; name="Area: comida"; break;
                    case InterestPoint.Type.ReloadingBench: kind=9; name="Bancada de recarga"; break;
                    case InterestPoint.Type.GunUpgradeTable: kind=9; name="Bancada de melhoria"; break;
                    case InterestPoint.Type.ZumbiePyre: kind=10; name="Fogueira"; break;
                    case InterestPoint.Type.VendorVan: kind=11; name="Mercador"; break;
                    case InterestPoint.Type.StartingHouse: kind=12; name="Respawn"; break;
                    default: continue;
                }
                var position=point.objTransform!=null ? point.objTransform.position : point.pos3D;
                Add(kind,name,position,kind>=8 && kind<=11);
            }
        }
        static void Team() {
            teamCount=0;
            foreach(var player in PlayersController.instance.players) {
                if(player==null || player.HasLocalControl || player.healthState==PlayerMain.HealthState.Dead || player.movement==null || player.movement.hitbox==null)continue;
                if(teamCount++>=32)break;
                var bounds=player.movement.hitbox.bounds;
                float distance=Vector3.Distance(origin,bounds.center);
                if(!(distance>=0) || float.IsInfinity(distance))continue;
                float left=1,right=0,top=1,bottom=0;int projected=0;var corners=new float[24];
                for(int corner=0;corner<8;++corner){
                    var point=bounds.center+new Vector3((corner&1)==0?-bounds.extents.x:bounds.extents.x,(corner&2)==0?-bounds.extents.y:bounds.extents.y,(corner&4)==0?-bounds.extents.z:bounds.extents.z);
                    var screen=camera.WorldToViewportPoint(point);if(!(screen.z>.01f))continue;
                    corners[corner*3]=screen.x;corners[corner*3+1]=1-screen.y;corners[corner*3+2]=1;
                    left=Math.Min(left,screen.x);right=Math.Max(right,screen.x);top=Math.Min(top,1-screen.y);bottom=Math.Max(bottom,1-screen.y);++projected;
                }
                if(projected<2 || right<0 || left>1 || bottom<0 || top>1)continue;
                // Remote peers synchronize HealthState, not numeric healthFast/MaxHealth.
                // Negative values encode known state without fabricating a percentage.
                var marker=new Marker{Kind=14,X=(left+right)*.5f,Y=bottom,Distance=distance,Left=Math.Max(0,left),Right=Math.Min(1,right),Top=Math.Max(0,top),Bottom=Math.Min(1,bottom),Health=player.healthState==PlayerMain.HealthState.Dying?-2:-1,MaxHealth=0,Name=new byte[96],Corners=corners,Bones=new float[51]};
                TeamBones(player,marker.Bones);
                string name=player.lobbyPlayer==null?"Aliado":player.lobbyPlayer.playerName;
                if(string.IsNullOrWhiteSpace(name))name="Aliado";
                if(name.Length>28)name=name.Substring(0,28);
                var bytes=Encoding.UTF8.GetBytes(name);Array.Copy(bytes,marker.Name,Math.Min(bytes.Length,95));
                // Reserve room for actual teammates, not generic map dots.
                if(result.Count>=256)result.RemoveAt(result.Count-1);
                result.Add(marker);
            }
        }
        static readonly HumanBodyBones[] joints={HumanBodyBones.Head,HumanBodyBones.Neck,HumanBodyBones.Chest,HumanBodyBones.Spine,HumanBodyBones.Hips,
            HumanBodyBones.LeftUpperArm,HumanBodyBones.LeftLowerArm,HumanBodyBones.LeftHand,HumanBodyBones.RightUpperArm,HumanBodyBones.RightLowerArm,HumanBodyBones.RightHand,
            HumanBodyBones.LeftUpperLeg,HumanBodyBones.LeftLowerLeg,HumanBodyBones.LeftFoot,HumanBodyBones.RightUpperLeg,HumanBodyBones.RightLowerLeg,HumanBodyBones.RightFoot};
        static void TeamBones(PlayerMain player,float[] output){
            var skin=player.SpawnedSkin;if(skin==null || skin.animator==null || !skin.animator.isHuman)return;
            for(int i=0;i<joints.Length;++i){var bone=skin.animator.GetBoneTransform(joints[i]);if(bone==null)continue;var point=camera.WorldToViewportPoint(bone.position);if(!(point.z>.01f))continue;output[i*3]=point.x;output[i*3+1]=1-point.y;output[i*3+2]=1;}
        }
        static void Wave(){
            var loader=ZombieLoader.Instance;if(loader==null)return;Vector3 sum=Vector3.zero;int count=0;
            foreach(var zombie in loader.zombies)if(zombie!=null && zombie.isWaveZombie && zombie.obj!=null && zombie.health!=null && zombie.health.isAlive){sum+=zombie.obj.transform.position;if(++count>=512)break;}
            if(count>0)Add(7,"Horda ativa ("+count+")",sum/count,false);
        }
    }
}
