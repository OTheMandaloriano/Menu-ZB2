using System;
using UnityEngine;
using UnityEngine.Rendering;

namespace Zb2Menu {
    public static class ChamsBridge {
        static Camera camera;
        static CommandBuffer commands;
        static Material visible,hidden;
        public static string Error="";
        static Color ColorOf(int packed){uint v=(uint)packed;return new Color32((byte)v,(byte)(v>>8),(byte)(v>>16),(byte)(v>>24));}
        static Material Create(Shader shader,CompareFunction depth) {
            var material=new Material(shader){hideFlags=HideFlags.HideAndDontSave};
            material.SetInt("_SrcBlend",(int)BlendMode.SrcAlpha);material.SetInt("_DstBlend",(int)BlendMode.OneMinusSrcAlpha);
            material.SetInt("_Cull",(int)CullMode.Off);material.SetInt("_ZWrite",0);material.SetInt("_ZTest",(int)depth);return material;
        }
        public static string Apply(int enabled,int front,int behind) {
            if(enabled==0){Clear();Error="";return Error;}
            try {
                var next=MainCamera.instance==null?null:MainCamera.instance.cam;
                if(next==null || ZombieLoader.Instance==null){Clear();return "Chams: aguardando camera";}
                if(GraphicsSettings.currentRenderPipeline!=null){Clear();return "Chams: pipeline de renderizacao nao suportado";}
                if(next!=camera)Clear();
                if(commands==null) {
                    var shader=Shader.Find("Hidden/Internal-Colored");
                    if(shader==null || !shader.isSupported)return "Chams: shader compativel indisponivel";
                    visible=Create(shader,CompareFunction.LessEqual);hidden=Create(shader,CompareFunction.Greater);
                    if(!visible.HasProperty("_ZTest") || !visible.HasProperty("_Color")){Clear();return "Chams: shader sem controle de profundidade/cor";}
                    commands=new CommandBuffer{name="ZB2Menu Chams"};camera=next;
                    camera.AddCommandBuffer(CameraEvent.AfterForwardAlpha,commands);
                }
                visible.SetColor("_Color",ColorOf(front));hidden.SetColor("_Color",ColorOf(behind));commands.Clear();
                int count=0;
                foreach(var zombie in ZombieLoader.Instance.zombies) {
                    if(zombie==null || zombie.obj==null || zombie.health==null || !zombie.health.isAlive || zombie.health.amount<=0)continue;
                    var renderer=zombie.obj.meshRenderer;
                    if(renderer==null || !renderer.enabled || renderer.sharedMesh==null)continue;
                    if(count++>=128)break;
                    for(int sub=0;sub<Math.Min(renderer.sharedMesh.subMeshCount,8);++sub){commands.DrawRenderer(renderer,hidden,sub,0);commands.DrawRenderer(renderer,visible,sub,0);}
                }
                Error="";
            }catch(Exception ex){Clear();Error="Chams: "+ex.GetType().Name;}
            return Error;
        }
        public static void Clear(){
            if(camera!=null && commands!=null)camera.RemoveCommandBuffer(CameraEvent.AfterForwardAlpha,commands);
            if(commands!=null)commands.Release();commands=null;camera=null;
            if(visible!=null)UnityEngine.Object.Destroy(visible);if(hidden!=null)UnityEngine.Object.Destroy(hidden);visible=hidden=null;
        }
    }
}
