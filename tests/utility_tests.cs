using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using UnityEngine;
using HarmonyLib;
using Zb2Menu;
namespace UnityEngine {
 public struct Vector3 {public float x,y,z;public Vector3(float a,float b,float c){x=a;y=b;z=c;}public static Vector3 zero=new Vector3(),up=new Vector3(0,1,0),down=new Vector3(0,-1,0);public static Vector3 operator+(Vector3 a,Vector3 b){return new Vector3(a.x+b.x,a.y+b.y,a.z+b.z);}public static Vector3 operator-(Vector3 a,Vector3 b){return new Vector3(a.x-b.x,a.y-b.y,a.z-b.z);}public static Vector3 operator*(Vector3 a,float n){return new Vector3(a.x*n,a.y*n,a.z*n);}public static float Distance(Vector3 a,Vector3 b){var d=a-b;return (float)Math.Sqrt(d.x*d.x+d.y*d.y+d.z*d.z);}}
 public class Transform {public Vector3 position,eulerAngles;public Vector3 forward=new Vector3(0,0,1),right=new Vector3(1,0,0);public bool IsChildOf(Transform t){return ReferenceEquals(this,t);}}
 public class GameObject {public Transform transform=new Transform();}
 public class Rigidbody {public bool isKinematic;public Vector3 linearVelocity;}
 public class Collider {public Transform transform=new Transform();}
 public struct RaycastHit {public Vector3 point,normal;}
 public enum QueryTriggerInteraction {Ignore}
 public static class Physics {public static bool ground=true,blocked;public static bool Raycast(Vector3 p,Vector3 d,out RaycastHit hit,float distance,int mask,QueryTriggerInteraction q){hit=new RaycastHit{point=new Vector3(p.x,0,p.z),normal=Vector3.up};return ground;}public static bool CheckCapsule(Vector3 a,Vector3 b,float r,int m,QueryTriggerInteraction q){return blocked;}public static Collider[] OverlapCapsule(Vector3 a,Vector3 b,float r,int m,QueryTriggerInteraction q){return blocked?new[]{new Collider()}:new Collider[0];}}
 public static class Application {public static bool runInBackground;}
 public static class Time {public static int frameCount;public static float unscaledTime;}
 public static class Mathf {public static int Clamp(int v,int a,int b){return Math.Max(a,Math.Min(b,v));}}
}
namespace Zb2Menu {public sealed class NearbyCollisionScope:IDisposable {public NearbyCollisionScope(Vector3 p){}public void Arrived(){}public void Dispose(){}}}
public enum DamageType {Fall,Bullet}
public class Damage {public DamageType damageType;public float amount;}
public class PlayerCamera {public enum Mode {ThirdPerson,FirstPerson}public Mode CurrentMode=Mode.FirstPerson;public float baseCameraFov=70,baseCameraDistance=3.5f;public Transform CameraTransform=new Transform();public void SetCameraMode(Mode m){CurrentMode=m;}}
public class PlayerMovement {public int groundMask=-1;}
public class PlayerMain {
 public enum HealthState {Alive,Dying,Dead}public HealthState healthState;public bool HasLocalControl=true;public float healthFast=100,defaultHeight=1.7f;
 public Transform transform=new Transform();public PlayerCamera cam=new PlayerCamera();public PlayerMovement movement=new PlayerMovement();public Rigidbody body=new Rigidbody();public int revives;
 public T GetComponent<T>() where T:class{return body as T;}
 public void Revive(){healthState=HealthState.Alive;healthFast=30;revives++;}
 [MethodImpl(MethodImplOptions.NoInlining)]public void TakeDamage(Damage damage){healthFast-=damage.amount;}
}
public class PlayersController {public static PlayersController instance=new PlayersController();public PlayerMain player=new PlayerMain();public PlayerMain MyPlayer(){return player;}}
public class ZBMain {public static ZBMain instance=new ZBMain();public bool mapIsLoaded=true;}
public class MultiplayerController {public static MultiplayerController instance=new MultiplayerController();public bool server=true;public bool IsServer(){return server;}public bool IsOnlineServer(){return false;}}
public class DaytimeController {public static DaytimeController instance=new DaytimeController();public float dayDurationInMinutes=10,curTime=12,syncTimer;}
public class InventoryDisplay {public enum SideMenu {AmmoCrafting,ExplosivesCrafting,GunUpgradeTable,ZumbiePyre}}
public class InteractableFurniture {public Transform transform=new Transform();public Vector3 InteractionPoint{get{return transform.position;}}}
public class WorkbenchInteractable:InteractableFurniture {public InventoryDisplay.SideMenu inventoryInteractionID;}
public class PyreInteractable:WorkbenchInteractable {public bool IsLit;}
public class WorkbenchInteractions {public static WorkbenchInteractions instance=new WorkbenchInteractions();readonly List<InteractableFurniture> allWorkbenches=new List<InteractableFurniture>();public void Add(InteractableFurniture v){allWorkbenches.Add(v);}}
public class VendorController {public static VendorController Instance=new VendorController();public bool VanExists=true;public Vector3 SpawnPoint;public GameObject Van=new GameObject();}
public enum ZombieType {Tier1Civilian=0,BossRiot=6,BossQueen=7,BossReaper=8}
public struct Identity {public int id;public ZombieType type;}
public class Health {public bool isAlive=true;}
public class Zombie {public Identity identity;public Health health=new Health();}
public class UnloadedZombie {public Identity identity;}
public class ZombieProp {public Identity identity;}
public class ZombieLoader {public static ZombieLoader Instance=new ZombieLoader();public List<Zombie> zombies=new List<Zombie>();public List<UnloadedZombie> unloadedZombies=new List<UnloadedZombie>();public List<ZombieProp> zombieProps=new List<ZombieProp>();public int loads;public void ForceLoadRealZombie(int id){loads++;}}
public class ZombieController {public static ZombieController instance=new ZombieController();public int created;public UnloadedZombie SpawnBasicZombie(ZombieType t,Vector3 p,bool skip,bool wave){var z=new UnloadedZombie{identity=new Identity{id=++created,type=t}};ZombieLoader.Instance.unloadedZombies.Add(z);return z;}public UnloadedZombie SpawnBoss(ZombieType t,Vector3 p,float angle,bool sync){return SpawnBasicZombie(t,p,true,false);}}
public class Speaker {public void SyncSingleZombie(UnloadedZombie z){}}
public class ServerController {public static ServerController instance=new ServerController();public Speaker GetSpeaker=new Speaker();}
class UtilityTests {
 static int checks,request;
 static void Check(bool b,string n){if(!b)throw new Exception(n);checks++;}
 static string Apply(int flags=0,int command=0,float speed=2,float x=10){if(command!=0)request++;return UtilityBridge.Apply(1,flags,100,6,speed,command,request,x,0,10,18,3,0,IntPtr.Zero);}
 static void Main(){
  var h=new Harmony("utility.tests");UtilityBridge.Install(h);var p=PlayersController.instance.player;
  Apply(1|2|4|16);Check(p.cam.baseCameraFov==100 && p.cam.CurrentMode==PlayerCamera.Mode.ThirdPerson && p.cam.baseCameraDistance==6,"camera overrides");Check(DaytimeController.instance.dayDurationInMinutes==5 && Application.runInBackground,"day and background overrides");
  Apply(1|2|4|16,speed:5);Check(DaytimeController.instance.dayDurationInMinutes==2,"day multiplier based on original");
  Apply();Check(p.cam.baseCameraFov==70 && p.cam.CurrentMode==PlayerCamera.Mode.FirstPerson && p.cam.baseCameraDistance==3.5f && DaytimeController.instance.dayDurationInMinutes==10 && !Application.runInBackground,"all-off exact restoration");
  Apply(8);p.TakeDamage(new Damage{amount=500,damageType=DamageType.Fall});Check(p.healthFast==100,"fall damage blocked");p.TakeDamage(new Damage{amount=10,damageType=DamageType.Bullet});Check(p.healthFast==90,"other damage retained");Apply();p.TakeDamage(new Damage{amount=5,damageType=DamageType.Fall});Check(p.healthFast==85,"fall damage restored");
  Apply(command:1);Check(p.transform.position.x==10,"XYZ teleport");p.transform.position=Vector3.zero;Apply();Check(p.transform.position.x==0,"action consumed once");Physics.ground=false;Apply(command:1);Check(p.transform.position.x==0,"void rejected");Physics.ground=true;Physics.blocked=true;Apply(command:1);Check(p.transform.position.x==0,"wall rejected");Physics.blocked=false;
  Apply(command:7);Check(DaytimeController.instance.curTime==18,"set hour action");MultiplayerController.instance.server=false;DaytimeController.instance.curTime=12;Apply(command:7);Check(DaytimeController.instance.curTime==12,"client cannot set host clock");MultiplayerController.instance.server=true;
  p.healthFast=0;p.healthState=PlayerMain.HealthState.Dying;Apply(command:8);Check(p.revives==1 && p.healthState==PlayerMain.HealthState.Alive,"revive works while downed");Apply(command:8);Check(p.revives==1,"alive not revived again");
  Apply(command:9);Check(ZombieController.instance.created==0,"queue not bulk spawned on click");Apply();Check(ZombieController.instance.created==1,"one spawn per pass");Apply();Check(ZombieController.instance.created==1,"queue throttle");Time.unscaledTime=1;Apply();Check(ZombieController.instance.created==2,"queue advances");Apply(command:10);Time.unscaledTime=2;Apply();Check(ZombieController.instance.created==2,"queue cancel");
  Apply(command:11);Check(ZombieController.instance.created==3,"boss created");Apply(command:11);Check(ZombieController.instance.created==3,"duplicate boss rejected");
  ZBMain.instance.mapIsLoaded=false;Apply(1|2|4,command:1,x:40);ZBMain.instance.mapIsLoaded=true;Apply();Check(p.transform.position.x==0,"loading request not replayed");
  h.UnpatchAll("utility.tests");Console.WriteLine(checks+" utility checks passed");
 }
}
