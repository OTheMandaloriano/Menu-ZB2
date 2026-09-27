using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using HarmonyLib;
using Zb2Menu;
using UnityEngine;
namespace UnityEngine {
    public class Object { }
    public class Transform {public Vector3 position;public Vector3 TransformDirection(Vector3 p){return p;}public bool IsChildOf(Transform t){return ReferenceEquals(this,t);}}
    public struct Vector3 {public float x,y,z;public Vector3(float a,float b,float c){x=a;y=b;z=c;}public float sqrMagnitude{get{return x*x+y*y+z*z;}}public void Normalize(){float n=(float)Math.Sqrt(sqrMagnitude);x/=n;y/=n;z/=n;}public static Vector3 operator *(Vector3 v,float n){return new Vector3(v.x*n,v.y*n,v.z*n);}public static Vector3 operator +(Vector3 a,Vector3 b){return new Vector3(a.x+b.x,a.y+b.y,a.z+b.z);}}
    public class Rigidbody {public bool isKinematic,detectCollisions=true;}
    public struct Bounds {public Vector3 center,extents;}
    public class Collider {public Transform transform=new Transform();public Bounds bounds;}
    public struct Quaternion {public static Quaternion identity {get{return new Quaternion();}}}
    public enum QueryTriggerInteraction {Ignore}
    public static class Physics {public static bool blocked;public static int OverlapBoxNonAlloc(Vector3 a,Vector3 b,Collider[] c,Quaternion d,int e,QueryTriggerInteraction f){if(!blocked)return 0;c[0]=new Collider();return 1;}}
    public static class Time {public static float unscaledTime,deltaTime=.1f,timeScale=1;}
    public static class Application {public static bool isFocused=true;}
    public enum KeyCode {W,A,S,D,Space,LeftControl,LeftShift}
    public static class Input {public static HashSet<KeyCode> keys=new HashSet<KeyCode>();public static bool GetKey(KeyCode k){return keys.Contains(k);}}
    public static class Mathf {public static float Clamp(float x,float a,float b){return Math.Max(a,Math.Min(b,x));}}
}
public struct IntVec2 {public int x,y;public IntVec2(int a,int b){x=a;y=b;}}
public class DatabaseItem {public IntVec2 Size(bool rotated){return new IntVec2(1,1);}}
public class InventoryItem {public bool empty;public IntVec2 pos;public bool rotated;public bool IsEmpty(){return empty;}public DatabaseItem GetDataBaseItem(){return new DatabaseItem();}}
public class ItemContainer {public IntVec2 TotalSize=new IntVec2(6,8),UsableSize=new IntVec2(6,4);public List<InventoryItem> items=new List<InventoryItem>();public void SetUsableSize(int x,int y){UsableSize=new IntVec2(x,y);}}
public class PlayerEquippedItems {public List<InventoryItem> misc=new List<InventoryItem>{new InventoryItem{empty=true},new InventoryItem{empty=true},new InventoryItem{empty=true},new InventoryItem{empty=true}};public int UnlockedMiscSlotsCount=2;public int MiscCount{get{return misc.Count;}}public InventoryItem GetMisc(int i){return misc[i];}public void SetUnlockedMiscSlotsCount(int n){if(n>misc.Count)throw new Exception("out of bounds");UnlockedMiscSlotsCount=n;}}
public class PlayerInventory {public ItemContainer storage=new ItemContainer();public PlayerEquippedItems equippedItems=new PlayerEquippedItems();public bool TotalStorageUnlocked{get;private set;}public bool TotalMiscSlotsUnlocked{get;private set;}public int updates;public void OnEquipmentChanged(){++updates;}}
public class PlayerMovement {public Collider hitbox=new Collider();}
public class PlayerCamera {public Transform CameraTransform=new Transform();}
public class PlayerMain {public PlayerInventory inventory=new PlayerInventory();public NoClip noClip=new NoClip();public PlayerMovement movement=new PlayerMovement();public Transform transform=new Transform();public PlayerCamera cam=new PlayerCamera();public float healthFast=100;}
public static class GlobalTexting {public static bool IsTexting;}
public static class DeveloperConsole {public static bool Opened;}
public class NoClip {public Rigidbody targetBody=new Rigidbody();public bool enabled;public float speed=10;public Transform transform=new Transform();public int originalCalls;[MethodImpl(MethodImplOptions.NoInlining)]public void Update(){++originalCalls;}}
class FeatureTests {
    static int checks;
    static void Check(bool b,string n){if(!b)throw new Exception(n);++checks;}
    static void Main(){
        var p=new PlayerMain();var inv=p.inventory;
        SlotsBridge.Apply(p,true);Check(inv.equippedItems.UnlockedMiscSlotsCount==4,"uses actual misc count");Check(inv.storage.UsableSize.x==6 && inv.storage.UsableSize.y==8,"uses actual grid");
        for(int repeat=0;repeat<100;++repeat)SlotsBridge.Apply(p,true);Check(inv.updates==1,"no repeated inventory refresh during drag");
        for(int i=0;i<inv.equippedItems.UnlockedMiscSlotsCount;++i)Check(inv.equippedItems.GetMisc(i)!=null,"slots movable within list");
        inv.storage.items.Add(new InventoryItem{pos=new IntVec2(0,6)});SlotsBridge.Apply(p,false);Check(SlotsBridge.Pending && inv.storage.UsableSize.y==8,"refuses to strand stored item");
        inv.storage.items[0].pos=new IntVec2(0,2);inv.equippedItems.misc[3].empty=false;SlotsBridge.Apply(p,false);Check(SlotsBridge.Pending,"refuses to strand equipped item");
        inv.equippedItems.misc[3].empty=true;SlotsBridge.Apply(p,false);Check(!SlotsBridge.Pending && inv.storage.UsableSize.y==4 && inv.equippedItems.UnlockedMiscSlotsCount==2,"restores exact originals");
        SlotsBridge.Apply(p,true);SlotsBridge.Apply(p,false);Check(!inv.TotalStorageUnlocked && !inv.TotalMiscSlotsUnlocked,"repeat toggle restores flags");
        var h=new Harmony("features.tests");NoClipBridge.Install(h);
        NoClipBridge.Apply(p,true,2,true);Check(p.noClip.enabled && p.noClip.targetBody.isKinematic && !p.noClip.targetBody.detectCollisions,"noclip physics enabled");
        Input.keys.Add(KeyCode.W);p.noClip.Update();Check(p.noClip.originalCalls==0 && p.noClip.transform.position.z==2,"owned update moves once");
        NoClipBridge.Apply(p,true,1,true);p.noClip.Update();Check(p.noClip.transform.position.z==3 && p.noClip.speed==10,"speed reduction uses original");
        NoClipBridge.Apply(p,true,1,false);p.noClip.Update();Check(p.noClip.transform.position.z==3,"menu blocks movement");
        NoClipBridge.Apply(p,true,1,true);GlobalTexting.IsTexting=true;p.noClip.Update();Check(p.noClip.transform.position.z==3,"chat blocks movement");GlobalTexting.IsTexting=false;
        NoClipBridge.Apply(p,false,1,true);Check(!p.noClip.enabled && !p.noClip.targetBody.isKinematic && p.noClip.targetBody.detectCollisions,"physics restored");
        var other=new NoClip();other.Update();Check(other.originalCalls==1,"unowned noclip untouched");
        NoClipBridge.Apply(p,true,2,true);Physics.blocked=true;p.noClip.transform.position=new Vector3(99,99,99);NoClipBridge.Restore();Check(p.noClip.transform.position.z==3,"exit inside obstacle returns to safe position");Physics.blocked=false;
        NoClipBridge.Apply(p,true,1,true);Time.unscaledTime=1;p.noClip.Update();Check(!p.noClip.enabled,"stale callback restores physics");
        h.UnpatchAll("features.tests");Console.WriteLine(checks+" player feature checks passed");
    }
}
