using System;
using System.Collections.Generic;
using Zb2Menu;
using UnityEngine;
namespace UnityEngine {
 public struct Vector3 {public float x,y,z;public Vector3(float a,float b,float c){x=a;y=b;z=c;}public static Vector3 up{get{return new Vector3(0,1,0);}}public static Vector3 down{get{return new Vector3(0,-1,0);}}public static Vector3 operator +(Vector3 a,Vector3 b){return new Vector3(a.x+b.x,a.y+b.y,a.z+b.z);}public static Vector3 operator -(Vector3 a,Vector3 b){return new Vector3(a.x-b.x,a.y-b.y,a.z-b.z);}public static Vector3 operator *(Vector3 a,float v){return new Vector3(a.x*v,a.y*v,a.z*v);}public static float Distance(Vector3 a,Vector3 b){var d=a-b;return (float)Math.Sqrt(d.x*d.x+d.y*d.y+d.z*d.z);}}
 public class Transform {public Vector3 position,forward=new Vector3(0,0,1),right=new Vector3(1,0,0);public Transform parent;public void SetParent(Transform p,bool w){parent=p;}}
 public class GameObject {public Transform transform=new Transform();}
 public struct RaycastHit {public Vector3 point,normal;}
 public enum QueryTriggerInteraction {Ignore}
 public static class Physics {public static bool floor=true;public static bool Raycast(Vector3 a,Vector3 d,out RaycastHit hit,float length,int mask,QueryTriggerInteraction t){hit=new RaycastHit{point=new Vector3(a.x,0,a.z),normal=Vector3.up};return floor;}}
 public static class Mathf {public static float Clamp(float v,float a,float b){return Math.Max(a,Math.Min(b,v));}}
 public static class Time {public static float unscaledTime,timeScale=1;}
 public static class Application {public static bool isFocused=true;}
}
public static class GlobalTexting {public static bool IsTexting;}
public static class DeveloperConsole {public static bool Opened;}
public class PlayerMain {public bool HasLocalControl=true;public float healthFast=100;public Transform transform=new Transform();}
public class InventoryItem {public enum ID{None,Wood,PistolAmmo,Gun};public ID id=ID.Wood;public DatabaseItem db=new DatabaseItem();public DatabaseItem GetDataBaseItem(){db.itemID=id;return db;}}
public class DatabaseItem {public enum SubType{Misc,Melee};public InventoryItem.ID itemID;public bool hidden,reserved;public SubType GetSubType(){return SubType.Misc;}}
public class DatabaseGun:DatabaseItem{}
public static class Pricing {public static bool blocked;public static bool IsBlocked(InventoryItem.ID id){return blocked;}}
public class ItemsBase {public static ItemsBase instance;public static float ItemCount=128;public DatabaseItem GetItem(InventoryItem.ID id){return null;}}
public class DroppedLoot {public int id;public bool IsSack;public int? reservedPlayerID;public InventoryItem item=new InventoryItem();public Transform transform=new Transform();}
public class HashCell {public List<DroppedLoot> loot=new List<DroppedLoot>();public GameObject lootHolder=new GameObject();}
public struct Coord {public int x,y;}
public class MapHash {public static MapHash instance=new MapHash();public bool IsCreated=true;public HashCell source=new HashCell(),target=new HashCell();public Coord GetHashCoord(Vector3 v){return new Coord();}public HashCell GetCell(int x,int y){return source;}public HashCell GetCell(Vector3 v){return target;}}
public class LootController {public static LootController Instance=new LootController();public HashCell GetLootLocation(int id,out int index){foreach(var c in new[]{MapHash.instance.source,MapHash.instance.target}){index=c.loot.FindIndex(l=>l.id==id);if(index>=0)return c;}index=-1;return null;}}
public class MultiplayerController {public static MultiplayerController instance=new MultiplayerController();public bool server=true,online=true;public bool IsServer(){return server;}public bool IsOnlineServer(){return online;}}
public class Speaker {public int removed,spawned;public void BroadcastLootRemoval(HashCell c,int id){removed++;}public void SendSingleLootSpawn(DroppedLoot l,HashCell c){spawned++;}}
public class ServerController {public static ServerController instance=new ServerController();public Speaker GetSpeaker=new Speaker();}
class LootTests {
 static int checks;static void Check(bool b,string n){if(!b)throw new Exception(n);checks++;}
 static void Apply(PlayerMain p){Time.unscaledTime+=1;LootMagnetBridge.Apply(p,true,true,50,3,-1,-1,-1,-1);}
 static void Main(){
  var p=new PlayerMain();var item=new DroppedLoot{id=9};item.transform.position=new Vector3(0,0,20);MapHash.instance.source.loot.Add(item);
  Pricing.blocked=true;Apply(p);Check(MapHash.instance.source.loot.Count==1 && ServerController.instance.GetSpeaker.removed==0,"blocked loot never moves");Pricing.blocked=false;
  item.item.db.hidden=true;Apply(p);Check(MapHash.instance.source.loot.Count==1,"hidden loot never moves");item.item.db.hidden=false;
  MultiplayerController.instance.server=false;Apply(p);Check(MapHash.instance.source.loot.Count==1,"client cannot move authoritative loot");MultiplayerController.instance.server=true;
  item.reservedPlayerID=4;Apply(p);Check(MapHash.instance.source.loot.Count==1,"reserved player loot preserved");item.reservedPlayerID=null;
  Physics.floor=false;Apply(p);Check(MapHash.instance.source.loot.Count==1,"no relocation without ground");Physics.floor=true;
  var original=item.item;Apply(p);Check(MapHash.instance.source.loot.Count==0 && MapHash.instance.target.loot.Count==1,"cell ownership transferred");Check(ReferenceEquals(original,item.item) && item.id==9,"item and identity retained");Check(ServerController.instance.GetSpeaker.removed==1 && ServerController.instance.GetSpeaker.spawned==1,"single remove and reannounce");
  Apply(p);Check(MapHash.instance.target.loot.Count==1 && ServerController.instance.GetSpeaker.spawned==1,"no duplicate or repeated pickup");
  Console.WriteLine(checks+" loot magnet checks passed");
 }
}
