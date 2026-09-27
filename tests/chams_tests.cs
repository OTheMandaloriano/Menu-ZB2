using System;
using System.Collections.Generic;
using Zb2Menu;
namespace UnityEngine {
    public class Object {public static int destroyed;public static void Destroy(object o){if(o!=null)++destroyed;}}
    public enum HideFlags {HideAndDontSave}
    public struct Color {}
    public struct Color32 {public Color32(byte r,byte g,byte b,byte a){}public static implicit operator Color(Color32 c){return new Color();}}
    public class Shader {public static bool available=true;public bool isSupported=true;public static Shader Find(string s){return available?new Shader():null;}}
    public class Material {public HideFlags hideFlags;public static bool properties=true;public Material(Shader s){}public void SetInt(string s,int n){}public void SetColor(string s,Color c){}public bool HasProperty(string s){return properties;}}
    public class Mesh {public int subMeshCount=2;}
    public class Renderer {public bool enabled=true;public Mesh sharedMesh=new Mesh();public object originalMaterial=new object();}
    public class Camera {public int attached,removed;public Rendering.CommandBuffer buffer;public void AddCommandBuffer(Rendering.CameraEvent e,Rendering.CommandBuffer b){attached++;buffer=b;}public void RemoveCommandBuffer(Rendering.CameraEvent e,Rendering.CommandBuffer b){removed++;buffer=null;}}
}
namespace UnityEngine.Rendering {
    public enum CompareFunction {Greater,LessEqual}public enum BlendMode {SrcAlpha,OneMinusSrcAlpha}public enum CullMode {Off}public enum CameraEvent {AfterForwardAlpha}
    public static class GraphicsSettings {public static object currentRenderPipeline;}
    public class CommandBuffer {public string name;public int draws;public bool released;public void DrawRenderer(UnityEngine.Renderer r,UnityEngine.Material m,int sub,int pass){draws++;}public void Clear(){draws=0;}public void Release(){released=true;}}
}
public class MainCamera {public static MainCamera instance=new MainCamera();public UnityEngine.Camera cam=new UnityEngine.Camera();}
public class ZombieHealth {public bool isAlive=true;public float amount=100;}
public class ZombieObject {public UnityEngine.Renderer meshRenderer=new UnityEngine.Renderer();}
public class Zombie {public ZombieHealth health=new ZombieHealth();public ZombieObject obj=new ZombieObject();}
public class ZombieLoader {public static ZombieLoader Instance=new ZombieLoader();public List<Zombie> zombies=new List<Zombie>();}
class ChamsTests {
    static int checks;static void Check(bool b,string text){if(!b)throw new Exception(text);checks++;}
    static void Main(){
        var z=new Zombie();ZombieLoader.Instance.zombies.Add(z);var material=z.obj.meshRenderer.originalMaterial;var camera=MainCamera.instance.cam;
        Check(ChamsBridge.Apply(1,0,0)=="","initialize chams");Check(camera.attached==1 && camera.buffer.draws==4,"two depth passes for each submesh");
        ChamsBridge.Apply(1,1,2);Check(camera.attached==1 && camera.buffer.draws==4,"reuse command buffer");Check(ReferenceEquals(material,z.obj.meshRenderer.originalMaterial),"never overwrites game material");
        z.health.isAlive=false;ChamsBridge.Apply(1,1,2);Check(camera.buffer.draws==0,"dead bodies removed");
        var buffer=camera.buffer;ChamsBridge.Apply(0,0,0);Check(camera.removed==1 && buffer.released,"disable removes and disposes buffer");
        UnityEngine.Shader.available=false;Check(ChamsBridge.Apply(1,0,0).Contains("shader"),"missing shader reports unsupported");UnityEngine.Shader.available=true;
        UnityEngine.Material.properties=false;Check(ChamsBridge.Apply(1,0,0).Contains("profundidade"),"reject shader without required properties");UnityEngine.Material.properties=true;
        UnityEngine.Rendering.GraphicsSettings.currentRenderPipeline=new object();Check(ChamsBridge.Apply(1,0,0).Contains("pipeline"),"unsupported render pipeline rejected");
        Console.WriteLine(checks+" chams lifecycle checks passed");
    }
}
