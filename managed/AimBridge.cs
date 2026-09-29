using System;
using System.Diagnostics;
using System.Reflection;
using System.Reflection.Emit;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using HarmonyLib;
using UnityEngine;

namespace Zb2Menu {
    // Patched callbacks execute in the game's firing thread. Publish does no Unity work.
    public static class AimBridge {
        const string PatchId = "zb2.menu.aim.v2";
        const int Silent = 1, AutoFire = 2, Trigger = 4;
        static readonly object Gate = new object();
        static readonly object InstallationGate = new object();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void NativeUpdate();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate int NativeUiState();
        static NativeUpdate nativeUpdate;
        static NativeUiState nativeUi;
        [ThreadStatic] static RaycastHit[] hits;
        static Harmony harmony;
        static Request request;
        static string error = "";
        static long redirected, blocked, automatic;
        struct Request {
            public PlayerMain Player;
            public Zombie Target;
            public Transform Bone;
            public int Flags;
            public float Range;
            public long Published;
        }
        [DllImport("user32.dll")] static extern IntPtr GetForegroundWindow();
        [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr hwnd, out uint process);
        [DllImport("kernel32.dll")] static extern uint GetCurrentProcessId();

        public static bool Install() {
            lock (InstallationGate) {
                if (harmony != null) return true;
                var candidate = new Harmony(PatchId);
                try {
                    var shoot = AccessTools.Method(typeof(PlayerArms), "ShootGun", new[] { typeof(InventoryItem) });
                    var input = AccessTools.Method(typeof(PlayerArms), "ReadFireInput", new[] { typeof(InventoryItem) });
                    var update = AccessTools.Method(typeof(ZBMain), "Update", Type.EmptyTypes);
                    if (shoot == null || input == null || update == null) throw new MissingMethodException("Assinatura do jogo mudou.");
                    candidate.Patch(shoot, transpiler: new HarmonyMethod(typeof(AimBridge), "PatchShotPath"));
                    candidate.Patch(input, postfix: new HarmonyMethod(typeof(AimBridge), "AfterFireInput"));
                    candidate.Patch(update, postfix: new HarmonyMethod(typeof(AimBridge), "AfterGameUpdate"));
                    RangeBridge.Install(candidate);
                    NoClipBridge.Install(candidate);
                    MagnetFreeze.Install(candidate);
                    GodModeBridge.Install(candidate);
                    CombatExtrasBridge.Install(candidate);
                    MenuInputBridge.Install(candidate);
                    harmony = candidate;
                    return true;
                } catch (Exception ex) {
                    candidate.UnpatchAll(PatchId);
                    error = ex.ToString();
                    return false;
                }
            }
        }
        public static string LastError() { lock (Gate) return error; }
        public static string Statistics() {
            return "redirected=" + System.Threading.Interlocked.Read(ref redirected) +
                " blocked=" + System.Threading.Interlocked.Read(ref blocked) +
                " autoSignals=" + System.Threading.Interlocked.Read(ref automatic);
        }
        public static void Publish(PlayerMain player, Zombie target, Transform bone, int flags, float range) {
            lock (Gate) request = new Request { Player=player, Target=target, Bone=bone,
                Flags=flags, Range=range, Published=Stopwatch.GetTimestamp() };
        }
        public static void Clear() { lock (Gate) request = new Request(); }
        public static void Shutdown() {
            Clear();
            lock (Gate) {nativeUpdate = null;nativeUi=null;}
            MenuInputBridge.Update(0);
            lock (InstallationGate) { if (harmony != null) harmony.UnpatchAll(PatchId); harmony = null; }
        }
        public static bool StartLoop(IntPtr callback,IntPtr uiCallback) {
            if (callback == IntPtr.Zero || !Install()) return false;
            var action = (NativeUpdate)Marshal.GetDelegateForFunctionPointer(callback, typeof(NativeUpdate));
            var ui=uiCallback==IntPtr.Zero?null:(NativeUiState)Marshal.GetDelegateForFunctionPointer(uiCallback,typeof(NativeUiState));
            lock (Gate) {nativeUpdate = action;nativeUi=ui;}
            return true;
        }
        static void AfterGameUpdate() {
            NativeUpdate action;
            NativeUiState ui;
            lock (Gate) {action = nativeUpdate;ui=nativeUi;}
            try {MenuInputBridge.Update(ui==null?0:ui());}catch(Exception ex){RecordFailure(ex);}
            if (action == null || Time.timeScale <= 0) return;
            try { action(); }
            catch (Exception ex) { lock (Gate) nativeUpdate = null; RecordFailure(ex); }
        }
        static bool Current(out Request state) {
            lock (Gate) state = request;
            double age = (Stopwatch.GetTimestamp()-state.Published)/(double)Stopwatch.Frequency;
            return state.Flags != 0 && age >= 0 && age <= .15 && Focused() &&
                Time.timeScale > 0 && state.Player != null && state.Player.HasLocalControl && state.Player.healthFast > 0;
        }
        static bool Focused() {
#if AIM_TESTS
            return TestEnvironment.Focused;
#else
            uint foreground;
            GetWindowThreadProcessId(GetForegroundWindow(), out foreground);
            return foreground == GetCurrentProcessId();
#endif
        }
        static bool Alive(Zombie zombie) {
            return zombie != null && zombie.obj != null && zombie.health != null &&
                zombie.health.isAlive && zombie.health.amount > 0;
        }
        static bool Belongs(Transform child, Transform root) {
            return child != null && root != null && (child == root || child.IsChildOf(root));
        }
        // NonAlloc results are unordered. Ignore the local player's colliders,
        // then choose the actual nearest obstacle. Saturation fails closed.
        static bool FirstHit(Request state, Vector3 from, Vector3 direction, float distance, out Collider collider) {
            collider = null;
            if (hits == null) hits = new RaycastHit[64];
            int mask=state.Player.arms==null?~0:(int)state.Player.arms.shotLayerMask;
            int count = Physics.RaycastNonAlloc(from, direction, hits, distance, mask, QueryTriggerInteraction.Ignore);
            if(count==hits.Length && hits.Length<1024){hits=new RaycastHit[hits.Length==64?256:1024];count=Physics.RaycastNonAlloc(from,direction,hits,distance,mask,QueryTriggerInteraction.Ignore);}
            if(count==hits.Length && hits.Length<1024){hits=new RaycastHit[1024];count=Physics.RaycastNonAlloc(from,direction,hits,distance,mask,QueryTriggerInteraction.Ignore);}
            if (count == hits.Length) return false;
            float nearest = float.PositiveInfinity;
            for (int i=0; i<count; ++i) {
                var hit = hits[i]; var item = hit.collider;
                if (item == null || Belongs(item.transform, state.Player.transform)) continue;
                if (hit.distance < nearest) { nearest = hit.distance; collider = item; }
            }
            return true;
        }
        public static bool Visible(PlayerMain player, Zombie target, Transform bone, float range) {
            try {
                if (player == null || player.cam == null || player.cam.CameraTransform == null ||
                    !Alive(target) || bone == null || !Belongs(bone, target.obj.transform)) return false;
                var state = new Request {Player=player, Target=target, Range=range};
                return ClearToTarget(state, player.cam.CameraTransform.position, bone.position);
            } catch (Exception ex) { RecordFailure(ex); return false; }
        }
        static bool ClearToTarget(Request state, Vector3 from, Vector3 point) {
            Vector3 delta = point-from;
            float distance = delta.magnitude;
            if (float.IsNaN(distance) || float.IsInfinity(distance) || distance < .01f || distance > state.Range) return false;
            Collider collider;
            return FirstHit(state, from, delta/distance, distance, out collider) &&
                (collider == null || Belongs(collider.transform, state.Target.obj.transform) || MagnetFreeze.ShareAnchor(state.Target,collider));
        }
        static bool SilentReady(Request state, ref ShotPath path) {
            if (!Alive(state.Target) || state.Bone == null || !Belongs(state.Bone, state.Target.obj.transform)) return false;
            Vector3 point = state.Bone.position;
            if (!ClearToTarget(state, path.bulletOrigin, point) || !ClearToTarget(state, path.convergingOrigin, point)) return false;
            if (state.Player.cam == null || state.Player.cam.CameraTransform == null ||
                !ClearToTarget(state, state.Player.cam.CameraTransform.position, point)) return false;
            path.convergingDirection = (point-path.convergingOrigin).normalized;
            return true;
        }
        // Pass the caller's ShotPath by reference. ShootGun later sends this exact
        // local through SyncShotOnline, including host broadcasts. No extra packet.
        public static double LastAimCheckMs,LastOriginalShotMs,PeakOriginalShotMs;
        static void ExecuteShot(PhysicalGun gun, PlayerMain player, int mask,
                                ref ShotPath path, bool effect, DatabaseGun custom) {
            long started=System.Diagnostics.Stopwatch.GetTimestamp();
            try {
                Request state;
                if (Current(out state) && (state.Flags & Silent)!=0 && effect && player == state.Player) {
                    if (SilentReady(state, ref path)) System.Threading.Interlocked.Increment(ref redirected);
                    else System.Threading.Interlocked.Increment(ref blocked);
                }
            } catch (Exception ex) { RecordFailure(ex); }
            LastAimCheckMs=(System.Diagnostics.Stopwatch.GetTimestamp()-started)*1000.0/System.Diagnostics.Stopwatch.Frequency;
            started=System.Diagnostics.Stopwatch.GetTimestamp();
            try{gun.Shoot(player, mask, path, effect, custom);}
            finally{LastOriginalShotMs=(System.Diagnostics.Stopwatch.GetTimestamp()-started)*1000.0/System.Diagnostics.Stopwatch.Frequency;if(LastOriginalShotMs>PeakOriginalShotMs)PeakOriginalShotMs=LastOriginalShotMs;}
        }
        static IEnumerable<CodeInstruction> PatchShotPath(IEnumerable<CodeInstruction> instructions) {
            var code = new List<CodeInstruction>(instructions);
            var shoot = AccessTools.Method(typeof(PhysicalGun), "Shoot", new[] {
                typeof(PlayerMain), typeof(int), typeof(ShotPath), typeof(bool), typeof(DatabaseGun) });
            int replaced = 0;
            for (int i=3; i<code.Count; ++i) {
                if (!code[i].Calls(shoot)) continue;
                if (code[i-1].opcode != OpCodes.Ldnull || code[i-2].opcode != OpCodes.Ldc_I4_1)
                    throw new InvalidOperationException("ShootGun: argumentos de disparo mudaram.");
                var load = code[i-3];
                int slot;
                if (load.opcode == OpCodes.Ldloc_0) slot=0;
                else if (load.opcode == OpCodes.Ldloc_1) slot=1;
                else if (load.opcode == OpCodes.Ldloc_2) slot=2;
                else if (load.opcode == OpCodes.Ldloc_3) slot=3;
                else if (load.opcode == OpCodes.Ldloc || load.opcode == OpCodes.Ldloc_S)
                    slot = load.operand is LocalBuilder ? ((LocalBuilder)load.operand).LocalIndex : Convert.ToInt32(load.operand);
                else throw new InvalidOperationException("ShootGun: ShotPath nao e uma variavel local.");
                var address = CodeInstruction.LoadLocal(slot, true);
                address.labels.AddRange(load.labels); address.blocks.AddRange(load.blocks);
                code[i-3] = address;
                code[i].opcode = OpCodes.Call;
                code[i].operand = AccessTools.Method(typeof(AimBridge), "ExecuteShot");
                ++replaced;
            }
            if (replaced != 1) throw new InvalidOperationException("ShootGun: esperado um unico disparo fisico.");
            return code;
        }
        static bool CrosshairEnemy(Request state) {
            if (state.Player.cam == null || state.Player.cam.CameraTransform == null) return false;
            Transform camera = state.Player.cam.CameraTransform;
            Collider collider;
            if (!FirstHit(state, camera.position, camera.forward, state.Range, out collider) || collider == null) return false;
            var zombie = collider.GetComponentInParent<ZombieObject>();
            return zombie != null && Alive(zombie.GetZombie);
        }
        static void AfterFireInput(PlayerArms __instance, InventoryItem __0) {
            try {
                Request state;
                if (!Current(out state) || (state.Flags & (AutoFire|Trigger))==0 ||
                    __instance != state.Player.arms || __0 == null || __0.ammo <= 0) return;
                var gun = __instance.EquippedGun;
                if (gun == null || gun.IsCoolingDown || gun.DbReference == null ||
                    gun.DbReference.gunClass == DatabaseGun.GunClass.ExplosivesLauncher) return;
                bool shoot = false;
                if ((state.Flags & (AutoFire|Silent)) == (AutoFire|Silent)) {
                    var camera = state.Player.cam.CameraTransform;
                    var path = new ShotPath(gun.barrel.position, gun.barrel.position, camera.position, camera.forward);
                    shoot = SilentReady(state, ref path);
                } else shoot = CrosshairEnemy(state);
                if (shoot) { gun.SetShootSignal(true); System.Threading.Interlocked.Increment(ref automatic); }
            } catch (Exception ex) { RecordFailure(ex); }
        }
        static void RecordFailure(Exception exception) {
            lock (Gate) { error = exception.ToString(); request = new Request(); }
        }
    }
}
