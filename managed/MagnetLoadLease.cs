using System.Collections.Generic;
using UnityEngine;
namespace Zb2Menu {
    // Keep promoted models alive until the Magnet has processed them, independently of aim FOV.
    public static class MagnetLoadLease {
        static readonly HashSet<int> ids=new HashSet<int>();
        static ZombieLoader owner;
        static float updated;
        public static void Pulse(ZombieLoader loader){if(owner!=loader){ids.Clear();owner=loader;}updated=Time.unscaledTime;}
        public static void Retain(ZombieLoader loader,int id){Pulse(loader);ids.Add(id);}
        public static bool Contains(ZombieLoader loader,int id){return owner==loader && Time.unscaledTime-updated<1 && ids.Contains(id);}
        public static void Clear(){ids.Clear();owner=null;}
    }
}
