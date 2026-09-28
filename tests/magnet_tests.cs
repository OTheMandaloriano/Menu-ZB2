using System;
using System.Collections.Generic;
using UnityEngine;
using Zb2Menu;
using System.Runtime.CompilerServices;
public class MultiplayerController {public static MultiplayerController instance=new MultiplayerController();public bool server=true;public bool IsServer(){return server;}}
public struct ZombieIdentity {public int id;public bool IsBoss;}
public class ZombieHealth {public bool isAlive=true;public float amount=100;}
public class ZombieObject {public Rigidbody body=new Rigidbody();public Animator animator=new Animator();public Transform transform=new Transform();}
public enum ZombieState {Aware,Spawning,Transition}
public class Zombie {public ZombieState state,targetState;public int updates;[MethodImpl(MethodImplOptions.NoInlining)]public void UpdateStateMachine(int index){updates++;}[MethodImpl(MethodImplOptions.NoInlining)]public void UpdatePhysicsAndAnimation(){updates++;}[MethodImpl(MethodImplOptions.NoInlining)]public void UpdateBossBehaviour(){updates++;}public ZombieIdentity identity;public ZombieHealth health=new ZombieHealth();public ZombieObject obj=new ZombieObject();public int calls;public void TeleportTo(Vector3 p,Quaternion r){obj.transform.position=p;calls++;}}
public class ZombieLoader {public static ZombieLoader Instance=new ZombieLoader();public List<Zombie> zombies=new List<Zombie>();public List<Zombie> unloadedZombies=new List<Zombie>();public List<Zombie> zombieProps=new List<Zombie>();public void ForceLoadRealZombie(int id){var z=unloadedZombies.Find(v=>v.identity.id==id);if(z!=null){unloadedZombies.Remove(z);zombies.Add(z);}}}
class MagnetTests {
    static int checks;
    static void Check(bool b,string n){if(!b)throw new Exception(n);checks++;}
    static int Calls(){int n=0;foreach(var z in ZombieLoader.Instance.zombies)n+=z.calls;return n;}
    static void Main(){
        var p=new PlayerMain();for(int i=0;i<10;i++){var z=new Zombie{identity=new ZombieIdentity{id=i}};z.obj.transform.position=new Vector3(0,0,20+i);ZombieLoader.Instance.zombies.Add(z);}
        MultiplayerController.instance.server=false;MagnetBridge.Apply(p,true,true,50);Check(Calls()==0 && MagnetBridge.Status.Contains("host"),"client cannot move server zombies");
        MultiplayerController.instance.server=true;MagnetBridge.Apply(p,true,false,50);Check(Calls()==0,"menu blocks pull");
        MagnetBridge.Apply(p,true,true,50);Check(Calls()==4,"four moves per batch");MagnetBridge.Apply(p,true,true,50);Check(Calls()==4,"throttle");
        Time.unscaledTime=1;MagnetBridge.Apply(p,true,true,50);Check(Calls()==8,"next batch");Time.unscaledTime=2;MagnetBridge.Apply(p,true,true,50);Check(Calls()==10,"loaded eligible enemies gathered");
        Time.unscaledTime=3;MagnetBridge.Apply(p,true,true,50);Check(Calls()==10,"no repeated teleports");
        ZombieLoader.Instance.zombies[0].obj.transform.position=new Vector3(0,0,40);Time.unscaledTime=3.2f;MagnetBridge.Apply(p,true,true,50);Check(Calls()==10,"AI movement does not cause repeated teleport loop");
        p.transform.position=new Vector3(0,0,10);Time.unscaledTime=3.5f;MagnetBridge.Apply(p,true,true,50);Check(Calls()==14,"anchor follows moving player continuously");
        MagnetBridge.Apply(p,false,true,50);Check(!MagnetBridge.Active,"disable clears activation");
        Physics.ground=false;Time.unscaledTime=4;MagnetBridge.Apply(p,true,true,50);Check(Calls()==14,"no unsafe destination without ground");Physics.ground=true;
        Physics.blocked=true;Time.unscaledTime=5;MagnetBridge.Apply(p,true,true,50);Check(Calls()==14,"occupied destination rejected");Physics.blocked=false;
        p.healthFast=0;Time.unscaledTime=6;MagnetBridge.Apply(p,true,true,50);Check(!MagnetBridge.Active && Calls()==14,"dead player clears magnet");
        p.healthFast=100;ZombieLoader.Instance=new ZombieLoader();var boss=new Zombie{identity=new ZombieIdentity{id=100,IsBoss=true}};boss.obj.transform.position=new Vector3(0,0,900);ZombieLoader.Instance.unloadedZombies.Add(boss);
        var normal=new Zombie{identity=new ZombieIdentity{id=101}};normal.obj.transform.position=new Vector3(0,0,900);ZombieLoader.Instance.zombies.Add(normal);
        Time.unscaledTime=7;MagnetBridge.Apply(p,true,true,10,2);Check(boss.calls==1 && normal.calls==0,"boss only loads distant existing boss and ignores source radius");
        Check(boss.obj.transform.position.z>=p.transform.position.z+8,"boss landing is separated from normal anchor");
        MagnetBridge.Reset();boss.obj.transform.position=new Vector3(0,0,900);normal.obj.transform.position=new Vector3(0,0,15);Time.unscaledTime=8;MagnetBridge.Apply(p,true,true,50,1);Check(boss.calls==1 && normal.calls==1,"zombies only excludes boss");
        var harmony=new HarmonyLib.Harmony("magnet.freeze.tests");MagnetFreeze.Install(harmony);MagnetFreeze.Hold(normal);normal.UpdateStateMachine(0);normal.UpdatePhysicsAndAnimation();normal.UpdateBossBehaviour();Check(normal.updates==0 && normal.obj.body.isKinematic && normal.obj.animator.speed==0,"freeze suspends updates physics and animation");
        MagnetFreeze.Clear();normal.UpdateStateMachine(0);Check(normal.updates==1 && !normal.obj.body.isKinematic && normal.obj.animator.speed==1,"freeze restores exact body and animation state");
        normal.state=ZombieState.Spawning;MagnetFreeze.Hold(normal);normal.UpdateStateMachine(0);Check(normal.updates==2 && normal.obj.animator.speed==1 && !normal.obj.body.isKinematic,"spawn animation and physics remain active");
        normal.state=ZombieState.Transition;normal.targetState=ZombieState.Spawning;MagnetFreeze.Hold(normal);normal.UpdateStateMachine(0);Check(normal.updates==3 && normal.obj.animator.speed==1,"transition into spawn is never frozen");
        normal.state=normal.targetState=ZombieState.Aware;MagnetFreeze.Hold(normal);normal.UpdateStateMachine(0);Check(normal.updates==3 && normal.obj.animator.speed==0,"freeze applies after spawn finishes");
        normal.state=ZombieState.Spawning;normal.UpdateStateMachine(0);Check(normal.updates==4 && normal.obj.animator.speed==1,"new spawn releases previously frozen entity");
        MagnetFreeze.Clear();harmony.UnpatchAll("magnet.freeze.tests");
        MagnetBridge.Reset();p.healthFast=100;p.transform.position=new Vector3();p.cam.CameraTransform.position=new Vector3(0,3,20);normal.state=normal.targetState=ZombieState.Aware;
        ZombieLoader.Instance=new ZombieLoader();normal.obj.transform.position=new Vector3(0,0,40);ZombieLoader.Instance.zombies.Add(normal);
        Time.unscaledTime=10;MagnetBridge.Apply(p,true,true,100,1,2.5f,8,true,1);int fixedCalls=normal.calls;float fixedZ=normal.obj.transform.position.z;
        p.transform.position=new Vector3(0,0,10);p.cam.CameraTransform.position=new Vector3(0,3,60);Time.unscaledTime=11;MagnetBridge.Apply(p,true,true,100,1,2.5f,8,true,1);
        Check(normal.calls==fixedCalls && normal.obj.transform.position.z==fixedZ,"fixed anchor does not follow movement or view");
        MagnetBridge.Reset();Time.unscaledTime=12;MagnetBridge.Apply(p,true,true,100,1,2.5f,8,true,1);Check(normal.calls>fixedCalls,"reactivation captures a new point");MagnetBridge.Reset();
        p.transform.position=new Vector3();p.cam.CameraTransform.position=new Vector3(0,3,20);
        normal.obj.transform.position=new Vector3(0,0,40);Time.unscaledTime=13;
        MagnetBridge.Apply(p,true,true,100,1,2.5f,8,false,1);
        Check(normal.obj.body.isKinematic && normal.obj.animator.speed==0,"fixed point retains without optional freeze checkbox");
        MagnetBridge.Apply(p,false,true,100,1,2.5f,8,false,1);
        Check(!normal.obj.body.isKinematic && normal.obj.animator.speed==1,"disabling fixed point restores body and animation");
        normal.obj.transform.position=new Vector3(0,0,40);Time.unscaledTime=14;
        MagnetBridge.Apply(p,true,true,100,1,2.5f,8,false,2);float nearZ=normal.obj.transform.position.z;int nearCalls=normal.calls;
        Check(nearZ<10 && normal.obj.body.isKinematic,"fixed front captures nearby destination rather than view hit");
        p.transform.position=new Vector3(0,0,30);Time.unscaledTime=15;MagnetBridge.Apply(p,true,true,100,1,2.5f,8,false,2);
        Check(normal.calls==nearCalls && normal.obj.transform.position.z==nearZ,"fixed front stays after player moves");
        Time.unscaledTime=16;MagnetBridge.Apply(p,true,true,100,1,2.5f,8,false,0);
        Check(!normal.obj.body.isKinematic && normal.obj.animator.speed==1,"switching to follow releases automatic fixed freeze");
        MagnetBridge.Reset();normal.state=ZombieState.Spawning;normal.obj.transform.position=new Vector3(0,0,50);int spawnCalls=normal.calls;
        Time.unscaledTime=17;MagnetBridge.Apply(p,true,true,100,1,2.5f,8,false,2);
        Check(normal.calls==spawnCalls && !normal.obj.body.isKinematic,"fixed mode waits for spawn before moving or freezing");
        normal.state=ZombieState.Aware;Time.unscaledTime=18;MagnetBridge.Apply(p,true,true,100,1,2.5f,8,false,2);
        Check(normal.calls>spawnCalls && normal.obj.body.isKinematic,"fixed mode collects after spawn finishes");
        MagnetBridge.Reset();Physics.ground=false;Time.unscaledTime=19;int beforeInvalid=normal.calls;
        MagnetBridge.Apply(p,true,true,100,1,2.5f,8,false,2);
        Check(normal.calls==beforeInvalid && MagnetBridge.Status.Contains("chao seguro"),"fixed front rejects absent ground");
        Physics.ground=true;MagnetBridge.Reset();
        Console.WriteLine(checks+" magnet checks passed");
    }
}
