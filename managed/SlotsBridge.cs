using System;
using HarmonyLib;

namespace Zb2Menu {
    public static class SlotsBridge {
        static PlayerInventory inventory;
        static IntVec2 usable;
        static int misc;
        static bool storageFlag,miscFlag;
        public static string Status="";
        public static bool Pending {get{return inventory!=null;}}
        static void Flags(PlayerInventory target,bool storage,bool slots) {
            AccessTools.Property(typeof(PlayerInventory),"TotalStorageUnlocked").SetValue(target,storage,null);
            AccessTools.Property(typeof(PlayerInventory),"TotalMiscSlotsUnlocked").SetValue(target,slots,null);
        }
        public static void Apply(PlayerMain player,bool enabled) {
            var current=player==null ? null : player.inventory;
            if(inventory!=null && (current!=inventory || !enabled)) {
                if(!Restore())return;
            }
            if(!enabled || current==null || current.storage==null || current.equippedItems==null)return;
            if(inventory==null) {
                inventory=current;usable=current.storage.UsableSize;
                misc=current.equippedItems.UnlockedMiscSlotsCount;
                storageFlag=current.TotalStorageUnlocked;miscFlag=current.TotalMiscSlotsUnlocked;
            }
            var total=current.storage.TotalSize;
            if(total.x<=0 || total.y<=0)throw new InvalidOperationException("Inventario sem dimensoes validas");
            bool changed=current.storage.UsableSize.x!=total.x || current.storage.UsableSize.y!=total.y ||
                current.equippedItems.UnlockedMiscSlotsCount!=current.equippedItems.MiscCount || !current.TotalStorageUnlocked || !current.TotalMiscSlotsUnlocked;
            if(changed) {
                current.storage.SetUsableSize(total.x,total.y);
                current.equippedItems.SetUnlockedMiscSlotsCount(current.equippedItems.MiscCount);
                Flags(current,true,true);current.OnEquipmentChanged();
            }
            Status="Slots existentes desbloqueados";
        }
        public static bool Restore() {
            if(inventory==null){inventory=null;Status="";return true;}
            var storage=inventory.storage;var equipped=inventory.equippedItems;
            if(storage==null || equipped==null){inventory=null;Status="";return true;}
            foreach(var item in storage.items) {
                if(item==null || item.IsEmpty())continue;
                var size=item.GetDataBaseItem().Size(item.rotated);
                if(item.pos.x+size.x>usable.x || item.pos.y+size.y>usable.y) {
                    Status="Para restaurar: mova os itens da area extra";return false;
                }
            }
            for(int i=Math.Max(0,misc);i<equipped.MiscCount;++i)
                if(!equipped.GetMisc(i).IsEmpty()){Status="Para restaurar: esvazie os slots extras";return false;}
            storage.SetUsableSize(usable.x,usable.y);
            equipped.SetUnlockedMiscSlotsCount(Math.Min(misc,equipped.MiscCount));
            Flags(inventory,storageFlag,miscFlag);inventory.OnEquipmentChanged();inventory=null;Status="";return true;
        }
    }
}
