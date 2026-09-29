using UnityEngine;
namespace Zb2Menu {
    public static class UtilitySpawnQueue {
        static int remaining,index;
        static float next;
        static Vector3 origin,forward,right;
        static ZombieLoader owner;
        public static string Status="";
        public static void Clear(){remaining=0;owner=null;Status="";}
        public static string Enqueue(PlayerMain player,int count,int boss) {
            if(remaining>0)return "Aguarde ou cancele a fila atual";
            if(ZombieController.instance==null || ZombieLoader.Instance==null)return "Carregador indisponivel";
            var view=player.cam.CameraTransform;RaycastHit hit;
            if(view==null || !Physics.Raycast(view.position,view.forward,out hit,50,player.movement.groundMask,QueryTriggerInteraction.Ignore) || hit.normal.y<.7f)return "Mire em chao livre a menos de 50 m";
            if(ZombieLoader.Instance.zombies.Count>=256)return "Limite de 256 modelos ativos atingido";
            if(boss>=0){
                if(boss>2)return "Boss invalido";
                var type=(ZombieType)(6+boss);var loader=ZombieLoader.Instance;
                foreach(var z in loader.zombies)if(z!=null && z.identity.type==type && (z.health==null || z.health.isAlive))return "Esse boss ja existe; use o Magnet";
                foreach(var z in loader.unloadedZombies)if(z!=null && z.identity.type==type)return "Esse boss ja existe; use o Magnet";
                foreach(var z in loader.zombieProps)if(z!=null && z.identity.type==type)return "Esse boss ja existe; use o Magnet";
                if(Physics.CheckCapsule(hit.point+Vector3.up*1.6f,hit.point+Vector3.up*4,1.5f,player.movement.groundMask,QueryTriggerInteraction.Ignore))return "Sem espaco para o boss";
                var created=ZombieController.instance.SpawnBoss(type,hit.point,player.transform.eulerAngles.y,MultiplayerController.instance.IsOnlineServer());
                if(created==null)return "Jogo recusou o boss";
                loader.ForceLoadRealZombie(created.identity.id);return "Boss criado pelo jogo";
            }
            remaining=Mathf.Clamp(count,1,100);index=0;origin=hit.point;forward=player.transform.forward;right=player.transform.right;owner=ZombieLoader.Instance;next=0;
            return "Fila criada: "+remaining+" zumbis";
        }
        public static void Tick(PlayerMain player,bool host) {
            if(!host || player==null || player.healthFast<=0 || owner!=ZombieLoader.Instance){Clear();return;}
            if(remaining<=0 || Time.unscaledTime<next)return;
            if(owner.zombies.Count>=256){Clear();Status="Fila encerrada: 256 modelos ativos";return;}
            next=Time.unscaledTime+.25f;
            var point=origin+right*((index%5-2)*1.5f)+forward*((index/5)*1.5f);RaycastHit hit;
            ++index;--remaining;
            if(!Physics.Raycast(point+Vector3.up*2,Vector3.down,out hit,5,player.movement.groundMask,QueryTriggerInteraction.Ignore) || hit.normal.y<.7f){Status="Posicao da fila sem chao; restantes="+remaining;return;}
            if(Physics.CheckCapsule(hit.point+Vector3.up*.4f,hit.point+Vector3.up*1.5f,.35f,player.movement.groundMask,QueryTriggerInteraction.Ignore)){Status="Posicao ocupada; restantes="+remaining;return;}
            var created=ZombieController.instance.SpawnBasicZombie((ZombieType)0,hit.point,false,false);
            if(created==null){Status="Criacao recusada pelo jogo; restantes="+remaining;return;}
            if(MultiplayerController.instance.IsOnlineServer())ServerController.instance.GetSpeaker.SyncSingleZombie(created);
            owner.ForceLoadRealZombie(created.identity.id);Status="Zumbi criado; restantes="+remaining;
        }
    }
}
