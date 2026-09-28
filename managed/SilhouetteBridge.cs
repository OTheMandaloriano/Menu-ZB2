using System;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityEngine.Rendering;

namespace Zb2Menu {
    // Unity owns the mask and command buffer. Present only consumes a retained GPU view.
    public static class SilhouetteBridge {
        [DllImport("kiero-dx11-base.dll", CallingConvention=CallingConvention.Cdecl)]
        static extern int Zb2PublishOutlineMask(IntPtr texture);
        internal static Func<IntPtr,int> PublishMask=Zb2PublishOutlineMask;
        public static string Failure="";
        static Camera camera;
        static RenderTexture mask;
        static CommandBuffer commands;
        static Material visible, hidden;

        static Material Create(Shader shader, CompareFunction depth, Color color) {
            var material = new Material(shader) { hideFlags=HideFlags.HideAndDontSave };
            material.SetInt("_SrcBlend", (int)BlendMode.One);
            material.SetInt("_DstBlend", (int)BlendMode.One);
            material.SetInt("_Cull", (int)CullMode.Off);
            material.SetInt("_ZWrite", 0);
            material.SetInt("_ZTest", (int)depth);
            material.SetColor("_Color", color);
            return material;
        }

        public static string Apply(int enabled) {
            if (enabled==0) { Clear(); Failure=""; return ""; }
            try {
                var next=MainCamera.instance==null ? null : MainCamera.instance.cam;
                if (next==null || ZombieLoader.Instance==null) { Clear(); return "Silhueta: aguardando camera"; }
                if (GraphicsSettings.currentRenderPipeline!=null || SystemInfo.graphicsDeviceType!=GraphicsDeviceType.Direct3D11) {
                    Clear(); return "Silhueta: requer pipeline padrao D3D11";
                }
                // A single-sample color target cannot share a multisampled scene depth buffer.
                if ((next.allowMSAA && QualitySettings.antiAliasing>1) || next.targetTexture!=null || next.stereoEnabled) {
                    Clear(); return "Silhueta: camera com MSAA, alvo externo ou VR nao suportada";
                }
                int width=next.pixelWidth, height=next.pixelHeight;
                if (width<1 || height<1) { Clear(); return "Silhueta: camera sem tamanho"; }
                if (camera!=next || mask==null || mask.width!=width || mask.height!=height) Clear();
                if (commands==null) Initialize(next,width,height);
                Failure="";return "";
            } catch (Exception error) { Clear(); Failure="Silhueta: "+error.GetType().Name+" ("+error.Message+")";return Failure; }
        }

        static void RefreshCamera(Camera current) {
            if(current!=camera || commands==null)return;
            try {RefreshCommands();}catch(Exception error){Clear();Failure="Silhueta: "+error.Message;}
        }
        static void RefreshCommands() {
                commands.Clear();
                commands.SetRenderTarget(new RenderTargetIdentifier(mask), new RenderTargetIdentifier(BuiltinRenderTextureType.CameraTarget));
                commands.ClearRenderTarget(false,true,Color.clear);
                commands.SetViewport(new Rect(0,0,camera.pixelWidth,camera.pixelHeight));
                commands.SetViewProjectionMatrices(camera.worldToCameraMatrix,camera.projectionMatrix);
                int count=0;
                foreach (var zombie in ZombieLoader.Instance.zombies) {
                    if (zombie==null || zombie.obj==null || zombie.health==null || !zombie.health.isAlive || zombie.health.amount<=0) continue;
                    var renderer=zombie.obj.meshRenderer;
                    if (renderer==null || !renderer.enabled || renderer.sharedMesh==null) continue;
                    if (count++>=128) break;
                    for (int sub=0; sub<Math.Min(renderer.sharedMesh.subMeshCount,8); ++sub) {
                        commands.DrawRenderer(renderer,hidden,sub,0);
                        commands.DrawRenderer(renderer,visible,sub,0);
                    }
                }
        }
        static void Initialize(Camera next,int width,int height) {
            var shader=Shader.Find("Hidden/Internal-Colored");
            if (shader==null || !shader.isSupported) throw new NotSupportedException("shader");
            visible=Create(shader,CompareFunction.LessEqual,Color.red);
            hidden=Create(shader,CompareFunction.Greater,Color.green);
            if (!visible.HasProperty("_ZTest") || !visible.HasProperty("_Color")) throw new NotSupportedException("shader properties");
            mask=new RenderTexture(width,height,0,RenderTextureFormat.ARGB32,RenderTextureReadWrite.Linear) {
                name="ZB2 silhouette mask", hideFlags=HideFlags.HideAndDontSave,
                filterMode=FilterMode.Point, wrapMode=TextureWrapMode.Clamp, antiAliasing=1
            };
            if (!mask.Create() || PublishMask(mask.GetNativeTexturePtr())==0) throw new NotSupportedException("GPU view");
            camera=next;
            commands=new CommandBuffer { name="ZB2 silhouette mask" };
            camera.AddCommandBuffer(CameraEvent.AfterForwardAlpha,commands);
            Camera.onPreRender+=RefreshCamera;
        }

        public static void Clear() {
            Camera.onPreRender-=RefreshCamera;
            if (camera!=null && commands!=null) camera.RemoveCommandBuffer(CameraEvent.AfterForwardAlpha,commands);
            if (commands!=null) commands.Release();
            commands=null; camera=null;
            if (mask!=null) {
                PublishMask(IntPtr.Zero);
                mask.Release(); UnityEngine.Object.Destroy(mask); mask=null;
            }
            if (visible!=null) UnityEngine.Object.Destroy(visible);
            if (hidden!=null) UnityEngine.Object.Destroy(hidden);
            visible=hidden=null;
        }
    }

    public static class VisualEffectsBridge {
        public static string Apply(int chams,int front,int behind,int silhouette) {
            string a=ChamsBridge.Apply(chams,front,behind), b=SilhouetteBridge.Apply(silhouette);
            return a.Length==0 ? b : (b.Length==0 ? a : a+" | "+b);
        }
    }
}
