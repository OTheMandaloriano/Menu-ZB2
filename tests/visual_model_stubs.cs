using System.Collections.Generic;
using UnityEngine;
namespace UnityEngine {
    public enum HumanBodyBones {Head,Neck,Chest,Spine,Hips,LeftUpperArm,LeftLowerArm,LeftHand,RightUpperArm,RightLowerArm,RightHand,LeftUpperLeg,LeftLowerLeg,LeftFoot,RightUpperLeg,RightLowerLeg,RightFoot}
    public class Animator {public bool isHuman=true;public Transform GetBoneTransform(HumanBodyBones joint){return new Transform{position=new Vector3(0,2,10)};}}
}
public class CharacterSkin {public Animator animator=new Animator();}
public class ZombieHealth {public bool isAlive=true;}
#if !RANGE_TESTS
public class ZombieObject {public Transform transform=new Transform();}
public class Zombie {public bool isWaveZombie;public ZombieObject obj=new ZombieObject();public ZombieHealth health=new ZombieHealth();}
public class ZombieLoader {public static ZombieLoader Instance=new ZombieLoader();public List<Zombie> zombies=new List<Zombie>();}
#endif
