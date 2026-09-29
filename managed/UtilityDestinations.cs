using System;
using System.Collections.Generic;
using System.Reflection;
using UnityEngine;
namespace Zb2Menu {
    public static class UtilityDestinations {
        static readonly FieldInfo all=typeof(WorkbenchInteractions).GetField("allWorkbenches",BindingFlags.Instance|BindingFlags.NonPublic);
        public static string Workbench(PlayerMain player,InventoryDisplay.SideMenu kind) {
            var controller=WorkbenchInteractions.instance;
            var entries=controller==null || all==null?null:all.GetValue(controller) as List<InteractableFurniture>;
            WorkbenchInteractable best=null;float nearest=float.PositiveInfinity;
            if(entries!=null)foreach(var entry in entries){
                var bench=entry as WorkbenchInteractable;if(bench==null || bench.inventoryInteractionID!=kind)continue;
                var pyre=bench as PyreInteractable;if(pyre!=null && pyre.IsLit)continue;
                float distance=Vector3.Distance(player.transform.position,bench.InteractionPoint);
                if(distance<nearest){nearest=distance;best=bench;}
            }
            return best==null?"Nenhuma bancada disponivel":Teleport(player,best.InteractionPoint);
        }
        public static string Teleport(PlayerMain player,Vector3 point) {
            if(!Finite(point.x)||!Finite(point.y)||!Finite(point.z))return "Coordenadas invalidas";
            using(var collisions=new NearbyCollisionScope(point)){
                for(int i=0;i<25;++i){
                    float angle=i*(float)Math.PI/4;
                    float radius=i==0?0:.65f+((i-1)/8)*.3f;
                    var sample=point+new Vector3((float)Math.Sin(angle)*radius,1.5f,(float)Math.Cos(angle)*radius);
                    RaycastHit hit;
                    if(!Physics.Raycast(sample,Vector3.down,out hit,6,player.movement.groundMask,QueryTriggerInteraction.Ignore) || hit.normal.y<.7f)continue;
                    float height=player.defaultHeight;
                    if(!(height>.5f && height<4))return "Altura indisponivel";
                    bool blocked=false;
                    foreach(var collider in Physics.OverlapCapsule(hit.point+Vector3.up*.4f,hit.point+Vector3.up*(height-.27f),.3f,player.movement.groundMask,QueryTriggerInteraction.Ignore))
                        if(collider!=null && !collider.transform.IsChildOf(player.transform)){blocked=true;break;}
                    if(blocked)continue;
                    player.transform.position=hit.point+Vector3.up*(height*.5f+.08f);collisions.Arrived();
                    var body=player.GetComponent<Rigidbody>();if(body!=null && !body.isKinematic)body.linearVelocity=Vector3.zero;
                    return "Teleportado para ponto livre com chao";
                }
            }
            return "Destino sem chao ou espaco livre";
        }
        static bool Finite(float value){return !float.IsNaN(value)&&!float.IsInfinity(value)&&Math.Abs(value)<100000;}
    }
}
