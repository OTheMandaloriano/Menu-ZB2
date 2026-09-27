using System;
using System.Collections;
using System.Collections.Generic;
using System.Reflection;
using HarmonyLib;
using UnityEngine;

namespace Zb2Menu {
    public static class ModifierBridge {
        static readonly OriginalValues values = new OriginalValues();
        static readonly Dictionary<string, MemberInfo> members = new Dictionary<string, MemberInfo>();
        static PlayerMain previous;
        static string lastError = "";
        public static string LastError() { return lastError; }
        public static bool CanUseKeys() {return !GlobalTexting.IsTexting && !DeveloperConsole.Opened && Application.isFocused && Time.timeScale>0;}
        public static string Status() {
            var messages=new List<string>();
            if(SlotsBridge.Status.StartsWith("Para restaurar:"))messages.Add(SlotsBridge.Status);
            if(NoClipBridge.Status.Length>0)messages.Add(NoClipBridge.Status);
            if(MagnetBridge.Status.Length>0)messages.Add(MagnetBridge.Status);
            return string.Join(" | ",messages.ToArray());
        }
        static bool Exists(object target) {
            return target != null && (!(target is UnityEngine.Object) || (UnityEngine.Object)target != null);
        }
        static MemberInfo Member(object target, string name) {
            string key = target.GetType().FullName + ":" + name;
            MemberInfo member;
            if (!members.TryGetValue(key, out member)) {
                member = (MemberInfo)AccessTools.Field(target.GetType(), name) ?? AccessTools.Property(target.GetType(), name);
                if (member == null) throw new MissingMemberException(key);
                members.Add(key, member);
            }
            return member;
        }
        static object Read(object target, MemberInfo member) {
            var field = member as FieldInfo;
            return field != null ? field.GetValue(target) : ((PropertyInfo)member).GetValue(target, null);
        }
        static void Write(object target, MemberInfo member, object value) {
            if (!Exists(target)) return; // Destroyed Unity objects cannot affect a new scene.
            var field = member as FieldInfo;
            if (field != null) field.SetValue(target, value);
            else ((PropertyInfo)member).SetValue(target, value, null);
        }
        static void Change<T>(object target, string name, Func<T,T> change) {
            if (!Exists(target)) return;
            var member = Member(target, name);
            values.Apply(target, name, () => (T)Read(target, member), value => Write(target, member, value), change);
        }
        static float Mult(float value, float maximum) {
            return float.IsNaN(value) || float.IsInfinity(value) ? 1 : Math.Max(1, Math.Min(maximum, value));
        }
        static void Scale(object target, string name, float multiplier) {
            Change<float>(target, name, original => original * multiplier);
        }
        public static bool Reset() {
            try { values.RestoreAll(); SlotsBridge.Restore(); NoClipBridge.Restore(); MagnetBridge.Reset(); previous=null; lastError=""; }
            catch (Exception ex) { lastError=ex.ToString(); }
            return values.Count != 0 || SlotsBridge.Pending || NoClipBridge.Active || MagnetBridge.Active;
        }
        // Called only by the native callback inside ZBMain.Update, including all-off frames.
        public static bool Apply(PlayerMain player, int flags, float rapid, float speed, float jump, float roll, float knife, float noclip,float magnetRadius,int magnetTargets) {
            try {
                if (!ReferenceEquals(previous, player)) { values.RestoreAll(); previous=player; }
                values.Begin();
                bool alive=player!=null && player.HasLocalControl && player.healthFast>0;
                MagnetBridge.Apply(alive?player:null,alive && (flags&8192)!=0,(flags&4096)==0,magnetRadius,magnetTargets);
                SlotsBridge.Apply(alive?player:null,alive && (flags&1024)!=0);
                float normalWalk=alive && player.movement!=null ? values.ReadOriginal(player.movement,"walkSpeed",()=>player.movement.walkSpeed) : 0;
                NoClipBridge.Apply(alive?player:null,alive && (flags&2048)!=0,noclip,(flags&4096)==0,normalWalk);
                if (player != null && player.HasLocalControl && player.healthFast > 0) {
                    Movement(player, flags, Mult(speed,5), Mult(jump,10), Mult(roll,5));
                    Weapons(player, flags, Mult(rapid,5));
                    if ((flags & 256) != 0) Melee(Mult(knife,5));
                    if ((flags & 4) != 0 && WeaponBase.instance != null)
                        Scale(WeaponBase.instance, "gunSway", 0);
                    if ((flags & 8) != 0) TightCrosshair();
                }
                values.End();
                lastError="";
            } catch (Exception ex) {
                lastError=ex.ToString();
                // A failed chain must not leave other disabled features applied.
                try { values.End(); } catch (Exception restore) { lastError += "\n" + restore; }
            }
            return values.Count != 0 || SlotsBridge.Pending || NoClipBridge.Active || MagnetBridge.Active;
        }
        static void Movement(PlayerMain player, int flags, float speed, float jump, float roll) {
            var movement=player.movement;
            if ((flags & 32)!=0) Scale(movement,"walkSpeed",speed);
            if ((flags & 64)!=0) { Scale(movement,"jumpSpeed",jump); Scale(movement,"fallDamageThreshold",jump); }
            if ((flags & 128)!=0) Scale(movement,"rollSpeed",roll);
        }
        static void Weapons(PlayerMain player, int flags, float rapid) {
            if ((flags & (1|2|8|16|512))==0 || player.inventory == null || player.inventory.equippedItems == null) return;
            var equipment=player.inventory.equippedItems;
            var weapons=Read(equipment,Member(equipment,"weapons")) as IEnumerable;
            if(weapons==null) return;
            foreach (InventoryItem item in weapons) {
                if (item == null) continue;
                var gun=item.GetDataBaseItem() as DatabaseGun;
                if (gun == null) continue;
                if ((flags & (1|8))!=0) {
                    Change<Vector2>(gun,"recoil", original => Vector2.zero);
                    Scale(gun,"recoilRandomness",0);
                }
                if ((flags & (2|8))!=0) Scale(gun,"spread",0);
                if ((flags & 16)!=0) Scale(gun,"rof",rapid);
                if ((flags & 512)!=0) {
                    Change<bool>(gun,"fullAuto", original => true);
                    Change<int>(gun,"burstCount", original => 0);
                }
            }
        }
        static void Melee(float multiplier) {
            var owner=MeleeAttackBase.Instance;
            if (owner == null) return;
            var attacks=Read(owner, Member(owner,"AllAttacks")) as IDictionary;
            if (attacks == null) return;
            foreach (object attack in attacks.Values) Scale(attack,"Duration",1/multiplier);
        }
        static void TightCrosshair() {
            var hud=PlayerHUD.instance;
            if(hud==null) return;
            var transform=Read(hud,Member(hud,"innerCrossHairTransform")) as Transform;
            if(transform!=null) Change<Vector3>(transform,"localScale", original => original*.5f);
        }
    }
}
