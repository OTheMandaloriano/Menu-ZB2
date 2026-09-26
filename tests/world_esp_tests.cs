using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using Zb2Menu;
namespace UnityEngine {
    public struct Vector3 {
        public float x,y,z;
        public Vector3(float a,float b,float c){x=a;y=b;z=c;}
        public static Vector3 operator +(Vector3 a,Vector3 b){return new Vector3(a.x+b.x,a.y+b.y,a.z+b.z);}
        public static Vector3 operator -(Vector3 a,Vector3 b){return new Vector3(a.x-b.x,a.y-b.y,a.z-b.z);}
        public static float Distance(Vector3 a,Vector3 b){var d=a-b;return (float)Math.Sqrt(d.x*d.x+d.y*d.y+d.z*d.z);}
        public static float Angle(Vector3 a,Vector3 b){float den=Distance(a,new Vector3())*Distance(b,new Vector3());return den==0?0:(float)(Math.Acos(Math.Max(-1,Math.Min(1,(a.x*b.x+a.y*b.y+a.z*b.z)/den)))*180/Math.PI);}
    }
    public class Transform {public Vector3 position;public Vector3 forward=new Vector3(0,0,1);}
    public class Camera {public Transform transform=new Transform();public Vector3 WorldToViewportPoint(Vector3 p){return new Vector3(.5f+p.x/100,.5f+p.y/100,p.z);}}
    public static class Time { public static float unscaledTime; }
}
public class PlayerMain {public float healthFast=100;public UnityEngine.Transform transform=new UnityEngine.Transform();}
public class PlayersController {public static PlayersController instance=new PlayersController(); public PlayerMain player=new PlayerMain();public PlayerMain MyPlayer(){return player;}}
public class MainCamera {public static MainCamera instance=new MainCamera(); public UnityEngine.Camera cam=new UnityEngine.Camera();}
public class InventoryItem {public enum ID{RifleAmmo,SniperAmmo,ShotgunAmmo,PistolAmmo,Gun}; public ID id=ID.Gun; public DatabaseItem db=new DatabaseItem();public DatabaseItem GetDataBaseItem(){return db;}}
public class DatabaseItem {public enum SubType{Melee,Misc};public int tier;public string GetName{get{return "Objeto";}}public virtual SubType GetSubType(){return SubType.Misc;}}
public class DatabaseGun:DatabaseItem{}
public class DroppedLoot {public InventoryItem item=new InventoryItem();public UnityEngine.Transform transform=new UnityEngine.Transform();}
public struct Coord {public int x,y;}
public class HashCell {public List<DroppedLoot> loot=new List<DroppedLoot>();}
public class MapHash {public static MapHash instance=new MapHash();public bool IsCreated=true;public float cellSize=96;public HashCell cell=new HashCell();public HashCell GetCell(int x,int y){return cell;}public Coord GetHashCoord(UnityEngine.Vector3 p){return new Coord();}}
public class InterestPointController {public static InterestPointController instance=new InterestPointController();public List<InterestPoint> points=new List<InterestPoint>();}
public class InterestPoint {
    public enum Type {Helicopter,Bossfight,QuestionMark,Bomb,Ammo,Gun,Melee,HealingItem,CraftingMaterial,Food,ReloadingBench,GunUpgradeTable,ZumbiePyre,VendorVan,StartingHouse,PlayerArrow};
    public Type type; public UnityEngine.Transform objTransform; public UnityEngine.Vector3 pos3D=new UnityEngine.Vector3(0,0,10);
}
class WorldTests {
    static int checks;
    static IntPtr buffer;
    static void Check(bool v,string label){if(!v)throw new Exception(label+" "+WorldEspBridge.LastError());++checks;}
    static int Collect(int mask,float range=150,int cap=256,float poiRange=150){return WorldEspBridge.Collect(buffer,cap,mask,range,poiRange);}
    static DroppedLoot Loot(float x,float z,DatabaseItem db,InventoryItem.ID id=InventoryItem.ID.Gun){var l=new DroppedLoot();l.item.db=db;l.item.id=id;l.transform.position=new UnityEngine.Vector3(x,0,z);return l;}
    static void Main(){
        buffer=Marshal.AllocHGlobal(112*256+16);
        try {
            MapHash.instance.cell.loot.Add(Loot(0,10,new DatabaseGun()));
            MapHash.instance.cell.loot.Add(Loot(0,12,new DatabaseItem(),InventoryItem.ID.PistolAmmo));
            MapHash.instance.cell.loot.Add(Loot(0,14,new DatabaseItem()));
            MapHash.instance.cell.loot.Add(Loot(0,16,new DatabaseGun{tier=3}));
            Check(Collect(0)==0,"all off");Check(Collect(1)==2,"weapons only");Check(Collect(2)==1,"rare only");
            Check(Collect(4)==1,"ammo only");Check(Collect(8)==1,"supplies only");Check(Collect(15)==4,"no duplicate rare weapon");
            Check(Collect(15,11)==1,"distance slider immediate");Check(Collect(15,150)==4,"distance increases immediate");
            InterestPointController.instance.points.Add(new InterestPoint{type=InterestPoint.Type.Helicopter});
            Check(Collect(16)==1,"POI independent of items/zombies");Check(Marshal.ReadInt32(buffer)==4,"marker category ABI");
            byte[] name=new byte[96];Marshal.Copy(IntPtr.Add(buffer,16),name,0,96);Check(System.Text.Encoding.UTF8.GetString(name).StartsWith("Helicoptero"),"UTF8 marker layout");
            InterestPointController.instance.points.Add(new InterestPoint{type=InterestPoint.Type.ZumbiePyre});
            Check(Collect(1<<10)==1,"POI category isolated");Check(Collect(16|1024)==2,"multiple categories");
            InterestPointController.instance.points[0].pos3D=new UnityEngine.Vector3(0,0,-1);Check(Collect(16)==0,"behind camera rejected");
            InterestPointController.instance.points[0].pos3D=new UnityEngine.Vector3(100,0,10);Check(Collect(16)==0,"outside viewport rejected");
            InterestPointController.instance.points[0].pos3D=new UnityEngine.Vector3(0,0,.1f);Check(Collect(16)==1,"near marker retained");
            PlayersController.instance.player.healthFast=0;Check(Collect(15|16)==0,"dead player clears");PlayersController.instance.player.healthFast=100;
            Check(Collect(15,150,1)==1,"caller capacity honored");
            for(int i=0;i<300;++i)InterestPointController.instance.points.Add(new InterestPoint{type=InterestPoint.Type.Helicopter});
            Marshal.WriteInt32(buffer,112*256,1234567);Check(Collect(16)==256,"capacity bound");Check(Marshal.ReadInt32(buffer,112*256)==1234567,"no buffer overrun");
            InterestPointController.instance.points.Clear();
            InterestPointController.instance.points.Add(new InterestPoint{type=InterestPoint.Type.ZumbiePyre,pos3D=new UnityEngine.Vector3(0,0,100)});
            Check(Collect(15|1024,11,256,150)==2,"short item radius keeps distant POI");
            Check(Collect(15|1024,150,256,20)==4,"short POI radius keeps all items");
            Check(Collect(15|1024,11,256,20)==1,"both radii independent");
            Check(Collect(15|1024,150,256,150)==5,"both radii expanded");
            Check(WorldEspBridge.LastError()=="","no adapter error");
            Console.WriteLine(checks+" world ESP checks passed");
        } finally {Marshal.FreeHGlobal(buffer);}
    }
}
