using System;
using System.Collections.Generic;
using UnityEngine;
namespace Zb2Menu {
    public static class LootMagnetBridge {
        static float nextPass;
        static readonly List<DroppedLoot> candidates=new List<DroppedLoot>();
        public static string Status="";
        public static void Apply(PlayerMain player,bool enabled,bool inputAllowed,float range,int category,int mask0,int mask1,int mask2,int mask3) {
            Status="";
            if(!enabled || player==null || !player.HasLocalControl || player.healthFast<=0)return;
            var multiplayer=MultiplayerController.instance;
            if(multiplayer==null || !multiplayer.IsServer()){Status="Item Magnet requer autoridade do host";return;}
            if(!inputAllowed || !Application.isFocused || GlobalTexting.IsTexting || DeveloperConsole.Opened || Time.timeScale<=0 || Time.unscaledTime<nextPass)return;
            nextPass=Time.unscaledTime+.25f;
            var map=MapHash.instance;if(map==null || !map.IsCreated || LootController.Instance==null)return;
            range=float.IsNaN(range)||float.IsInfinity(range)?50:Mathf.Clamp(range,10,200);
            var origin=player.transform.position;var cell=map.GetCell(origin);if(cell==null || cell.loot==null || cell.lootHolder==null)return;
            var first=map.GetHashCoord(origin-new Vector3(range,0,range));var last=map.GetHashCoord(origin+new Vector3(range,0,range));
            int[] masks={mask0,mask1,mask2,mask3};candidates.Clear();
            for(int x=first.x;x<=last.x && candidates.Count<2;++x)for(int y=first.y;y<=last.y && candidates.Count<2;++y){
                var source=map.GetCell(x,y);if(source==null || source.loot==null)continue;
                foreach(var loot in source.loot){
                    if(loot==null || loot.item==null || loot.IsSack || (loot.reservedPlayerID.HasValue && loot.reservedPlayerID.Value>=0))continue;
                    var db=loot.item.GetDataBaseItem();if(!ItemEligibility.Allowed(db))continue;
                    int id=(int)loot.item.id;if(id<0 || id>=128 || ((uint)masks[id/32]&(1u<<(id%32)))==0)continue;
                    bool weapon=db is DatabaseGun || db.GetSubType()==DatabaseItem.SubType.Melee;
                    bool ammo=loot.item.id.ToString().IndexOf("Ammo",StringComparison.Ordinal)>=0;
                    if((category==0 && !weapon)||(category==1 && !ammo)||(category==2 && (weapon||ammo)))continue;
                    float distance=Vector3.Distance(origin,loot.transform.position);if(distance<3 || distance>range)continue;
                    candidates.Add(loot);if(candidates.Count==2)break;
                }
            }
            for(int i=0;i<candidates.Count;++i){
                var loot=candidates[i];if(loot==null || !ItemEligibility.Allowed(loot.item.GetDataBaseItem()))continue;
                RaycastHit ground;var proposed=origin+player.transform.forward*1.5f+player.transform.right*(i==0?-.5f:.5f)+Vector3.up*2;
                if(!Physics.Raycast(proposed,Vector3.down,out ground,4,~0,QueryTriggerInteraction.Ignore) || ground.normal.y<.6f)continue;
                int index;var old=LootController.Instance.GetLootLocation(loot.id,out index);if(old==null || index<0 || index>=old.loot.Count || old.loot[index]!=loot)continue;
                // Keep the identity and item; remove/reannounce its network representation
                // using the game's own messages, and keep spatial cell ownership consistent.
                var destination=map.GetCell(ground.point);if(destination==null || destination.loot==null || destination.lootHolder==null)continue;
                if(multiplayer.IsOnlineServer())ServerController.instance.GetSpeaker.BroadcastLootRemoval(old,loot.id);
                var oldParent=loot.transform.parent;var oldPosition=loot.transform.position;
                try {
                    old.loot.RemoveAt(index);loot.transform.SetParent(destination.lootHolder.transform,true);loot.transform.position=ground.point+Vector3.up*.1f;destination.loot.Add(loot);
                }catch {
                    destination.loot.Remove(loot);if(!old.loot.Contains(loot))old.loot.Add(loot);loot.transform.SetParent(oldParent,true);loot.transform.position=oldPosition;
                    if(multiplayer.IsOnlineServer())ServerController.instance.GetSpeaker.SendSingleLootSpawn(loot,old);throw;
                }
                if(multiplayer.IsOnlineServer())ServerController.instance.GetSpeaker.SendSingleLootSpawn(loot,destination);
            }
        }
    }
}
