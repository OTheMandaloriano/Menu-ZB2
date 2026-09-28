using System;
using System.Collections.Generic;
using UnityEngine;

namespace Zb2Menu {
    // A click may target a building whose collision LOD is currently disabled.
    // On failure restore it; after a successful teleport normal proximity LOD owns it.
    public sealed class NearbyCollisionScope : IDisposable {
        readonly List<LODCollider> enabled=new List<LODCollider>();
        bool arrived;
        public NearbyCollisionScope(Vector3 destination) {
            try {
                foreach(var target in UnityEngine.Object.FindObjectsByType<LODTarget>(FindObjectsInactive.Include,FindObjectsSortMode.None)) {
                    if(target==null || target.lodCollider==null || target.lodCollider.colliding)continue;
                    if(Vector3.Distance(target.transform.position,destination)>16)continue;
                    if(enabled.Count>=256)throw new InvalidOperationException("Area com colisoes demais para verificar");
                    enabled.Add(target.lodCollider);target.lodCollider.SetColliding(true,true);
                }
                Physics.SyncTransforms();
            }catch{Dispose();throw;}
        }
        public void Arrived(){arrived=true;}
        public void Dispose(){if(!arrived)foreach(var collider in enabled)collider.SetColliding(false,true);enabled.Clear();}
    }
}
