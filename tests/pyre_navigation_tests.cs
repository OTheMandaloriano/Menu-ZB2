using System;
using System.Collections.Generic;
using Zb2Menu;
namespace UnityEngine {
 public enum FindObjectsInactive {Include}public enum FindObjectsSortMode {None}
 public class Object {public static LODTarget[] targets=new LODTarget[0];public static T[] FindObjectsByType<T>(FindObjectsInactive a,FindObjectsSortMode b){return targets as T[];}}

 public struct Vector3 {
  public float x,y,z;public Vector3(float a,float b,float c){x=a;y=b;z=c;}
  public static Vector3 up=new Vector3(0,1,0),down=new Vector3(0,-1,0);
  public static Vector3 operator +(Vector3 a,Vector3 b){return new Vector3(a.x+b.x,a.y+b.y,a.z+b.z);}
  public static Vector3 operator *(Vector3 a,float n){return new Vector3(a.x*n,a.y*n,a.z*n);}
  public float sqrMagnitude {get{return x*x+y*y+z*z;}}
  public static Vector3 zero {get{return new Vector3();}}
  public static float Distance(Vector3 a,Vector3 b){return (float)Math.Sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)+(a.z-b.z)*(a.z-b.z));}
 }
 public struct Quaternion {public Vector3 direction;public static Quaternion LookRotation(Vector3 v){return new Quaternion{direction=v};}}
 public struct Bounds {
  public Vector3 center,extents;
  public Bounds(Vector3 c,Vector3 size){center=c;extents=size*.5f;}
  public void Encapsulate(Vector3 p){float lx=Math.Min(center.x-extents.x,p.x),ly=Math.Min(center.y-extents.y,p.y),lz=Math.Min(center.z-extents.z,p.z);float hx=Math.Max(center.x+extents.x,p.x),hy=Math.Max(center.y+extents.y,p.y),hz=Math.Max(center.z+extents.z,p.z);center=new Vector3((lx+hx)*.5f,(ly+hy)*.5f,(lz+hz)*.5f);extents=new Vector3((hx-lx)*.5f,(hy-ly)*.5f,(hz-lz)*.5f);}
  public float SqrDistance(Vector3 p){float x=Math.Max(0,Math.Abs(p.x-center.x)-extents.x),y=Math.Max(0,Math.Abs(p.y-center.y)-extents.y),z=Math.Max(0,Math.Abs(p.z-center.z)-extents.z);return x*x+y*y+z*z;}
 }
 public class Mesh {public Bounds bounds;}
 public class MeshCollider:Collider {public Mesh sharedMesh;}
 public class BoxCollider:Collider {public Vector3 center,size;}
 public class GameObject {public Collider[] colliders=new Collider[0];public T[] GetComponentsInChildren<T>(bool inactive){return colliders as T[];}}
 public class Transform {public Quaternion rotation;public Vector3 TransformPoint(Vector3 p){return position+p;}public Vector3 position;public bool IsChildOf(Transform t){return ReferenceEquals(this,t);}}
 public class Rigidbody {public bool isKinematic;public Vector3 linearVelocity;}
 public class Collider {public Bounds bounds;public string name="Wall";public Transform transform=new Transform();}
 public struct RaycastHit {public Vector3 point,normal;}
 public enum QueryTriggerInteraction {Ignore}
 public static class Time {public static float unscaledTime;}
 public static class Physics {public static void SyncTransforms(){}
  public static Func<Vector3,Collider[]> obstacleProbe;public static bool ground=true;public static Collider[] obstacles=new Collider[0];
  public static bool Raycast(Vector3 p,Vector3 d,out RaycastHit hit,float range,int mask,QueryTriggerInteraction q){hit=new RaycastHit{point=new Vector3(p.x,0,p.z),normal=Vector3.up};return ground;}
  public static Collider[] OverlapCapsule(Vector3 a,Vector3 b,float r,int mask,QueryTriggerInteraction q){return obstacleProbe!=null?obstacleProbe(a):obstacles;}
 }
}
public class LODCollider {public UnityEngine.Collider col;public UnityEngine.GameObject obj;public bool colliding;public void SetColliding(bool b,bool f){colliding=b;}}
public class LODTarget {public UnityEngine.Transform transform=new UnityEngine.Transform();public LODCollider lodCollider=new LODCollider();}
public class InteractableFurniture {public UnityEngine.Vector3 InteractionPoint {get{return transform.position;}}public UnityEngine.Transform transform=new UnityEngine.Transform();}
public class PyreInteractable:InteractableFurniture {public bool IsLit;}
public class WorkbenchInteractions {
 public static WorkbenchInteractions instance=new WorkbenchInteractions();
 readonly List<InteractableFurniture> allWorkbenches=new List<InteractableFurniture>();
 public void Add(InteractableFurniture item){allWorkbenches.Add(item);}
}
public class PlayerMovement {public int groundMask=-1;}
public class PlayerMain {public PlayerMovement movement=new PlayerMovement();
 public float defaultHeight=1.8f;public bool HasLocalControl=true;public float healthFast=100;
 public UnityEngine.Transform transform=new UnityEngine.Transform();public UnityEngine.Rigidbody body=new UnityEngine.Rigidbody();
 public T GetComponent<T>() where T:class {return body as T;}
}
public class PlayersController {public static PlayersController instance=new PlayersController();public PlayerMain player=new PlayerMain();public PlayerMain MyPlayer(){return player;}}
class PyreTests {
 static int checks;static void Check(bool ok,string name){if(!ok)throw new Exception(name);checks++;}
 static void Main(){
  var registry=WorkbenchInteractions.instance;var player=PlayersController.instance.player;
  registry.Add(new InteractableFurniture());registry.Add(new PyreInteractable{transform=new UnityEngine.Transform{position=new UnityEngine.Vector3(20,0,20)}});
  registry.Add(new PyreInteractable{IsLit=true,transform=new UnityEngine.Transform{position=new UnityEngine.Vector3(40,0,40)}});
  Check(PyreNavigation.All().Count==2,"full registry includes lit and undiscovered, excludes other furniture");
  Check(PyreNavigation.Update(1,0,1)==2 && PyreNavigation.Status().Contains("aceso"),"selected status and count");
  PyreNavigation.Update(1,1,1);Check(player.transform.position.x==40 && player.transform.position.z>40,"selected destination rather than closest");
  player.transform.position=new UnityEngine.Vector3();PyreNavigation.Update(1,1,1);Check(player.transform.position.x==0,"request consumed once");
  UnityEngine.Physics.ground=false;PyreNavigation.Update(0,2,1);Check(player.transform.position.x==0 && PyreNavigation.Status().Contains("sem chao="),"no teleport into void");
  UnityEngine.Physics.ground=true;UnityEngine.Physics.obstacles=new[]{new UnityEngine.Collider()};PyreNavigation.Update(0,3,1);Check(player.transform.position.x==0,"blocked landing rejected");
  UnityEngine.Physics.obstacles=new[]{new UnityEngine.Collider{transform=player.transform}};PyreNavigation.Update(0,4,1);Check(player.transform.position.x==20,"own collider does not block landing");
  player.HasLocalControl=false;player.transform.position=new UnityEngine.Vector3();PyreNavigation.Update(0,5,1);Check(player.transform.position.x==0,"never moves remote player");
  player.HasLocalControl=true;PyreNavigation.Update(0,6,0);PyreNavigation.Update(0,6,1);Check(player.transform.position.x==0,"request during scene transition not replayed");
  WorkbenchInteractions.instance=new WorkbenchInteractions();Check(PyreNavigation.All().Count==0,"map change clears registry");
  var lod=new LODTarget();UnityEngine.Object.targets=new[]{lod};
  using(var scope=new NearbyCollisionScope(new UnityEngine.Vector3())){Check(lod.lodCollider.colliding,"destination LOD prepared before query");}
  Check(!lod.lodCollider.colliding,"failed query restores original collision LOD");
  using(var scope=new NearbyCollisionScope(new UnityEngine.Vector3())){scope.Arrived();}
  Check(lod.lodCollider.colliding,"arrival hands enabled nearby collision to normal LOD");
  var broad=new LODTarget{transform=new UnityEngine.Transform{position=new UnityEngine.Vector3(100,0,100)}};
  broad.lodCollider.col=new UnityEngine.MeshCollider{transform=broad.transform,sharedMesh=new UnityEngine.Mesh{bounds=new UnityEngine.Bounds(new UnityEngine.Vector3(-100,0,-100),new UnityEngine.Vector3(40,2,40))}};
  UnityEngine.Object.targets=new[]{broad};
  using(var scope=new NearbyCollisionScope(new UnityEngine.Vector3())){Check(broad.lodCollider.colliding,"disabled large floor selected by geometry despite remote pivot");}
  Check(!broad.lodCollider.colliding,"large floor restored after failed preparation");
  WorkbenchInteractions.instance=registry;UnityEngine.Object.targets=new LODTarget[0];UnityEngine.Physics.obstacles=new UnityEngine.Collider[0];
  UnityEngine.Physics.obstacleProbe=pos=>Math.Abs(pos.x-20.325f)<.02f && Math.Abs(pos.z-20.563f)<.02f?new UnityEngine.Collider[0]:new[]{new UnityEngine.Collider()};
  PyreNavigation.Update(0,10,1);
  Check(Math.Abs(player.transform.position.x-20.325f)<.02f,"dense samples find passage missed by old 45 degree spacing");
  Check(UnityEngine.Vector3.Distance(player.transform.position,new UnityEngine.Vector3(20,0,20))<1.45f,"successful destination remains inside interaction range");
  Check(player.transform.rotation.direction.z<0,"arrival faces interaction point");
  UnityEngine.Physics.obstacleProbe=null;
  Console.WriteLine(checks+" pyre navigation checks passed");
 }
}
