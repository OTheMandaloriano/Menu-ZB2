using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using UnityEngine;
using Zb2Menu;
using HarmonyLib;
public struct ZombieIdentity {public int id,type;}
public struct ZombieTransform {public Vector3 position;}
public class UnloadedZombie {public ZombieIdentity identity;public ZombieTransform transform;public bool isWaveZombie;}
public class ZombieProp {public ZombieIdentity identity;public Transform transform=new Transform();public bool isWaveZombie;}
public class ZombieObject {public Transform transform=new Transform();}
public class Zombie {public ZombieHealth health=new ZombieHealth();public ZombieIdentity identity;public ZombieObject obj=new ZombieObject();public bool isWaveZombie;}
public class ZombieLoader {
    public enum ZombieLoadState {Unloaded,Prop,Real}
    public static ZombieLoader Instance;
    public List<UnloadedZombie> unloadedZombies=new List<UnloadedZombie>();
    public List<ZombieProp> zombieProps=new List<ZombieProp>();
    public List<Zombie> zombies=new List<Zombie>();
    public int Calls;
    public float sleepDistance=75;
    public List<ZombieLoadState> results=new List<ZombieLoadState>();
    public bool LastActive;
    [MethodImpl(MethodImplOptions.NoInlining)]
    public ZombieLoadState GetTargetZombieState(ZombieLoadState current,int index,Zombie real,bool wave,out bool active) {
        ++Calls;active=wave;return wave?ZombieLoadState.Real:ZombieLoadState.Unloaded;
    }
    [MethodImpl(MethodImplOptions.NoInlining)]
    public void UpdateLoadState() {
        results.Clear();Calls=0;bool active;
        for(int i=0;i<unloadedZombies.Count;++i){results.Add(GetTargetZombieState(ZombieLoadState.Unloaded,i,null,unloadedZombies[i].isWaveZombie,out active));LastActive=active;}
        for(int j=0;j<zombieProps.Count;++j){results.Add(GetTargetZombieState(ZombieLoadState.Prop,unloadedZombies.Count+j,null,zombieProps[j].isWaveZombie,out active));LastActive=active;}
        for(int k=0;k<zombies.Count;++k){results.Add(GetTargetZombieState(ZombieLoadState.Real,unloadedZombies.Count+zombieProps.Count+k,zombies[k],zombies[k].isWaveZombie,out active));LastActive=active;}
    }
}
class RangeTests {
    static int checks;
    static void Check(bool ok,string label){if(!ok)throw new Exception(label+" "+RangeBridge.LastError());++checks;}
    static int Real(ZombieLoader loader){return loader.results.FindAll(s=>s==ZombieLoader.ZombieLoadState.Real).Count;}
    static void Main(){
        var harmony=new Harmony("range.tests");RangeBridge.Install(harmony);
        var loader=new ZombieLoader();ZombieLoader.Instance=loader;
        for(int i=0;i<12;++i)loader.unloadedZombies.Add(new UnloadedZombie {identity=new ZombieIdentity{id=i},transform=new ZombieTransform{position=new Vector3(0,0,100+i)}});
        RangeBridge.Configure(0,200,0,360,800,600);loader.UpdateLoadState();Check(Real(loader)==0,"disabled uses normal LOD");
        RangeBridge.Configure(1,50,0,360,800,600);loader.UpdateLoadState();Check(Real(loader)==0,"below range not loaded");
        RangeBridge.Configure(1,200,0,360,800,600);loader.UpdateLoadState();Check(Real(loader)==2,"only two new loads per pass");
        Check(loader.Calls==12,"original called exactly once per entity");Check(!loader.LastActive,"AI activation preserved");
        Check(RangeBridge.Statistics().Contains("candidates=8"),"eight candidate cap");
        var previous=loader.unloadedZombies[0];loader.unloadedZombies.RemoveAt(0);
        loader.zombies.Add(new Zombie{identity=previous.identity,obj=new ZombieObject{transform=new Transform{position=previous.transform.position}}});
        loader.UpdateLoadState();Check(Real(loader)==3,"retains promoted real plus two new");
        RangeBridge.Configure(1,20,0,360,800,600);loader.UpdateLoadState();Check(Real(loader)==0,"range reduction releases remote real");
        RangeBridge.Configure(1,200,0,360,800,600);Time.unscaledTime=1;loader.UpdateLoadState();Check(Real(loader)==0,"stale request expires");
        var other=new ZombieLoader();ZombieLoader.Instance=other;
        other.zombieProps.Add(new ZombieProp{identity=new ZombieIdentity{id=99},transform=new Transform{position=new Vector3(0,0,130)}});
        RangeBridge.Configure(1,200,0,360,800,600);other.UpdateLoadState();Check(Real(other)==1,"prop indexed correctly");
        other.zombieProps[0].transform.position=new Vector3(0,0,-130);
        RangeBridge.Configure(1,200,0,360,800,600);other.UpdateLoadState();Check(Real(other)==0,"front FOV excludes rear");
        RangeBridge.Configure(1,200,1,360,800,600);other.UpdateLoadState();Check(Real(other)==1,"360 includes rear within range");
        other.zombieProps[0].isWaveZombie=true;RangeBridge.Configure(0,10,0,360,800,600);other.UpdateLoadState();Check(Real(other)==1 && other.LastActive,"normal wave logic untouched");
        var buffer=Marshal.AllocHGlobal(20*256+4);
        try {
            ZombieLoader.Instance=loader;
            Check(RangeBridge.Collect(buffer,256,50)==0,"ESP 50 excludes distant states");
            Check(RangeBridge.Collect(buffer,256,200)==11,"ESP 200 includes dormant states without spawning");
            Check(loader.unloadedZombies.Count==11,"ESP never creates entities");
            Check(RangeBridge.Collect(buffer,1,200)==1,"snapshot respects capacity");
            Marshal.WriteInt32(buffer,20*256,12345);
            for(int i=20;i<350;++i)loader.unloadedZombies.Add(new UnloadedZombie{identity=new ZombieIdentity{id=i},transform=new ZombieTransform{position=new Vector3(0,0,110)}});
            Check(RangeBridge.Collect(buffer,256,200)==256,"snapshot bounded");Check(Marshal.ReadInt32(buffer,20*256)==12345,"ABI no overflow");
        }finally{Marshal.FreeHGlobal(buffer);harmony.UnpatchAll("range.tests");}
        Console.WriteLine(checks+" range/LOD Harmony checks passed");
    }
}
