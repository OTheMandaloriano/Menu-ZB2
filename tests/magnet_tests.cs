using System;
using System.Collections.Generic;
using UnityEngine;
using Zb2Menu;
using System.Runtime.CompilerServices;
public class MultiplayerController {public static MultiplayerController instance=new MultiplayerController();public bool server=true;public bool IsServer(){return server;}}
public struct ZombieIdentity {public int id;public bool IsBoss;}
public class ZombieHealth {public bool isAlive=true;public float amount=100;}
public class ZombieObject {public Rigidbody body=new Rigidbody();public Animator animator=new Animator();public Transform transform=new Transform();}
public class Zombie {public int updates;[MethodImpl(MethodImplOptions.NoInlining)]public void UpdateStateMachine(int index){updates++;}[MethodImpl(MethodImplOptions.NoInlining)]public void UpdatePhysicsAndAnimation(){updates++;}[MethodImpl(MethodImplOptions.NoInlining)]public void UpdateBossBehaviour(){updates++;}public ZombieIdentity identity;public ZombieHealth health=new ZombieHealth();public ZombieObject obj=new ZombieObject();public int calls;public void TeleportTo(Vector3 p,Quaternion r){obj.transform.position=p;calls++;}}
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
        MagnetFreeze.Clear();normal.UpdateStateMachine(0);Check(normal.updates==1 && !normal.obj.body.isKinematic && normal.obj.animator.speed==1,"freeze restores exact body and animation state");harmony.UnpatchAll("magnet.freeze.tests");
        Console.WriteLine(checks+" magnet checks passed");
    }
}
