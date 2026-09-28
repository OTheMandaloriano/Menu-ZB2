using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using UnityEngine;
namespace Zb2Menu {
    public static class ZombieRigBridge {
        sealed class Rig {public Transform[] Points=new Transform[4];public Renderer[] Renderers;public float Retry;}
        static readonly ConditionalWeakTable<ZombieObject,Rig> cache=new ConditionalWeakTable<ZombieObject,Rig>();
        static readonly HumanBodyBones[] human={HumanBodyBones.Head,HumanBodyBones.Neck,HumanBodyBones.Chest,HumanBodyBones.Hips};
        static readonly string[][] aliases={new[]{"head","bip001head","mixamorighead"},new[]{"neck","bip001neck","mixamorigneck"},new[]{"chest","sp2","spine2","bip001spine2","mixamorigspine2"},new[]{"hips","pelvis","hl","bip001pelvis","mixamorighips"}};
        static string Key(string name){return name.Replace("_","").Replace(" ","").Replace(":","").ToLowerInvariant();}
        static Rig Get(ZombieObject obj) {
            var rig=cache.GetOrCreateValue(obj);
            if(Time.unscaledTime<rig.Retry)return rig;
            rig.Retry=Time.unscaledTime+2;
            rig.Renderers=obj.GetComponentsInChildren<Renderer>(true);
            var animator=obj.animator;
            for(int i=0;i<4;++i)rig.Points[i]=animator!=null && animator.isHuman?animator.GetBoneTransform(human[i]):null;
            var bones=obj.GetComponentsInChildren<Transform>(true);
            for(int i=0;i<4;++i)if(rig.Points[i]==null)
                foreach(var bone in bones){if(bone==null)continue;string key=Key(bone.name);if(Array.IndexOf(aliases[i],key)>=0){rig.Points[i]=bone;break;}}
            return rig;
        }
        public static Transform Bone(ZombieObject obj,int selection) {
            if(obj==null || selection<0 || selection>3)return null;
            var point=Get(obj).Points[selection];
            // Eyes are an acceptable head reference, never a substitute for neck/chest/pelvis.
            return point!=null?point:selection==0?obj.zombieEyeRef:null;
        }
        public static bool Bounds(ZombieObject obj,IntPtr output) {
            if(obj==null || output==IntPtr.Zero)return false;
            var rig=Get(obj);UnityEngine.Bounds bounds=new UnityEngine.Bounds();bool found=false;
            foreach(var renderer in rig.Renderers){
                if(renderer==null || !renderer.enabled || !renderer.gameObject.activeInHierarchy)continue;
                if(!(renderer is SkinnedMeshRenderer) && !(renderer is MeshRenderer))continue;
                var b=renderer.bounds;
                if(float.IsNaN(b.extents.sqrMagnitude)||float.IsInfinity(b.extents.sqrMagnitude)||b.extents.sqrMagnitude<=0)continue;
                if(!found){bounds=b;found=true;}else bounds.Encapsulate(b);
            }
            if(!found)return false;
            Marshal.Copy(new[]{bounds.center.x,bounds.center.y,bounds.center.z,bounds.extents.x,bounds.extents.y,bounds.extents.z},0,output,6);
            return true;
        }
    }
}
