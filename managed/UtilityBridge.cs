using System;
using System.Runtime.InteropServices;
using HarmonyLib;
using UnityEngine;
namespace Zb2Menu {
    public static class UtilityBridge {
        static readonly OriginalValues originals=new OriginalValues();
        static readonly object application=new object();
        static PlayerMain protectedPlayer;
        static int heartbeat;
        static int lastRequest;
        static string status="";
        static bool wasInMap;static float statusUntil;
        public static void Install(Harmony harmony){harmony.Patch(AccessTools.Method(typeof(PlayerMain),"TakeDamage",new[]{typeof(Damage)}),prefix:new HarmonyMethod(typeof(UtilityBridge),"AllowDamage"));}
        static bool AllowDamage(PlayerMain __instance,Damage damage){return Time.frameCount-heartbeat>60 || !ReferenceEquals(protectedPlayer,__instance) || !__instance.HasLocalControl || damage==null || damage.damageType!=DamageType.Fall;}
        static float Limit(float value,float min,float max){return float.IsNaN(value)||float.IsInfinity(value)?min:Math.Max(min,Math.Min(max,value));}
        public static string Apply(int ready,int flags,float fov,float distance,float speed,int command,int request,float x,float y,float z,float hour,int count,int boss,IntPtr position) {
            bool action=request!=lastRequest;lastRequest=request;
            var player=PlayersController.instance==null?null:PlayersController.instance.MyPlayer();
            bool inMap=ZBMain.instance!=null && ZBMain.instance.mapIsLoaded && player!=null && player.HasLocalControl;
            if(inMap && !wasInMap)status="";wasInMap=inMap;
            bool host=inMap && MultiplayerController.instance!=null && MultiplayerController.instance.IsServer();
            if(inMap && position!=IntPtr.Zero){var p=player.transform.position;Marshal.Copy(new[]{p.x,p.y,p.z},0,position,3);}
            protectedPlayer=inMap && (flags&8)!=0?player:null;heartbeat=Time.frameCount;
            originals.Begin();
            try{Configure(inMap?player:null,host,flags,fov,distance,speed);}finally{originals.End();}
            UtilitySpawnQueue.Tick(inMap?player:null,host);
            if(!inMap){UtilitySpawnQueue.Clear();status="Entre no mapa";return status;}
            if(!action)return Time.unscaledTime<statusUntil?status:UtilitySpawnQueue.Status.Length>0?UtilitySpawnQueue.Status:status;
            statusUntil=Time.unscaledTime+4;
            if(command==8){
                if(player.healthState!=PlayerMain.HealthState.Dying)return status="Reviver so atua quando caido";
                player.Revive();return status="Rotina de reviver executada";
            }
            if(player.healthFast<=0)return status="Jogador precisa estar vivo";
            if(command==1)return status=UtilityDestinations.Teleport(player,new Vector3(x,y,z));
            if(command>=2 && command<=5){var kinds=new[]{InventoryDisplay.SideMenu.AmmoCrafting,InventoryDisplay.SideMenu.ExplosivesCrafting,InventoryDisplay.SideMenu.GunUpgradeTable,InventoryDisplay.SideMenu.ZumbiePyre};return status=UtilityDestinations.Workbench(player,kinds[command-2]);}
            if(command==6){var vendor=VendorController.Instance;if(vendor==null || !vendor.VanExists)return status="Mercador indisponivel";return status=UtilityDestinations.Teleport(player,vendor.SpawnPoint-vendor.Van.transform.right*4);}
            if(command==10){UtilitySpawnQueue.Clear();UtilitySpawnQueue.Status="";return status="Fila cancelada";}
            if(!host)return status="Acao requer solo/host";
            if(command==7){var day=DaytimeController.instance;if(day==null)return status="Relogio indisponivel";day.curTime=Limit(hour,0,23.99f);day.syncTimer=1.1f;return status="Hora aplicada; o ciclo continua";}
            if(command==9 || command==11){UtilitySpawnQueue.Status="";return status=UtilitySpawnQueue.Enqueue(player,count,command==11?boss:-1);}
            return status;
        }
        static void Configure(PlayerMain player,bool host,int flags,float fov,float distance,float speed) {
            if(player==null)return;
            var cam=player.cam;
            if(cam!=null){
                if((flags&1)!=0)originals.Apply(cam,"fov",()=>cam.baseCameraFov,v=>{if(cam!=null)cam.baseCameraFov=v;},v=>Limit(fov,60,120));
                if((flags&2)!=0){
                    originals.Apply(cam,"mode",()=>cam.CurrentMode,v=>{if(cam!=null)cam.SetCameraMode(v);},v=>PlayerCamera.Mode.ThirdPerson);
                    originals.Apply(cam,"distance",()=>cam.baseCameraDistance,v=>{if(cam!=null)cam.baseCameraDistance=v;},v=>Limit(distance,1,10));
                }
            }
            var day=DaytimeController.instance;
            if(host && day!=null && (flags&4)!=0)originals.Apply(day,"duration",()=>day.dayDurationInMinutes,v=>{if(day!=null)day.dayDurationInMinutes=v;},v=>v/Limit(speed,1,10));
            if((flags&16)!=0)originals.Apply(application,"background",()=>Application.runInBackground,v=>Application.runInBackground=v,v=>true);
        }
    }
}
