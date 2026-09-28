using System;
using Zb2Menu;
namespace UnityEngine {
 public struct Vector3 {public float x,y,z;public Vector3(float a,float b,float c){x=a;y=b;z=c;}public float sqrMagnitude{get{return x*x+y*y+z*z;}}}
 public struct Bounds {public Vector3 center,extents;public void Encapsulate(Bounds b){float lo=Math.Min(center.x-extents.x,b.center.x-b.extents.x),hi=Math.Max(center.x+extents.x,b.center.x+b.extents.x);center.x=(hi+lo)/2;extents.x=(hi-lo)/2;}}
 public class Transform {public string name;}
 public class GameObject {public bool activeInHierarchy=true;}
 public class Renderer {public bool enabled=true;public GameObject gameObject=new GameObject();public Bounds bounds;}
 public class SkinnedMeshRenderer:Renderer {}public class MeshRenderer:Renderer {}
 public enum HumanBodyBones {Head,Neck,Chest,Hips}
 public class Animator {public bool isHuman;public Transform[] bones=new Transform[4];public Transform GetBoneTransform(HumanBodyBones b){return bones[(int)b];}}
 public static class Time {public static float unscaledTime;}
}
public class ZombieObject {public UnityEngine.Animator animator;public UnityEngine.Transform zombieEyeRef;public UnityEngine.Transform[] bones=new UnityEngine.Transform[0];public UnityEngine.Renderer[] renderers=new UnityEngine.Renderer[0];public T[] GetComponentsInChildren<T>(bool all){return typeof(T)==typeof(UnityEngine.Transform)?bones as T[]:renderers as T[];}}
class RigTests {
 static int checks;static void Check(bool b,string s){if(!b)throw new Exception(s);checks++;}
 static void Main(){
  var z=new ZombieObject{zombieEyeRef=new UnityEngine.Transform{name="eye"},bones=new[]{new UnityEngine.Transform{name="Bip001 Neck"},new UnityEngine.Transform{name="mixamorig:Spine2"},new UnityEngine.Transform{name="Pelvis"}}};
  Check(ZombieRigBridge.Bone(z,0)==z.zombieEyeRef,"head may use explicit eye reference");
  Check(ZombieRigBridge.Bone(z,1)==z.bones[0] && ZombieRigBridge.Bone(z,2)==z.bones[1] && ZombieRigBridge.Bone(z,3)==z.bones[2],"distinct targets use named bones");
  var absent=new ZombieObject{zombieEyeRef=z.zombieEyeRef};Check(ZombieRigBridge.Bone(absent,2)==null,"missing chest never silently becomes head");
  var h=new ZombieObject{animator=new UnityEngine.Animator{isHuman=true}};h.animator.bones[2]=new UnityEngine.Transform{name="custom"};Check(ZombieRigBridge.Bone(h,2)==h.animator.bones[2],"humanoid mapping preferred");
  var body=new UnityEngine.SkinnedMeshRenderer{bounds=new UnityEngine.Bounds{center=new UnityEngine.Vector3(0,0,0),extents=new UnityEngine.Vector3(1,2,1)}};
  var wings=new UnityEngine.MeshRenderer{bounds=new UnityEngine.Bounds{center=new UnityEngine.Vector3(3,0,0),extents=new UnityEngine.Vector3(2,1,1)}};
  var model=new ZombieObject{renderers=new UnityEngine.Renderer[]{body,wings}};var ptr=System.Runtime.InteropServices.Marshal.AllocHGlobal(24);
  try {Check(ZombieRigBridge.Bounds(model,ptr),"bounds available");var data=new float[6];System.Runtime.InteropServices.Marshal.Copy(ptr,data,0,6);Check(data[0]==2 && data[3]==3,"bounds union includes wings beyond torso");}finally{System.Runtime.InteropServices.Marshal.FreeHGlobal(ptr);}
  Console.WriteLine(checks+" rig mapping and bounds checks passed");
 }
}
