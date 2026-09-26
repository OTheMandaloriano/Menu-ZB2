using System;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.InteropServices;
using HarmonyLib;
using UnityEngine;

namespace Zb2Menu {
    // Patched callbacks execute in the game's firing thread. Publish does no Unity work.
    public static class AimBridge {
        const string PatchId = "zb2.menu.aim.v2";
        const int Silent = 1, AutoFire = 2, Trigger = 4;
        static readonly object Gate = new object();
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
            lock (Gate) {
                if (harmony != null) return true;
                var candidate = new Harmony(PatchId);
                try {
                    var shoot = AccessTools.Method(typeof(PhysicalGun), "Shoot", new[] {
                        typeof(PlayerMain), typeof(int), typeof(ShotPath), typeof(bool), typeof(DatabaseGun) });
                    var input = AccessTools.Method(typeof(PlayerArms), "ReadFireInput", new[] { typeof(InventoryItem) });
                    if (shoot == null || input == null) throw new MissingMethodException("Assinatura de disparo mudou.");
                    candidate.Patch(shoot, prefix: new HarmonyMethod(typeof(AimBridge), "BeforeShoot"));
                    candidate.Patch(input, postfix: new HarmonyMethod(typeof(AimBridge), "AfterFireInput"));
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
            lock (Gate) { if (harmony != null) harmony.UnpatchAll(PatchId); harmony = null; }
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
            int count = Physics.RaycastNonAlloc(from, direction, hits, distance, ~0, QueryTriggerInteraction.Ignore);
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
                (collider == null || Belongs(collider.transform, state.Target.obj.transform));
        }
        static bool SilentReady(Request state, ref ShotPath path) {
            // Online synchronization has a separate path: do not claim support for it.
            if (MultiplayerController.instance == null || !MultiplayerController.instance.IsSinglePlayer ||
                !Alive(state.Target) || state.Bone == null || !Belongs(state.Bone, state.Target.obj.transform)) return false;
            Vector3 point = state.Bone.position;
            if (!ClearToTarget(state, path.bulletOrigin, point) || !ClearToTarget(state, path.convergingOrigin, point)) return false;
            if (state.Player.cam == null || state.Player.cam.CameraTransform == null ||
                !ClearToTarget(state, state.Player.cam.CameraTransform.position, point)) return false;
            path.convergingDirection = (point-path.convergingOrigin).normalized;
            return true;
        }
        static void BeforeShoot(PlayerMain __0, ref ShotPath __2, bool __3) {
            try {
                Request state;
                if (!Current(out state) || (state.Flags & Silent)==0 || !__3 || __0 != state.Player) return;
                if (SilentReady(state, ref __2)) System.Threading.Interlocked.Increment(ref redirected);
                else System.Threading.Interlocked.Increment(ref blocked);
            } catch (Exception ex) { RecordFailure(ex); }
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
