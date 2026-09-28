using System;
using System.Collections.Generic;
using System.Reflection;
using UnityEngine;

namespace Zb2Menu {
    public static class PyreNavigation {
        static readonly FieldInfo benches=typeof(WorkbenchInteractions).GetField("allWorkbenches",BindingFlags.Instance|BindingFlags.NonPublic);
        static readonly List<PyreInteractable> pyres=new List<PyreInteractable>();
        static WorkbenchInteractions owner;
        static float nextRefresh;
        static int lastRequest,lastSelection=-1;
        static float statusUntil;
        static string status="";
        public static IList<PyreInteractable> All() {
            var current=WorkbenchInteractions.instance;
            if(current==null){owner=null;pyres.Clear();return pyres;}
            if(current==owner && Time.unscaledTime<nextRefresh)return pyres;
            owner=current;nextRefresh=Time.unscaledTime+1;pyres.Clear();
            var all=benches==null?null:benches.GetValue(current) as List<InteractableFurniture>;
            if(all!=null)foreach(var item in all){var pyre=item as PyreInteractable;if(pyre!=null)pyres.Add(pyre);}
            return pyres;
        }
        public static string Status(){return status;}
        public static int Update(int selected,int request,int ready) {
            bool triggered=request!=lastRequest;lastRequest=request;
            if(ready==0){status="Entre no mapa";statusUntil=0;lastSelection=-1;pyres.Clear();owner=null;return 0;}
            try {
                var list=All();int count=list.Count;
                if(count==0){status="Nenhum braseiro registrado";return 0;}
                selected=Math.Max(0,Math.Min(count-1,selected));
                var target=list[selected];var player=PlayersController.instance.MyPlayer();
                if(target==null || player==null){status="Destino indisponivel";return count;}
                if(selected!=lastSelection){statusUntil=0;lastSelection=selected;}
                if(triggered){status=Teleport(player,target);statusUntil=Time.unscaledTime+5;return count;}
                if(Time.unscaledTime<statusUntil)return count;
                status=string.Format("Braseiro {0}/{1} | {2} | {3:0} m",selected+1,count,target.IsLit?"aceso":"apagado",Vector3.Distance(player.transform.position,target.transform.position));
                return count;
            }catch(Exception ex){status="Braseiro: "+ex.GetType().Name;return 0;}
        }
        public static string LastProbe="";
        static string Teleport(PlayerMain player,PyreInteractable target) {
            if(!player.HasLocalControl || player.healthFast<=0)return "Jogador local indisponivel";
            float height=player.defaultHeight;
            if(!(height>.5f && height<4))return "Altura do jogador invalida";
            Vector3 interaction=target.InteractionPoint;
            int noGround=0,obstacles=0,outOfReach=0;
            string blocker="";int groundMask=player.movement.groundMask;
            using(var collision=new NearbyCollisionScope(interaction)) {
                for(int ring=0;ring<4;++ring)for(int angle=0;angle<24;++angle){
                    float radians=angle*(float)Math.PI/12;
                    float distance=.65f+ring*.25f;
                    var horizontal=new Vector3((float)Math.Sin(radians),0,(float)Math.Cos(radians))*distance;
                    RaycastHit hit;
                    if(!Physics.Raycast(interaction+horizontal+Vector3.up*.35f,Vector3.down,out hit,height+2,groundMask,QueryTriggerInteraction.Ignore) || hit.normal.y<.7f){++noGround;continue;}
                    var destination=hit.point+Vector3.up*(height*.5f+.08f);
                    // WorkbenchInteractions.CloseEnoughTo uses a strict 1.5 m sphere.
                    if(Vector3.Distance(destination,interaction)>=1.45f){++outOfReach;continue;}
                    float radius=Math.Min(.35f,height*.25f);bool blocked=false;
                    foreach(var collider in Physics.OverlapCapsule(hit.point+Vector3.up*(radius+.08f),hit.point+Vector3.up*(height-radius+.08f),radius,groundMask,QueryTriggerInteraction.Ignore))
                        if(collider!=null && !collider.transform.IsChildOf(player.transform)){blocked=true;blocker=collider.name;break;}
                    if(blocked){++obstacles;continue;}
                    player.transform.position=destination;
                    var facing=interaction+destination*(-1);facing.y=0;
                    if(facing.sqrMagnitude>.001f)player.transform.rotation=Quaternion.LookRotation(facing);
                    collision.Arrived();
                    var body=player.GetComponent<Rigidbody>();if(body!=null && !body.isKinematic)body.linearVelocity=Vector3.zero;
                    LastProbe="Sucesso: distancia="+Vector3.Distance(destination,interaction).ToString("F2")+"m; LODs="+collision.EnabledCount;
                    return "Ao alcance do braseiro; ative pelo jogo";
                }
                LastProbe="sem chao="+noGround+", bloqueados="+obstacles+", fora do alcance="+outOfReach+", LODs="+collision.EnabledCount+", mascara="+groundMask+" ("+blocker+")";
                return "Destino recusado: "+LastProbe;
            }
        }
    }
}
