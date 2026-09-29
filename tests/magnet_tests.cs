using System;
using System.Collections.Generic;
using UnityEngine;
using Zb2Menu;
using System.Runtime.CompilerServices;
public class MultiplayerController {public static MultiplayerController instance=new MultiplayerController();public bool server=true;public bool IsServer(){return server;}}
public struct ZombieIdentity {public int id;public bool IsBoss;}
public class ZombieHealth {public bool isAlive=true;public float amount=100;}
public class ZombieDoll {}
public class ZombieObject {public Zombie GetZombie;public Transform zombieFootRef,zombieEyeRef;public Rigidbody body=new Rigidbody();public Animator animator=new Animator();public Transform transform=new Transform();}
public enum ZombieState {Aware,Spawning,Transition}
public class Zombie {public Transform transform {get{return obj.transform;}}public ZombieState state,targetState;public int updates;[MethodImpl(MethodImplOptions.NoInlining)]public void UpdateStateMachine(int index){updates++;}[MethodImpl(MethodImplOptions.NoInlining)]public void UpdatePhysicsAndAnimation(){updates++;}[MethodImpl(MethodImplOptions.NoInlining)]public void UpdateBossBehaviour(){updates++;}public ZombieIdentity identity;public ZombieHealth health=new ZombieHealth();public ZombieObject obj=new ZombieObject();public int calls;public void TeleportTo(Vector3 p,Quaternion r){obj.transform.position=p;calls++;}}
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
        ZombieLoader.Instance.zombies[0].obj.transform.position=new Vector3(0,0,40);Time.unscaledTime=3.2f;MagnetBridge.Apply(p,true,true,50);Check(Calls()==11,"distributed mode reacquires an enemy that walked away");
        p.transform.position=new Vector3(0,0,10);Time.unscaledTime=3.5f;MagnetBridge.Apply(p,true,true,50);Check(Calls()==15,"anchor follows moving player continuously");
        MagnetBridge.Apply(p,false,true,50);Check(!MagnetBridge.Active,"disable clears activation");
        Physics.ground=false;Time.unscaledTime=4;MagnetBridge.Apply(p,true,true,50);Check(Calls()==15,"no unsafe destination without ground");Physics.ground=true;
        Physics.blocked=true;Time.unscaledTime=5;MagnetBridge.Apply(p,true,true,50);Check(Calls()==15,"occupied destination rejected");Physics.blocked=false;
        p.healthFast=0;Time.unscaledTime=6;MagnetBridge.Apply(p,true,true,50);Check(!MagnetBridge.Active && Calls()==15,"dead player clears magnet");
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
        MagnetFreeze.Clear();
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
        ZombieLoader.Instance=new ZombieLoader();p.transform.position=new Vector3();
        Zombie one=new Zombie{identity=new ZombieIdentity{id=301}},two=new Zombie{identity=new ZombieIdentity{id=302}};
        one.obj.transform.position=new Vector3(0,0,30);two.obj.transform.position=new Vector3(5,0,30);
        ZombieLoader.Instance.zombies.Add(one);ZombieLoader.Instance.zombies.Add(two);Time.unscaledTime=20;
        MagnetBridge.Apply(p,true,true,100,1,2.5f,8,false,4);
        Check(Vector3.Distance(one.obj.transform.position,two.obj.transform.position)<.001f,"stack mode uses one exact destination");
        Check(one.obj.body.isKinematic && two.obj.body.isKinematic,"stack holds bodies without disabling hit colliders");
        one.obj.GetZombie=one;two.obj.GetZombie=two;
        Check(MagnetFreeze.ShareAnchor(one,new Collider{zombie=two.obj}),"overlapping retained group can expose the common aim point");
        Check(!MagnetFreeze.ShareAnchor(one,new Collider()),"wall does not count as shared magnet group");
        float previousZ=one.obj.transform.position.z;p.transform.position=new Vector3(0,0,2);Time.unscaledTime=21;
        MagnetBridge.Apply(p,true,true,100,1,2.5f,8,false,4);
        Check(one.obj.transform.position.z>previousZ && Vector3.Distance(one.obj.transform.position,two.obj.transform.position)<.001f,"stack follows while retaining common point");
        Vector3 retained=one.obj.transform.position;one.obj.transform.position=new Vector3(99,50,99);one.obj.body.isKinematic=false;one.obj.animator.applyRootMotion=true;
        MagnetFreeze.Pulse();Check(Vector3.Distance(one.obj.transform.position,retained)<.001f && one.obj.body.isKinematic && !one.obj.animator.applyRootMotion,"external movement and root motion are corrected at the retained anchor");
        one.obj.body=new Rigidbody();MagnetFreeze.Pulse();Check(one.obj.body.isKinematic && Vector3.Distance(one.obj.transform.position,retained)<.001f,"replacement rigidbody receives retained anchor");
        Time.unscaledTime+=3;Time.frameCount++;one.UpdateStateMachine(0);Check(one.obj.body.isKinematic,"single stalled frame does not release an overlapping crowd");
        Time.frameCount+=61;one.UpdateStateMachine(0);Check(!one.obj.body.isKinematic,"missing heartbeat across many frames releases ownership");
        MagnetBridge.Reset();Check(!one.obj.body.isKinematic && !two.obj.body.isKinematic,"stack releases physics on disable");
        ZombieLoader.Instance=new ZombieLoader();var probe=new Zombie{identity=new ZombieIdentity{id=400}};
        probe.obj.transform.position=new Vector3(0,2,30);probe.obj.zombieFootRef=new Transform{position=new Vector3(0,0,30)};
        ZombieLoader.Instance.zombies.Add(probe);p.transform.position=new Vector3();Time.unscaledTime=40;
        Physics.RayResults=new[]{new RaycastHit{point=new Vector3(0,2,2.5f),normal=Vector3.up,distance=1,collider=new Collider{zombie=new ZombieDoll()}},new RaycastHit{point=new Vector3(0,0,2.5f),normal=Vector3.up,distance=3,collider=new Collider()}};
        MagnetBridge.Apply(p,true,true,100,1,2.5f,8,false,4);
        Check(Math.Abs(probe.obj.transform.position.y-2.08f)<.001f,"uses real foot offset and rejects corpse as ground");
        MagnetBridge.Reset();Physics.RayResults=null;Physics.ground=false;probe.obj.transform.position=new Vector3(0,0,30);probe.obj.zombieEyeRef=new Transform{position=new Vector3(0,1.5f,30)};
        p.cam.CameraTransform.position=new Vector3(0,8,0);Time.unscaledTime=41;
        MagnetBridge.Apply(p,true,true,100,1,2.5f,8,false,4,true);
        Check(Math.Abs(probe.obj.transform.position.y-6.5f)<.001f && probe.obj.body.isKinematic,"noclip follow aligns eye to camera point without dropping to floor");
        MagnetBridge.Reset();Physics.ground=true;
        MagnetBridge.Reset();ZombieLoader.Instance=new ZombieLoader();p.transform.position=new Vector3();p.cam.CameraTransform.position=new Vector3(0,3,20);
        var carried=new Zombie{identity=new ZombieIdentity{id=600}};carried.obj.transform.position=new Vector3(0,0,20);ZombieLoader.Instance.zombies.Add(carried);
        Time.unscaledTime=50;MagnetBridge.Apply(p,true,true,30,1,2.5f,8,false,5);int prior=carried.calls;
        MagnetBridge.Apply(p,false,true,30);ZombieLoader.Instance.zombies.Remove(carried);ZombieLoader.Instance.unloadedZombies.Add(carried);p.transform.position=new Vector3(0,0,200);p.cam.CameraTransform.position=new Vector3(0,3,210);
        Time.unscaledTime=51;MagnetBridge.Apply(p,true,true,30,1,2.5f,8,false,5);
        Check(carried.calls>prior && carried.obj.transform.position.z>200,"reactivation at B recalls previous cohort outside capture radius");
        MagnetBridge.Reset();
        MagnetBridge.Reset();ZombieLoader.Instance=new ZombieLoader();p.transform.position=new Vector3();
        var dormant=new Zombie{identity=new ZombieIdentity{id=700}};dormant.obj.transform.position=new Vector3(0,0,70);ZombieLoader.Instance.unloadedZombies.Add(dormant);
        Time.unscaledTime=60;MagnetBridge.Apply(p,true,true,60,1);Check(dormant.calls==0,"dormant enemy beyond radius not promoted");
        Time.unscaledTime=61;MagnetBridge.Apply(p,true,true,300,1);Check(dormant.calls==1,"ordinary dormant enemy inside configured radius promoted and gathered");
        MagnetBridge.Reset();
        MagnetBridge.Reset();p.transform.position=new Vector3();p.cam.CameraTransform.position=new Vector3(0,3,20);Physics.RayResults=null;Physics.ground=true;
        ZombieLoader.Instance=new ZombieLoader();var held=new Zombie{identity=new ZombieIdentity{id=900}};held.obj.transform.position=new Vector3(0,0,10);ZombieLoader.Instance.zombies.Add(held);
        Time.unscaledTime=80;MagnetBridge.Apply(p,true,true,300,1,2.5f,8,false,5,false);var original=held.obj.transform.position;int calls=held.calls;
        p.cam.CameraTransform.position=new Vector3(0,20,100);Time.unscaledTime=81;MagnetBridge.Apply(p,true,true,300,1,2.5f,8,false,5,true);
        Check(Vector3.Distance(original,held.obj.transform.position)<.001f && held.calls==calls,"NoClip toggle preserves fixed ground anchor");
        Time.unscaledTime=82;MagnetBridge.Apply(p,true,true,300,1,2.5f,8,false,5,false);
        Check(Vector3.Distance(original,held.obj.transform.position)<.001f,"NoClip disable does not recapture anchor");MagnetBridge.Reset();
        harmony.UnpatchAll("magnet.freeze.tests");
        Console.WriteLine(checks+" magnet checks passed");
    }
}
