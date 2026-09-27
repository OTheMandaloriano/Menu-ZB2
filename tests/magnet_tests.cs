using System;
using System.Collections.Generic;
using UnityEngine;
using Zb2Menu;
public class MultiplayerController {public static MultiplayerController instance=new MultiplayerController();public bool server=true;public bool IsServer(){return server;}}
public struct ZombieIdentity {public int id;}
public class ZombieHealth {public bool isAlive=true;public float amount=100;}
public class ZombieObject {public Transform transform=new Transform();}
public class Zombie {public ZombieIdentity identity;public ZombieHealth health=new ZombieHealth();public ZombieObject obj=new ZombieObject();public int calls;public void TeleportTo(Vector3 p,Quaternion r){obj.transform.position=p;calls++;}}
public class ZombieLoader {public static ZombieLoader Instance=new ZombieLoader();public List<Zombie> zombies=new List<Zombie>();}
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
        MagnetBridge.Apply(p,false,true,50);Check(!MagnetBridge.Active,"disable clears activation");
        Physics.ground=false;Time.unscaledTime=4;MagnetBridge.Apply(p,true,true,50);Check(Calls()==10,"no unsafe destination without ground");Physics.ground=true;
        Physics.blocked=true;Time.unscaledTime=5;MagnetBridge.Apply(p,true,true,50);Check(Calls()==10,"occupied destination rejected");Physics.blocked=false;
        p.healthFast=0;Time.unscaledTime=6;MagnetBridge.Apply(p,true,true,50);Check(!MagnetBridge.Active && Calls()==10,"dead player clears magnet");
        Console.WriteLine(checks+" magnet checks passed");
    }
}
