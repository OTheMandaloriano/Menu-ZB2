using System;
using System.Collections.Generic;
using UnityEngine;
namespace Zb2Menu {
    public static class LootMagnetBridge {
        static float nextPass;
        static readonly List<DroppedLoot> candidates=new List<DroppedLoot>();
        static readonly List<HashCell> candidateCells=new List<HashCell>();
        static MapHash scanMap;
        static int scanCell,scanItem;
        static bool active;
        static readonly HashSet<int> moved=new HashSet<int>();
        public static string Status="";
        public static void Apply(PlayerMain player,bool enabled,bool inputAllowed,float range,int category,int mask0,int mask1,int mask2,int mask3,bool wholeMap=false) {
            Status="";
            if(!enabled || player==null || !player.HasLocalControl || player.healthFast<=0){active=false;moved.Clear();return;}
            var multiplayer=MultiplayerController.instance;
            if(multiplayer==null || !multiplayer.IsServer()){active=false;moved.Clear();Status="Item Magnet requer autoridade do host";return;}
            if(!inputAllowed || !Application.isFocused || GlobalTexting.IsTexting || DeveloperConsole.Opened || Time.timeScale<=0 || Time.unscaledTime<nextPass)return;
            nextPass=Time.unscaledTime+.25f;
            var map=MapHash.instance;if(map==null || !map.IsCreated || LootController.Instance==null)return;
            int groundMask=player.movement==null?0:(int)player.movement.groundMask;
            if(groundMask==0){Status="Item Magnet: camada de chao indisponivel";return;}
            if(!active || scanMap!=map){scanMap=map;scanCell=scanItem=0;moved.Clear();active=true;}
            range=float.IsNaN(range)||float.IsInfinity(range)?50:Mathf.Clamp(range,10,1000);
            var origin=player.transform.position;var cell=map.GetCell(origin);if(cell==null || cell.loot==null || cell.lootHolder==null)return;
            int[] masks={mask0,mask1,mask2,mask3};candidates.Clear();candidateCells.Clear();
            if(map.width<=0 || map.height<=0)return;
            int cells=map.width*map.height,visited=0,budget=512;
            while(visited<cells && candidates.Count<2 && budget-->0){
                scanCell%=cells;
                var source=map.GetCell(scanCell%map.width,scanCell/map.width);
                if(source==null || source.loot==null || scanItem>=source.loot.Count){scanCell=(scanCell+1)%cells;scanItem=0;++visited;continue;}
                while(scanItem<source.loot.Count && candidates.Count<2 && budget-->0){
                    var loot=source.loot[scanItem++];
                    if(loot==null || loot.item==null || loot.IsSack || (loot.reservedPlayerID.HasValue && loot.reservedPlayerID.Value>=0))continue;
                    if(moved.Contains(loot.id))continue;
                    var db=loot.item.GetDataBaseItem();if(!ItemEligibility.Allowed(db))continue;
                    int id=(int)loot.item.id;if(id<0 || id>=128 || ((uint)masks[id/32]&(1u<<(id%32)))==0)continue;
                    bool weapon=db is DatabaseGun || db.GetSubType()==DatabaseItem.SubType.Melee;
                    bool ammo=loot.item.id.ToString().IndexOf("Ammo",StringComparison.Ordinal)>=0;
                    if((category==0 && !weapon)||(category==1 && !ammo)||(category==2 && (weapon||ammo)))continue;
                    float distance=Vector3.Distance(origin,loot.transform.position);if(distance<3 || (!wholeMap && distance>range))continue;
                    candidates.Add(loot);candidateCells.Add(source);if(candidates.Count==2)break;
                }
            }
            for(int i=0;i<candidates.Count;++i){
                var loot=candidates[i];if(loot==null || !ItemEligibility.Allowed(loot.item.GetDataBaseItem()))continue;
                RaycastHit ground;var proposed=origin+player.transform.forward*1.5f+player.transform.right*(i==0?-.5f:.5f)+Vector3.up*2;
                if(!Physics.Raycast(proposed,Vector3.down,out ground,4,groundMask,QueryTriggerInteraction.Ignore) || ground.normal.y<.6f)continue;
                float pivotHeight=0;
                var renderers=loot.GetComponentsInChildren<Renderer>(true);bool hasBounds=false;Bounds bounds=new Bounds();
                foreach(var renderer in renderers)if(renderer!=null){if(!hasBounds){bounds=renderer.bounds;hasBounds=true;}else bounds.Encapsulate(renderer.bounds);}
                if(!hasBounds)continue; // No dimensions: don't guess an offset that may bury the item.
                pivotHeight=loot.transform.position.y-bounds.min.y;
                if(float.IsNaN(pivotHeight)||float.IsInfinity(pivotHeight)||Math.Abs(pivotHeight)>5)continue;
                var old=candidateCells[i];int index=old.loot.IndexOf(loot);if(index<0)continue;
                // Keep the identity and item; remove/reannounce its network representation
                // using the game's own messages, and keep spatial cell ownership consistent.
                var destination=map.GetCell(ground.point);if(destination==null || destination.loot==null || destination.lootHolder==null)continue;
                if(multiplayer.IsOnlineServer())ServerController.instance.GetSpeaker.BroadcastLootRemoval(old,loot.id);
                var oldParent=loot.transform.parent;var oldPosition=loot.transform.position;
                try {
                    old.loot.RemoveAt(index);loot.transform.SetParent(destination.lootHolder.transform,true);loot.transform.position=ground.point+Vector3.up*(pivotHeight+.02f);destination.loot.Add(loot);
                }catch {
                    destination.loot.Remove(loot);if(!old.loot.Contains(loot))old.loot.Add(loot);loot.transform.SetParent(oldParent,true);loot.transform.position=oldPosition;
                    if(multiplayer.IsOnlineServer())ServerController.instance.GetSpeaker.SendSingleLootSpawn(loot,old);throw;
                }
                if(multiplayer.IsOnlineServer())ServerController.instance.GetSpeaker.SendSingleLootSpawn(loot,destination);
                moved.Add(loot.id);
            }
        }
    }
}
