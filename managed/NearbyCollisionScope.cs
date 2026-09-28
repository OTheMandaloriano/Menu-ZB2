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
                    if(!Near(target,destination))continue;
                    if(enabled.Count>=256)throw new InvalidOperationException("Area com colisoes demais para verificar");
                    enabled.Add(target.lodCollider);target.lodCollider.SetColliding(true,true);
                }
                Physics.SyncTransforms();
            }catch{Dispose();throw;}
        }
        static bool Near(LODTarget target,Vector3 destination) {
            if(Vector3.Distance(target.transform.position,destination)<=16)return true;
            if(NearCollider(target.lodCollider.col,destination))return true;
            var obj=target.lodCollider.obj;
            if(obj!=null)foreach(var collider in obj.GetComponentsInChildren<Collider>(true))
                if(NearCollider(collider,destination))return true;
            return false;
        }
        static bool NearCollider(Collider collider,Vector3 destination) {
            if(collider==null)return false;
            Bounds local;
            var mesh=collider as MeshCollider;var box=collider as BoxCollider;
            if(mesh!=null && mesh.sharedMesh!=null)local=mesh.sharedMesh.bounds;
            else if(box!=null)local=new Bounds(box.center,box.size);
            else {var bounds=collider.bounds;return bounds.extents.sqrMagnitude>.0001f ? bounds.SqrDistance(destination)<=256 : Vector3.Distance(collider.transform.position,destination)<=16;}
            // Collider.bounds can be empty for a disabled LOD. Local geometry remains valid.
            Bounds world=new Bounds(collider.transform.TransformPoint(local.center),Vector3.zero);
            for(int i=0;i<8;++i)world.Encapsulate(collider.transform.TransformPoint(local.center+new Vector3(
                (i&1)==0?-local.extents.x:local.extents.x,(i&2)==0?-local.extents.y:local.extents.y,(i&4)==0?-local.extents.z:local.extents.z)));
            return world.SqrDistance(destination)<=256;
        }
        public int EnabledCount {get{return enabled.Count;}}
        public void Arrived(){arrived=true;}
        public void Dispose(){if(!arrived)foreach(var collider in enabled)collider.SetColliding(false,true);enabled.Clear();}
    }
}
