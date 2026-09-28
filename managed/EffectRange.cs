using UnityEngine;
namespace Zb2Menu {
    public static class EffectRange {
        public static float Distance=200;
        public static bool InRange(Zombie zombie) {
            var camera=MainCamera.instance==null?null:MainCamera.instance.cam;
            if(camera==null || zombie==null || zombie.obj==null)return false;
            float limit=Distance;
            if(float.IsNaN(limit)||float.IsInfinity(limit))return false;
            limit=Mathf.Clamp(limit,10,500);
            var player=PlayersController.instance==null?null:PlayersController.instance.MyPlayer();
            var origin=player==null?camera.transform.position:player.transform.position;
            return Vector3.Distance(origin,zombie.obj.transform.position)<=limit;
        }
    }
}
