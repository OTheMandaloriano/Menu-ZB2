using System;
using HarmonyLib;
using UnityEngine;
namespace Zb2Menu {
    public static class MenuInputBridge {
        static bool open,capture;
        static CursorLockMode previousLock;
        static bool previousVisible;
        public static void Install(Harmony harmony) {
            var cursor=AccessTools.Method(typeof(MenuController),"CursorShouldBeLocked");
            var player=AccessTools.Method(typeof(PlayerInputReader),"MyUpdate");
            if(cursor==null || player==null)throw new MissingMethodException("Menu/input lifecycle changed");
            harmony.Patch(cursor,postfix:new HarmonyMethod(typeof(MenuInputBridge),"CursorDecision"));
            harmony.Patch(player,prefix:new HarmonyMethod(typeof(MenuInputBridge),"PlayerInput"));
            foreach(string name in new[]{"UnityEngine.EventSystems.StandaloneInputModule","UnityEngine.InputSystem.UI.InputSystemUIInputModule"}) {
                var type=AccessTools.TypeByName(name);var process=type==null?null:AccessTools.Method(type,"Process",Type.EmptyTypes);
                if(process!=null)harmony.Patch(process,prefix:new HarmonyMethod(typeof(MenuInputBridge),"GameMenuInput"));
            }
        }
        public static void Update(int flags) {
            bool next=(flags&1)!=0 && Application.isFocused;
            capture=next && (flags&6)!=0;
            if(next && !open){previousLock=Cursor.lockState;previousVisible=Cursor.visible;}
            if(!next && open){Cursor.lockState=previousLock;Cursor.visible=previousVisible;}
            open=next;
            if(open){Cursor.lockState=CursorLockMode.None;Cursor.visible=false;} // ImGui draws the pointer.
        }
        static void CursorDecision(ref bool __result){if(open)__result=false;}
        static bool PlayerInput(PlayerInputReader __instance){if(!open)return true;__instance.ClearAllInput();return false;}
        static bool GameMenuInput(){return !capture;}
    }
}
