using System;
using System.Runtime.CompilerServices;
using HarmonyLib;
using Zb2Menu;
namespace UnityEngine {public static class Time {public static float unscaledTime;}}
public class Damage {public float amount;}
public class PlayerMain {
 public enum HealthState {Alive,Dying,Dead}
 public bool HasLocalControl=true;public float healthFast=100,healthSlow=100,MaxHealth=100;
 public HealthState healthState;
 [MethodImpl(MethodImplOptions.NoInlining)] public void TakeDamage(Damage damage){healthFast-=damage.amount;if(healthFast<0)healthState=HealthState.Dying;}
}
class GodTests {
 static int checks;static void Check(bool ok,string name){if(!ok)throw new Exception(name);++checks;}
 static void Main(){
  var harmony=new Harmony("zb2.god.tests");GodModeBridge.Install(harmony);
  var player=new PlayerMain{MaxHealth=133.3f};GodModeBridge.Apply(player,true);
  Check(player.healthFast==133.3f && player.healthSlow==133.3f,"perk maximum used for both pools");
  player.TakeDamage(new Damage{amount=10000});Check(player.healthState==PlayerMain.HealthState.Alive && player.healthFast==133.3f,"fatal fall blocked before death transition");
  player.TakeDamage(new Damage{amount=30});Check(player.healthFast==133.3f,"ordinary damage blocked");
  player.MaxHealth=175;GodModeBridge.Apply(player,true);Check(player.healthFast==175,"changed perk maximum followed");
  var remote=new PlayerMain{HasLocalControl=false};remote.TakeDamage(new Damage{amount=20});Check(remote.healthFast==80,"remote actor untouched");
  GodModeBridge.Apply(player,false);player.TakeDamage(new Damage{amount=25});Check(player.healthFast==150,"disable restores damage without reverting health snapshot");
  GodModeBridge.Apply(player,true);UnityEngine.Time.unscaledTime=1;player.TakeDamage(new Damage{amount=10});Check(player.healthFast==165,"stale runtime does not leave invulnerability latched");
  player.healthState=PlayerMain.HealthState.Dead;player.healthFast=0;GodModeBridge.Apply(player,true);Check(player.healthFast==0,"does not resurrect an already dead player");
  player.healthState=PlayerMain.HealthState.Alive;player.healthFast=100;player.MaxHealth=float.NaN;GodModeBridge.Apply(player,true);player.TakeDamage(new Damage{amount=10});Check(player.healthFast==90,"invalid maximum fails open");
  player.MaxHealth=100;GodModeBridge.Apply(player,true);var other=new PlayerMain();GodModeBridge.Apply(other,true);player.TakeDamage(new Damage{amount=10});Check(player.healthFast==90,"old local instance released after respawn");
  GodModeBridge.Clear();harmony.UnpatchAll("zb2.god.tests");Console.WriteLine(checks+" God Mode Harmony checks passed");
 }
}
