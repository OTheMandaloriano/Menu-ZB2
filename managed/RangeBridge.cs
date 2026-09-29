using System;
using System.Collections.Generic;
using System.Reflection.Emit;
using System.Runtime.InteropServices;
using HarmonyLib;
using UnityEngine;

namespace Zb2Menu {
    // Extends only the selected aim candidates through the game's normal LOD pass.
    // ESP reads dormant records without spawning them or inventing health/bones.
    public static class RangeBridge {
        const int CandidateLimit=8, LoadsPerPass=2;
        static readonly HashSet<int> wanted=new HashSet<int>();
        static readonly HashSet<int> promoted=new HashSet<int>();
        struct Candidate { public int Id; public float Score; }
        static readonly List<Candidate> candidates=new List<Candidate>(CandidateLimit);
        static ZombieLoader loader;
        static Camera aimCamera;
        static Vector3 aimOrigin,aimForward;
        static bool enabled, fullCircle;
        static float range=120, radius=360, width=1280, height=720, updated;
        static int loads;
        static string error="";
        public static string LastError(){return error;}
        public static string Statistics(){return "aimRange="+range+" candidates="+wanted.Count+" promoted="+promoted.Count+
            (loader==null ? "" : " real="+loader.zombies.Count+" prop="+loader.zombieProps.Count+" unloaded="+loader.unloadedZombies.Count+" defaultSleep="+loader.sleepDistance);}
        public static void Install(Harmony harmony) {
            var update=AccessTools.Method(typeof(ZombieLoader),"UpdateLoadState",Type.EmptyTypes);
            if(update==null) throw new MissingMethodException("ZombieLoader.UpdateLoadState");
            harmony.Patch(update,prefix:new HarmonyMethod(typeof(RangeBridge),"BeforeLoading"),
                transpiler:new HarmonyMethod(typeof(RangeBridge),"PatchLoading"));
        }
        public static void Configure(int active,float distance,int circle,float pixels,float viewportWidth,float viewportHeight) {
            enabled=active!=0; range=Limit(distance); fullCircle=circle!=0;
            radius=pixels; width=viewportWidth; height=viewportHeight; updated=Time.unscaledTime;
            if(!enabled){wanted.Clear();promoted.Clear();}
        }
        static float Limit(float value) {
            return float.IsNaN(value) || float.IsInfinity(value) ? 10 : Math.Max(10,Math.Min(500,value));
        }
        static void BeforeLoading(ZombieLoader __instance) {
            wanted.Clear();candidates.Clear();loads=0;
            if(loader!=__instance){loader=__instance;promoted.Clear();}
            if(!enabled || Time.unscaledTime-updated>.25f || MainCamera.instance==null || MainCamera.instance.cam==null) return;
            try {
                aimCamera=MainCamera.instance.cam;aimOrigin=aimCamera.transform.position;aimForward=aimCamera.transform.forward;
                foreach(var zombie in loader.zombies)
                    if(zombie!=null && zombie.obj!=null && promoted.Contains(zombie.identity.id))
                        Consider(zombie.identity.id,zombie.obj.transform.position);
                foreach(var zombie in loader.unloadedZombies)
                    if(zombie!=null) Consider(zombie.identity.id,zombie.transform.position);
                foreach(var zombie in loader.zombieProps)
                    if(zombie!=null) Consider(zombie.identity.id,zombie.transform.position);
                foreach(var candidate in candidates) wanted.Add(candidate.Id);
                promoted.IntersectWith(wanted);
                error="";
            } catch(Exception ex){wanted.Clear();error=ex.ToString();}
        }
        static void Consider(int id,Vector3 point) {
            var camera=aimCamera;
            float distance=Vector3.Distance(aimOrigin,point);
            if(!(distance<=range)) return;
            float score;
            if(fullCircle) score=Vector3.Angle(aimForward,point-aimOrigin);
            else {
                var screen=camera.WorldToViewportPoint(point);
                if(!(screen.z>.01f && screen.x>=0 && screen.x<=1 && screen.y>=0 && screen.y<=1)) return;
                float dx=(screen.x-.5f)*width,dy=(screen.y-.5f)*height;
                score=(float)Math.Sqrt(dx*dx+dy*dy);
                if(radius>0 && score>radius) return;
            }
            if(float.IsNaN(score) || float.IsInfinity(score)) return;
            var candidate=new Candidate{Id=id,Score=score};
            if(candidates.Count<CandidateLimit){candidates.Add(candidate);return;}
            int worst=0;
            for(int i=1;i<candidates.Count;++i)if(candidates[i].Score>candidates[worst].Score)worst=i;
            if(score<candidates[worst].Score)candidates[worst]=candidate;
        }
        // Exact indexing audited in ZombieLoader.UpdateLoadState (this game SHA).
        static ZombieLoader.ZombieLoadState TargetState(ZombieLoader owner,ZombieLoader.ZombieLoadState current,
            int index,Zombie real,bool wave,out bool goActive) {
            var original=owner.GetTargetZombieState(current,index,real,wave,out goActive);
            if(original==ZombieLoader.ZombieLoadState.Real) return original;
            try {
                int id;
                if(current==ZombieLoader.ZombieLoadState.Unloaded) id=owner.unloadedZombies[index].identity.id;
                else if(current==ZombieLoader.ZombieLoadState.Prop) id=owner.zombieProps[index-owner.unloadedZombies.Count].identity.id;
                else {if(real==null)return original;id=real.identity.id;}
                if(MagnetLoadLease.Contains(owner,id))return ZombieLoader.ZombieLoadState.Real;
                if(!wanted.Contains(id)) return original;
                if(current!=ZombieLoader.ZombieLoadState.Real) {
                    if(loads>=LoadsPerPass)return original;
                    ++loads;promoted.Add(id);
                }
                // Preserve goActive, wave status, damage, networking and all normal spawns.
                return ZombieLoader.ZombieLoadState.Real;
            }catch(Exception ex){error=ex.ToString();return original;}
        }
        static IEnumerable<CodeInstruction> PatchLoading(IEnumerable<CodeInstruction> instructions) {
            var original=AccessTools.Method(typeof(ZombieLoader),"GetTargetZombieState");
            var replacement=AccessTools.Method(typeof(RangeBridge),"TargetState");
            var result=new List<CodeInstruction>(instructions);int count=0;
            foreach(var code in result)if(code.Calls(original)){code.opcode=OpCodes.Call;code.operand=replacement;++count;}
            if(count!=3)throw new InvalidOperationException("UpdateLoadState: esperado 3 caminhos LOD; encontrado "+count);
            return result;
        }
        [StructLayout(LayoutKind.Sequential,Pack=4)]
        public struct Marker {public float X,Y,Distance;public int Type,State;}
        static readonly List<Marker> markers=new List<Marker>(256);
        public static int Collect(IntPtr buffer,int capacity,float distance) {
            markers.Clear();
            var owner=ZombieLoader.Instance;
            if(buffer==IntPtr.Zero || capacity<=0 || owner==null || MainCamera.instance==null || MainCamera.instance.cam==null)return 0;
            float limit=Limit(distance);
            try {
                foreach(var zombie in owner.unloadedZombies)
                    if(zombie!=null) AddMarker(zombie.transform.position,(int)zombie.identity.type,0,limit);
                foreach(var zombie in owner.zombieProps)
                    if(zombie!=null) AddMarker(zombie.transform.position,(int)zombie.identity.type,1,limit);
            }catch(Exception ex){error=ex.ToString();}
            int count=Math.Min(capacity,markers.Count),size=Marshal.SizeOf(typeof(Marker));
            for(int i=0;i<count;++i)Marshal.StructureToPtr(markers[i],IntPtr.Add(buffer,i*size),false);
            return count;
        }
        static void AddMarker(Vector3 position,int type,int state,float limit) {
            var camera=MainCamera.instance.cam;
            float distance=Vector3.Distance(camera.transform.position,position);
            if(!(distance<=limit))return;
            var point=camera.WorldToViewportPoint(position);
            if(!(point.z>.01f && point.x>=0 && point.x<=1 && point.y>=0 && point.y<=1))return;
            var marker=new Marker{X=point.x,Y=1-point.y,Distance=distance,Type=type,State=state};
            if(markers.Count<256){markers.Add(marker);return;}
            int worst=0;
            for(int i=1;i<markers.Count;++i)if(markers[i].Distance>markers[worst].Distance)worst=i;
            if(distance<markers[worst].Distance)markers[worst]=marker;
        }
    }
}
