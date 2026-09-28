using System;
using System.Runtime.CompilerServices;
using HarmonyLib;
using Zb2Menu;
namespace UnityEngine {
 public enum CursorLockMode {None,Locked,Confined}
 public static class Cursor {public static CursorLockMode lockState;public static bool visible;}
 public static class Application {public static bool isFocused=true;}
}
namespace UnityEngine.EventSystems {
 public class StandaloneInputModule {public int processed;[MethodImpl(MethodImplOptions.NoInlining)]public void Process(){processed++;}}
}
public class MenuController {public bool gameHud=true;[MethodImpl(MethodImplOptions.NoInlining)]public bool CursorShouldBeLocked(){return gameHud;}}
public class PlayerInputReader {public int updates,cleared;[MethodImpl(MethodImplOptions.NoInlining)]public void MyUpdate(bool blockMouse){updates++;}public void ClearAllInput(){cleared++;}}
class MenuTests {
 static int checks;static void Check(bool b,string n){if(!b)throw new Exception(n);checks++;}
 static void Main(){
  var h=new Harmony("menu.input.tests");MenuInputBridge.Install(h);
  var menu=new MenuController();var input=new PlayerInputReader();var ui=new UnityEngine.EventSystems.StandaloneInputModule();
  UnityEngine.Cursor.lockState=UnityEngine.CursorLockMode.Locked;UnityEngine.Cursor.visible=false;
  MenuInputBridge.Update(1|2);Check(!menu.CursorShouldBeLocked() && UnityEngine.Cursor.lockState==UnityEngine.CursorLockMode.None,"overlay unlocks cursor without opening game menu");
  input.MyUpdate(false);Check(input.updates==0 && input.cleared==1,"gameplay input suppressed while menu open");
  ui.Process();Check(ui.processed==0,"captured click cannot reach underlying game UI");
  MenuInputBridge.Update(1);ui.Process();Check(ui.processed==1,"outside overlay game UI receives input");
  MenuInputBridge.Update(0);input.MyUpdate(false);ui.Process();Check(input.updates==1 && ui.processed==2,"closing restores commands");
  Check(UnityEngine.Cursor.lockState==UnityEngine.CursorLockMode.Locked && !UnityEngine.Cursor.visible,"restores prior game cursor");
  UnityEngine.Cursor.lockState=UnityEngine.CursorLockMode.None;UnityEngine.Cursor.visible=true;menu.gameHud=false;
  MenuInputBridge.Update(7);MenuInputBridge.Update(0);Check(UnityEngine.Cursor.visible && UnityEngine.Cursor.lockState==UnityEngine.CursorLockMode.None,"game menu cursor remains free on close");
  MenuInputBridge.Update(7);UnityEngine.Application.isFocused=false;MenuInputBridge.Update(7);ui.Process();Check(ui.processed==3 && UnityEngine.Cursor.visible,"focus loss releases ownership");
  h.UnpatchAll("menu.input.tests");Console.WriteLine(checks+" menu cursor and input checks passed");
 }
}
