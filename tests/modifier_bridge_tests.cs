using System;
using System.Collections.Generic;
using Zb2Menu;
namespace Zb2Menu {
 public static class LootMagnetBridge {public static string Status="";public static void Apply(PlayerMain p,bool a,bool b,float c,int d,int e,int f,int g,int h,bool all){}}
    public static class MagnetBridge {public static string Status="";public static bool Active=false;public static void Reset(){}public static void Apply(PlayerMain p,bool e,bool i,float r,int t,float front,float boss,bool freeze,int mode){}}
    public static class SlotsBridge {public static string Status="";public static bool Pending=false;public static bool Restore(){return true;}public static void Apply(PlayerMain p,bool enabled){}}
    public static class NoClipBridge {public static string Status="";public static bool Active=false;public static void Restore(){}public static float LastWalk;public static void Apply(PlayerMain p,bool e,float v,bool input,float walk){LastWalk=walk;}}
}
namespace UnityEngine {
    public static class Application {public static bool isFocused=true;}
    public static class Time {public static float timeScale=1;}
    public class Object { public bool Destroyed; public static bool operator ==(Object a,Object b) { return ReferenceEquals(a,b) || (ReferenceEquals(b,null) && !ReferenceEquals(a,null) && a.Destroyed); } public static bool operator !=(Object a,Object b){return !(a==b);} public override bool Equals(object v){return ReferenceEquals(this,v);} public override int GetHashCode(){return base.GetHashCode();} }
    public struct Vector2 { public float x,y; public Vector2(float a,float b){x=a;y=b;} public static Vector2 zero {get{return new Vector2();}} }
    public struct Vector3 { public float x,y,z; public static Vector3 operator *(Vector3 v,float m){return new Vector3{x=v.x*m,y=v.y*m,z=v.z*m};} }
    public class Transform : Object { public Vector3 localScale {get;set;} }
}
public class PlayerHUD : UnityEngine.Object { public static PlayerHUD instance=new PlayerHUD(); public UnityEngine.Transform innerCrossHairTransform=new UnityEngine.Transform {localScale=new UnityEngine.Vector3{x=1,y=1,z=1}}; }
public static class GlobalTexting {public static bool IsTexting;}
public static class DeveloperConsole {public static bool Opened;}
public class DatabaseGun : UnityEngine.Object { public float rof=8,spread=4,recoilRandomness=.5f; public UnityEngine.Vector2 recoil=new UnityEngine.Vector2(2,3); public bool fullAuto; public int burstCount=3; }
public class InventoryItem { public DatabaseGun Gun=new DatabaseGun(); public object GetDataBaseItem(){return Gun;} }
public class PlayerEquippedItems { public List<InventoryItem> weapons=new List<InventoryItem>(); }
public class PlayerInventory { public PlayerEquippedItems equippedItems=new PlayerEquippedItems(); }
public class PlayerMovement : UnityEngine.Object { public float walkSpeed=3.5f,jumpSpeed=6,fallDamageThreshold=-10,rollSpeed=9; }
public class PlayerMain : UnityEngine.Object { public PlayerMovement movement=new PlayerMovement(); public PlayerInventory inventory=new PlayerInventory(); public bool HasLocalControl=true; public float healthFast=100; }
public class WeaponBase : UnityEngine.Object { public static WeaponBase instance=new WeaponBase(); public float gunSway=.7f; }
public class PlayerMeleeAttack : UnityEngine.Object { public float Duration {get;private set;} public PlayerMeleeAttack(){Duration=2;} }
public class MeleeAttackBase : UnityEngine.Object { public static MeleeAttackBase Instance=new MeleeAttackBase(); public Dictionary<int,PlayerMeleeAttack> AllAttacks=new Dictionary<int,PlayerMeleeAttack>(); }
class Tests {
    static int checks;
    static void Check(bool ok,string name){if(!ok)throw new Exception(name+" "+ModifierBridge.LastError());++checks;}
    static void Apply(PlayerMain p,int flags,float rapid=2,float speed=2,float jump=2,float roll=2,float knife=2){ModifierBridge.Apply(p,flags,rapid,speed,jump,roll,knife,1,50,0,2.5f,8,50,3,-1,-1,-1,-1,0,0);Check(ModifierBridge.LastError()=="","no adapter failure");}
    static void Main(){
        var p=new PlayerMain(); var item=new InventoryItem(); p.inventory.equippedItems.weapons.Add(item); var g=item.Gun;
        Apply(p,16,5); Check(g.rof==40,"rapid 5x");
        for(int i=0;i<100;++i)Apply(p,16,5);
        Check(g.rof==40,"no compounding"); Apply(p,16,2); Check(g.rof==16,"lower slider");
        Apply(p,16,1);Check(g.rof==8,"1x original"); Apply(p,16,3); Apply(p,0); Check(g.rof==8,"off restores");
        Apply(p,512|16,3);Check(g.fullAuto && g.burstCount==0,"auto enabled");
        Apply(p,16,3);Check(!g.fullAuto && g.burstCount==3 && g.rof==24,"independent auto off");
        Apply(p,512);Check(g.rof==8 && g.fullAuto,"rapid off preserves auto");
        Apply(p,0);Check(!g.fullAuto && g.burstCount==3,"bool integer restore");
        Apply(p,1|2|8);Check(PlayerHUD.instance.innerCrossHairTransform.localScale.x==.5f,"crosshair reduced");
        Apply(p,1|2);Check(PlayerHUD.instance.innerCrossHairTransform.localScale.x==1,"crosshair restored");Check(g.spread==0 && g.recoil.x==0,"overlapping owners");
        Apply(p,0); Check(g.spread==4 && g.recoil.x==2 && g.recoil.y==3,"vector restore");
        Apply(p,32|64|128); Check(p.movement.walkSpeed==7 && p.movement.jumpSpeed==12 && p.movement.fallDamageThreshold==-20 && p.movement.rollSpeed==18,"movement enabled");
        Apply(p,32|64|128,2,3,3,3); Check(NoClipBridge.LastWalk==3.5f,"noclip base ignores active speed multiplier"); Check(p.movement.walkSpeed==10.5f && p.movement.jumpSpeed==18,"movement sliders");
        var old=p.movement; p.movement=new PlayerMovement(); Apply(p,32|64|128); Check(old.walkSpeed==3.5f && old.jumpSpeed==6,"replace object restores old");
        Apply(p,0);Check(p.movement.jumpSpeed==6 && p.movement.fallDamageThreshold==-10,"jump disabled");
        Apply(p,4);Check(WeaponBase.instance.gunSway==0,"sway enabled");Apply(p,0);Check(WeaponBase.instance.gunSway==.7f,"sway restored");
        MeleeAttackBase.Instance.AllAttacks[1]=new PlayerMeleeAttack();var attack=MeleeAttackBase.Instance.AllAttacks[1];
        Apply(p,256,2,2,2,2,5); Check(Math.Abs(attack.Duration-.4f)<.001f,"knife multiplier");
        Apply(p,256,2,2,2,2,2);Check(attack.Duration==1,"knife slider uses original");Apply(p,0);Check(attack.Duration==2,"knife restored");
        for(int i=0;i<70;++i)p.inventory.equippedItems.weapons.Add(new InventoryItem());
        Apply(p,16,4);Apply(p,0);foreach(var weapon in p.inventory.equippedItems.weapons)Check(weapon.Gun.rof==8,"no 16-slot truncation");
        Apply(p,16,3);p.inventory.equippedItems.weapons.Clear();Apply(p,16,3);Check(g.rof==8,"removed equipment restored");
        p.inventory.equippedItems.weapons.Add(item);Apply(p,16,3);ModifierBridge.Reset();Check(g.rof==8,"scene reset restores shared asset");
        Apply(p,16,2);Apply(new PlayerMain(),0);Check(g.rof==8,"player switch restores");
        Apply(p,16,float.NaN);Check(g.rof==8,"invalid slider safe");Apply(p,0);
        var registry=new OriginalValues();var target=new object();int value=7;bool fail=false;
        registry.Begin();registry.Apply(target,"value",()=>value,v=>{if(fail)throw new Exception();value=v;},v=>v*2);
        fail=true;try{registry.RestoreAll();}catch(InvalidOperationException){}Check(registry.Count==1,"failed restore retained");
        fail=false;registry.RestoreAll();Check(value==7 && registry.Count==0,"failed restore retried");
        Console.WriteLine(checks+" modifier checks passed");
    }
}

namespace Zb2Menu {public static class GodModeBridge {public static bool Active=false;public static void Install(HarmonyLib.Harmony h){}public static void Clear(){}public static void Apply(PlayerMain p,bool b){}}}
