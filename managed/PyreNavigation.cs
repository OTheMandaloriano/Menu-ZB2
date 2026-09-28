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
        static string Teleport(PlayerMain player,PyreInteractable target) {
            if(!player.HasLocalControl || player.healthFast<=0)return "Jogador local indisponivel";
            using(var collision=new NearbyCollisionScope(target.transform.position)) {
            // Candidate destinations are beside the prop, never inside its origin.
            for(int i=0;i<24;++i){
                float angle=(i%8)*(float)Math.PI/4;
                var offset=new Vector3((float)Math.Sin(angle),0,(float)Math.Cos(angle))*(1.2f+(i/8)*.7f);
                RaycastHit hit;
                if(!Physics.Raycast(target.InteractionPoint+offset+Vector3.up*1.5f,Vector3.down,out hit,5,~0,QueryTriggerInteraction.Ignore) || hit.normal.y<.7f)continue;
                float height=player.defaultHeight;
                if(!(height>.5f && height<4))return "Altura do jogador invalida";
                float radius=Math.Min(.35f,height*.25f);
                var destination=hit.point+Vector3.up*(height*.5f+.08f);
                bool blocked=false;
                foreach(var collider in Physics.OverlapCapsule(hit.point+Vector3.up*(radius+.08f),hit.point+Vector3.up*(height-radius+.08f),radius,~0,QueryTriggerInteraction.Ignore))
                    if(collider!=null && !collider.transform.IsChildOf(player.transform)){blocked=true;break;}
                if(blocked)continue;
                player.transform.position=destination;collision.Arrived();
                var body=player.GetComponent<Rigidbody>();if(body!=null && !body.isKinematic)body.linearVelocity=Vector3.zero;
                return "Teleportado ao lado do braseiro; ative pelo jogo";
            }
            return "Sem destino livre e com chao junto ao braseiro";
            }
        }
    }
}
