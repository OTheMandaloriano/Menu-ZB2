// Minimal game/Unity doubles. Uses the REAL pinned Harmony implementation.
using System;
using System.Runtime.CompilerServices;
using Zb2Menu;
using UnityEngine;
namespace Zb2Menu {
    internal static class TestEnvironment { public static bool Focused = true; }
    internal static class MenuInputBridge {public static int flags;public static void Install(HarmonyLib.Harmony h){}public static void Update(int value){flags=value;}}
    internal static class MagnetFreeze {public static void Install(HarmonyLib.Harmony h){}}
    internal static class NoClipBridge {public static void Install(HarmonyLib.Harmony h) {}}
    // Range/LOD has its own Harmony integration suite.
    internal static class RangeBridge { public static void Install(HarmonyLib.Harmony harmony) {} }
}
namespace UnityEngine {
    public struct Vector3 {
        public float x,y,z;
        public Vector3(float a,float b,float c) { x=a;y=b;z=c; }
        public float magnitude { get { return (float)Math.Sqrt(x*x+y*y+z*z); } }
        public Vector3 normalized { get { return this/magnitude; } }
        public static Vector3 operator -(Vector3 a,Vector3 b) { return new Vector3(a.x-b.x,a.y-b.y,a.z-b.z); }
        public static Vector3 operator /(Vector3 a,float f) { return new Vector3(a.x/f,a.y/f,a.z/f); }
    }
    public class Transform {
        public Transform parent;
        public Vector3 position,forward = new Vector3(0,0,1);
        public bool IsChildOf(Transform root) { for(var p=parent;p!=null;p=p.parent) if(p==root)return true;return false; }
    }
    public class Collider {
        public Transform transform;
        public ZombieObject owner;
        public T GetComponentInParent<T>() where T:class { return owner as T; }
    }
    public struct RaycastHit { public Collider collider; public float distance; }
    public enum QueryTriggerInteraction { UseGlobal,Ignore,Collide }
    public static class Physics {
        public static RaycastHit[] Results = new RaycastHit[0];
        public static bool Saturated, Fail;
        public static int Queries;
        public static int RaycastNonAlloc(Vector3 from,Vector3 direction,RaycastHit[] hits,float distance,int mask,QueryTriggerInteraction trigger) {
            ++Queries;
            if(Fail) throw new InvalidOperationException("physics unavailable");
            if(Saturated) return hits.Length;
            int count=0;
            foreach(var h in Results) if(h.distance<=distance) hits[count++]=h;
            return count;
        }
    }
    public static class Time { public static float timeScale=1; }
}
public class PlayerMain {
    public bool HasLocalControl=true;
    public float healthFast=100;
    public Transform transform=new Transform();
    public PlayerCamera cam=new PlayerCamera();
    public PlayerArms arms;
}
public class PlayerCamera { public Transform CameraTransform=new Transform(); }
public class ZBMain {
    [MethodImpl(MethodImplOptions.NoInlining)] public void Update() { }
}
public class PlayerArms {
    public PhysicalGun EquippedGun = new PhysicalGun();
    public PlayerMain Owner;
    public ShotPath InputPath;
    public Vector3 Synced;
    public int SyncCount;
    [MethodImpl(MethodImplOptions.NoInlining)]
    public void ShootGun(InventoryItem item) {
        ShotPath path = InputPath;
        --item.ammo;
        EquippedGun.Shoot(Owner, -1, path, true, null);
        SyncShotOnline(path.convergingDirection);
    }
    [MethodImpl(MethodImplOptions.NoInlining)]
    void SyncShotOnline(Vector3 direction) { Synced=direction; ++SyncCount; }
    [MethodImpl(MethodImplOptions.NoInlining)]
    public void ReadFireInput(InventoryItem item) { }
}
public class InventoryItem { public int ammo=10; }
public class ZombieHealth { public bool isAlive=true;public float amount=100; }
public class Zombie { public ZombieObject obj;public ZombieHealth health=new ZombieHealth(); }
public class ZombieObject { public Transform transform=new Transform();public Zombie GetZombie; }
public class MultiplayerController { public static MultiplayerController instance=new MultiplayerController();public bool IsSinglePlayer=true; }
public struct ShotPath {
    public Vector3 bulletOrigin,tracerOrigin,convergingOrigin,convergingDirection;
    public ShotPath(Vector3 b,Vector3 t,Vector3 c,Vector3 d){bulletOrigin=b;tracerOrigin=t;convergingOrigin=c;convergingDirection=d;}
}
public class DatabaseGun { public enum GunClass { Normal,ExplosivesLauncher } public GunClass gunClass; }
public class PhysicalGun {
    public bool IsCoolingDown, Signal;
    public DatabaseGun DbReference=new DatabaseGun();
    public Transform barrel=new Transform();
    public ShotPath Last;
    public int Calls;
    public void SetShootSignal(bool value) { Signal=value; }
    [MethodImpl(MethodImplOptions.NoInlining)]
    public void Shoot(PlayerMain playerShooter,int layerMask,ShotPath shotPath,bool shootForEffect,DatabaseGun customDbGun=null) {
        Last=shotPath;++Calls;
    }
}
static class ManagedAimTests {
    static int checks;
    static void Check(bool value,string text) { ++checks;if(!value)throw new Exception(text); }
    static PlayerMain player;
    static Zombie target;
    static Transform bone;
    static PhysicalGun gun;
    static ShotPath shot;
    static void Publish(int flags) { AimBridge.Publish(player,target,bone,flags,120); }
    static void Fire() { player.arms.Owner=player; player.arms.InputPath=shot; player.arms.ShootGun(new InventoryItem()); }
    static void Main() {
        player=new PlayerMain(); player.arms=new PlayerArms();gun=player.arms.EquippedGun;
        target=new Zombie(); target.obj=new ZombieObject();target.obj.GetZombie=target;
        bone=new Transform {parent=target.obj.transform,position=new Vector3(0,0,-5)};
        var enemy=new Collider {transform=target.obj.transform,owner=target.obj};
        var self=new Collider {transform=player.transform};
        var wall=new Collider {transform=new Transform()};
        shot=new ShotPath(new Vector3(0,0,0),new Vector3(1,0,0),new Vector3(0,0,0),new Vector3(0,0,1));
        Check(AimBridge.Install(),"Harmony install: "+AimBridge.LastError());
        Check(AimBridge.Install(),"install is idempotent");
        int callbacks = 0, callbackThread = 0;
        AimBridge.NativeUpdate update = delegate { ++callbacks; callbackThread=System.Threading.Thread.CurrentThread.ManagedThreadId; };
        int uiFlags=7;AimBridge.NativeUiState ui=delegate{return uiFlags;};
        Check(AimBridge.StartLoop(System.Runtime.InteropServices.Marshal.GetFunctionPointerForDelegate(update),System.Runtime.InteropServices.Marshal.GetFunctionPointerForDelegate(ui)),"register update callback");
        var game = new ZBMain(); game.Update();
        Check(callbacks==1 && callbackThread==System.Threading.Thread.CurrentThread.ManagedThreadId,"runtime callback executes on game update thread");
        Time.timeScale=0;uiFlags=0;game.Update();Check(callbacks==1,"paused update does no game memory work");Check(MenuInputBridge.flags==0,"cursor ownership updates even while simulation paused");Time.timeScale=1;
        Physics.Results=new[]{new RaycastHit{collider=enemy,distance=4},new RaycastHit{collider=self,distance=.2f}};
        Publish(1);Fire();
        Check(gun.Last.convergingDirection.z == -1,"silent redirects real patched Shoot argument to rear enemy");
        Check(player.cam.CameraTransform.forward.z==1,"silent leaves camera orientation intact");
        Check(gun.Last.tracerOrigin.x==1 && gun.Last.bulletOrigin.z==0,"origins preserved");
        Check(gun.Calls==1,"original shot executes exactly once");
        Check(player.arms.SyncCount==1 && player.arms.Synced.z==gun.Last.convergingDirection.z,
              "same ShotPath reaches simulation and original synchronization exactly once");
        Physics.Results=new[]{new RaycastHit{collider=enemy,distance=4},new RaycastHit{collider=wall,distance=2}};
        Publish(1);Fire();Check(gun.Last.convergingDirection.z==1,"nearest unsorted obstacle prevents redirect");
        Physics.Saturated=true;Publish(1);Fire();Check(gun.Last.convergingDirection.z==1,"saturated query prevents redirect");Physics.Saturated=false;
        Physics.Fail=true;Publish(1);Fire();Check(gun.Last.convergingDirection.z==1,"physics exception leaves original shot intact");Physics.Fail=false;
        Physics.Results=new[]{new RaycastHit{collider=enemy,distance=4}};
        MultiplayerController.instance.IsSinglePlayer=false;
        Publish(1);Fire();Check(gun.Last.convergingDirection.z==-1 && player.arms.Synced.z==-1,"online shot and sync share rear direction");
        MultiplayerController.instance.IsSinglePlayer=true;
        target.health.isAlive=false;Publish(1);Fire();Check(gun.Last.convergingDirection.z==1,"dead target not redirected");target.health.isAlive=true;
        Publish(1);gun.Shoot(new PlayerMain(),-1,shot,true);Check(gun.Last.convergingDirection.z==1,"other player shot unmodified");
        var remote = new PlayerMain {HasLocalControl=false};
        remote.arms=new PlayerArms {Owner=remote, InputPath=shot};
        Publish(1);remote.arms.ShootGun(new InventoryItem());
        Check(remote.arms.EquippedGun.Last.convergingDirection.z==1 && remote.arms.Synced.z==1,"remote player pipeline unmodified");
        Publish(1);TestEnvironment.Focused=false;Fire();Check(gun.Last.convergingDirection.z==1,"focus loss clears effect immediately");TestEnvironment.Focused=true;
        Publish(1);AimBridge.Clear();Fire();Check(gun.Last.convergingDirection.z==1,"disable clears published request");
        Publish(1);System.Threading.Thread.Sleep(170);Fire();Check(gun.Last.convergingDirection.z==1,"stale target expires");
        Publish(4);player.arms.ReadFireInput(new InventoryItem());Check(gun.Signal,"trigger activates original weapon signal");
        gun.Signal=false;Physics.Results=new[]{new RaycastHit{collider=wall,distance=1},new RaycastHit{collider=enemy,distance=4}};
        Publish(4);player.arms.ReadFireInput(new InventoryItem());Check(!gun.Signal,"trigger cannot fire through wall");
        Physics.Results=new[]{new RaycastHit{collider=enemy,distance=4}};
        gun.IsCoolingDown=true;Publish(2);player.arms.ReadFireInput(new InventoryItem());Check(!gun.Signal,"autofire respects cooldown");gun.IsCoolingDown=false;
        Publish(2);player.arms.ReadFireInput(new InventoryItem{ammo=0});Check(!gun.Signal,"autofire respects ammo");
        gun.DbReference.gunClass=DatabaseGun.GunClass.ExplosivesLauncher;Publish(3);player.arms.ReadFireInput(new InventoryItem());Check(!gun.Signal,"explosives excluded");gun.DbReference.gunClass=DatabaseGun.GunClass.Normal;
        Publish(3);player.arms.ReadFireInput(new InventoryItem());Check(gun.Signal,"silent autofire can request shot without camera alignment");
        AimBridge.Shutdown();Publish(1);Fire();Check(gun.Last.convergingDirection.z==1,"shutdown removes owned patches");
        game.Update();Check(callbacks==1,"shutdown disables native update callback");
        GC.KeepAlive(update);
        GC.KeepAlive(ui);
        Console.WriteLine("PASS: "+checks+" managed/Harmony regression checks; "+AimBridge.Statistics());
    }
}
