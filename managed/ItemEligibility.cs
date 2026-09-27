using System;
namespace Zb2Menu {
    public static class ItemEligibility {
        public static bool Allowed(DatabaseItem item) {
            return item!=null && item.itemID!=InventoryItem.ID.None && !item.hidden && !item.reserved && !Pricing.IsBlocked(item.itemID);
        }
        public static bool AllowedId(int id) {
            try {return ItemsBase.instance!=null && id>0 && id<ItemsBase.ItemCount && Allowed(ItemsBase.instance.GetItem((InventoryItem.ID)id));}
            catch {return false;}
        }
    }
}
