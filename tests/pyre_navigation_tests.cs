using System;
using System.Collections.Generic;
using Zb2Menu;
namespace UnityEngine {
 public struct Vector3 {
  public float x,y,z;public Vector3(float a,float b,float c){x=a;y=b;z=c;}
  public static Vector3 up=new Vector3(0,1,0),down=new Vector3(0,-1,0);
  public static Vector3 operator +(Vector3 a,Vector3 b){return new Vector3(a.x+b.x,a.y+b.y,a.z+b.z);}
  public static Vector3 operator *(Vector3 a,float n){return new Vector3(a.x*n,a.y*n,a.z*n);}
  public static Vector3 zero {get{return new Vector3();}}
  public static float Distance(Vector3 a,Vector3 b){return (float)Math.Sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)+(a.z-b.z)*(a.z-b.z));}
 }
 public class Transform {public Vector3 position;public bool IsChildOf(Transform t){return ReferenceEquals(this,t);}}
 public class Rigidbody {public bool isKinematic;public Vector3 linearVelocity;}
 public class Collider {public Transform transform=new Transform();}
 public struct RaycastHit {public Vector3 point,normal;}
 public enum QueryTriggerInteraction {Ignore}
 public static class Time {public static float unscaledTime;}
 public static class Physics {
  public static bool ground=true;public static Collider[] obstacles=new Collider[0];
  public static bool Raycast(Vector3 p,Vector3 d,out RaycastHit hit,float range,int mask,QueryTriggerInteraction q){hit=new RaycastHit{point=new Vector3(p.x,0,p.z),normal=Vector3.up};return ground;}
  public static Collider[] OverlapCapsule(Vector3 a,Vector3 b,float r,int mask,QueryTriggerInteraction q){return obstacles;}
 }
}
public class InteractableFurniture {public UnityEngine.Transform transform=new UnityEngine.Transform();}
public class PyreInteractable:InteractableFurniture {public bool IsLit;}
public class WorkbenchInteractions {
 public static WorkbenchInteractions instance=new WorkbenchInteractions();
 readonly List<InteractableFurniture> allWorkbenches=new List<InteractableFurniture>();
 public void Add(InteractableFurniture item){allWorkbenches.Add(item);}
}
public class PlayerMain {
 public bool HasLocalControl=true;public float healthFast=100;
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
  UnityEngine.Physics.ground=false;PyreNavigation.Update(0,2,1);Check(player.transform.position.x==0 && PyreNavigation.Status().Contains("Sem destino"),"no teleport into void");
  UnityEngine.Physics.ground=true;UnityEngine.Physics.obstacles=new[]{new UnityEngine.Collider()};PyreNavigation.Update(0,3,1);Check(player.transform.position.x==0,"blocked landing rejected");
  UnityEngine.Physics.obstacles=new[]{new UnityEngine.Collider{transform=player.transform}};PyreNavigation.Update(0,4,1);Check(player.transform.position.x==20,"own collider does not block landing");
  player.HasLocalControl=false;player.transform.position=new UnityEngine.Vector3();PyreNavigation.Update(0,5,1);Check(player.transform.position.x==0,"never moves remote player");
  player.HasLocalControl=true;PyreNavigation.Update(0,6,0);PyreNavigation.Update(0,6,1);Check(player.transform.position.x==0,"request during scene transition not replayed");
  WorkbenchInteractions.instance=new WorkbenchInteractions();Check(PyreNavigation.All().Count==0,"map change clears registry");
  Console.WriteLine(checks+" pyre navigation checks passed");
 }
}
