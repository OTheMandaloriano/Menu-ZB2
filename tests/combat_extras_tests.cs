using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using HarmonyLib;
using Zb2Menu;
namespace UnityEngine {
 public struct Vector3 {public float x,y,z;public Vector3(float a,float b,float c){x=a;y=b;z=c;}public static Vector3 up=new Vector3(0,1,0);public static Vector3 operator*(Vector3 v,float n){return new Vector3(v.x*n,v.y*n,v.z*n);}public static Vector3 operator+(Vector3 a,Vector3 b){return new Vector3(a.x+b.x,a.y+b.y,a.z+b.z);}}
 public class Transform {public Vector3 forward=new Vector3(0,0,1);}
 public class Rigidbody {public bool isKinematic;public Vector3 linearVelocity;}
 public static class Time {public static float unscaledTime;}
 public struct AnimatorStateInfo {public string name;public bool IsName(string n){return name==n;}}
 public class Animator {public string name="Punch";public AnimatorStateInfo GetCurrentAnimatorStateInfo(int n){return new AnimatorStateInfo{name=name};}}
}
public class MultiplayerController {public static MultiplayerController instance=new MultiplayerController();public bool server=true;public bool IsServer(){return server;}}
public class DatabaseItem {public int stackMax=20;}
public class DatabaseThrowable:DatabaseItem {public Explosion.ID explosionID;public float timeOut=4;public bool contactDestroy;}
public class Explosion {public enum ID {None,Frag,Dynamite,BossFlash}public float rangeMeters=5,dmgMin=10,dmgMax=100,maxTotalDamage=1000;}
public class ExplosionController {public static ExplosionController instance=new ExplosionController();public Explosion frag=new Explosion(),dynamite=new Explosion();public Explosion GetExplosion(Explosion.ID id){return id==Explosion.ID.Frag?frag:dynamite;}}
public class ItemsBase {public static ItemsBase instance=new ItemsBase();public static int ItemCount=4;public DatabaseItem[] items={null,new DatabaseThrowable{explosionID=Explosion.ID.Frag},new DatabaseItem{stackMax=1},new DatabaseThrowable{explosionID=Explosion.ID.BossFlash}};public DatabaseItem GetItem(InventoryItem.ID id){return items[(int)id];}}
public class InventoryItem {public enum ID {None,Frag,Gun,Boss}public ID id;public int stackCount;public InventoryItem(ID id){this.id=id;}public void SetGenericNumericValue(int n){stackCount=n;}}
public enum LootPlacingFilter {Both}
public class PlayerInventory {public bool full;public List<InventoryItem> items=new List<InventoryItem>();public object FindPlaceFor(InventoryItem.ID id,int n,LootPlacingFilter filter){return full?null:new object();}public void PutLootIntoPosition(InventoryItem item,object place){items.Add(item);}}
public enum PlayerMeleeAttackID {LeftJab,RightJab,RightOverhandPunch,SpinningBackfistLeft,KnifeStab}
public class PlayerMeleeAttack {public PlayerMeleeAttackID ID;public class Settings {public string Name="Punch";}public Settings Animation=new Settings();}
public class PlayerMovement {public PlayerMeleeAttack CurrentMeleeAttack=new PlayerMeleeAttack();}
public class PlayerMain {public bool HasLocalControl=true;public float healthFast=100;public UnityEngine.Transform transform=new UnityEngine.Transform();public UnityEngine.Animator MyAnimator=new UnityEngine.Animator();public PlayerMovement movement=new PlayerMovement();public PlayerInventory inventory=new PlayerInventory();}
public class PlayersController {public static PlayersController instance=new PlayersController();public PlayerMain player=new PlayerMain();public PlayerMain MyPlayer(){return player;}}
public interface IDamageTarget {void TakeDamage(Damage d);}
public enum DamageType {BluntMelee,Bullet}
public struct ZombieDamage {public PlayerMain sourcePlayer;}
public class Damage {public float amount;public DamageType damageType;public ZombieDamage zombieDamage;public Damage Clone(){return (Damage)MemberwiseClone();}[MethodImpl(MethodImplOptions.NoInlining)]public static void ProcessDamage(IDamageTarget target,Damage damage,bool localCall){target.TakeDamage(damage);}}
public class ZombieHealth {public bool isAlive=true;}
public class ZombieObject {public UnityEngine.Rigidbody body=new UnityEngine.Rigidbody();}
public class Zombie:IDamageTarget {public ZombieObject obj=new ZombieObject();public ZombieHealth health=new ZombieHealth();public float received;public void TakeDamage(Damage d){received=d.amount;}}
public class ZombieDoll {public UnityEngine.Rigidbody[] armaturePhysics={new UnityEngine.Rigidbody()};[MethodImpl(MethodImplOptions.NoInlining)]public void CopyArmature(ZombieObject original,UnityEngine.Vector3 deathForce){}}
namespace Zb2Menu {public static class ItemEligibility {public static bool blocked;public static bool Allowed(DatabaseItem db){return db!=null && !blocked;}}public static class MagnetFreeze {public static int released;public static void Release(Zombie z){++released;}}}
class ExtrasTests {
 static int checks;static void Check(bool b,string n){if(!b)throw new Exception(n);++checks;}
 static string Apply(int punch=0,int exp=0,int contact=0,float fuse=2,float radius=8,float damage=500,int id=0,int amount=1,int request=0,int ready=1){return CombatExtrasBridge.Apply(ready,punch,exp,contact,fuse,radius,damage,id,amount,request);}
 static void Main(){
  var h=new Harmony("zb2.extras.test");CombatExtrasBridge.Install(h);var player=PlayersController.instance.player;
  var db=(DatabaseThrowable)ItemsBase.instance.items[1];var exp=ExplosionController.instance.frag;
  Apply(exp:1,contact:1);Check(db.timeOut==2 && db.contactDestroy && exp.rangeMeters==8 && exp.dmgMax==500,"explosive settings applied");
  Apply(exp:1,contact:0,fuse:3,damage:200);Check(db.timeOut==3 && !db.contactDestroy && exp.dmgMin==20 && exp.maxTotalDamage==2000,"slider recalculates from original, contact restores independently");
  Check(((DatabaseThrowable)ItemsBase.instance.items[3]).timeOut==4,"boss throwable unchanged");
  Apply();Check(db.timeOut==4 && exp.dmgMin==10 && exp.dmgMax==100 && exp.rangeMeters==5,"disable restores exact baseline");
  Apply(exp:1);MultiplayerController.instance.server=false;Apply(exp:1);Check(db.timeOut==4 && exp.dmgMax==100,"authority loss restores modifications");MultiplayerController.instance.server=true;
  Apply(id:1,amount:999,request:1);Check(player.inventory.items.Count==1 && player.inventory.items[0].stackCount==20,"grant clamps to real stack limit");
  Apply(id:1,request:1);Check(player.inventory.items.Count==1,"grant consumed once");
  player.inventory.full=true;Check(Apply(id:1,request:2).Contains("sem espaco") && player.inventory.items.Count==1,"full inventory creates nothing");player.inventory.full=false;
  ItemEligibility.blocked=true;Apply(id:1,request:3);Check(player.inventory.items.Count==1,"blocked item rejected");ItemEligibility.blocked=false;
  Apply(id:2,amount:999,request:4);Check(player.inventory.items[1].stackCount==1,"weapon count is one");
  Apply(id:1,request:5,ready:0);Apply(id:1,request:5);Check(player.inventory.items.Count==2,"loading request not replayed");
  Apply(punch:1,request:5);var zombie=new Zombie();var damage=new Damage{amount=10,damageType=DamageType.BluntMelee,zombieDamage=new ZombieDamage{sourcePlayer=player}};
  Damage.ProcessDamage(zombie,damage,true);Check(zombie.received==4000000 && damage.amount==10 && MagnetFreeze.released==1,"punch clones damage and releases captured target");
  Check(zombie.obj.body.linearVelocity.z==25,"punch launches live body");var doll=new ZombieDoll();doll.CopyArmature(zombie.obj,new UnityEngine.Vector3());Check(doll.armaturePhysics[0].linearVelocity.z==25,"death ragdoll receives same impulse");
  player.movement.CurrentMeleeAttack.ID=PlayerMeleeAttackID.KnifeStab;Damage.ProcessDamage(zombie,damage,true);Check(zombie.received==10,"knife unaffected");player.movement.CurrentMeleeAttack.ID=PlayerMeleeAttackID.LeftJab;
  damage.damageType=DamageType.Bullet;Damage.ProcessDamage(zombie,damage,true);Check(zombie.received==10,"bullet during punch unaffected");damage.damageType=DamageType.BluntMelee;
  player.MyAnimator.name="Idle";Damage.ProcessDamage(zombie,damage,true);Check(zombie.received==10,"stale attack record not enough");player.MyAnimator.name="Punch";
  Apply(request:5);Damage.ProcessDamage(zombie,damage,true);Check(zombie.received==10,"disable restores normal punch without undoing kills");
  h.UnpatchAll("zb2.extras.test");Console.WriteLine(checks+" combat extras checks passed");
 }
}
