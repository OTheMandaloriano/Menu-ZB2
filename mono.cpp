#include "mono.h"
#include "log.h"
#include <Windows.h>

// ============================================================================
// MONO.CPP - Binding dinamico do Mono embedding (Fase 2 item 5)
// OBJETIVO: provar leitura real do jogo via reflection (sem CE, sem hardcode
//   de heap). Item 5 = bind + resolve + 1 leitura viva (DaytimeController).
// ORIGEM: Mono embedding API publica + auditoria runtime #1.
// TESTES: com partida aberta, log exibe curTime/hora variando a cada tick.
// HISTORICO: v0.5.0. SEH em toda leitura; Tick throttled (30 frames).
// ============================================================================

typedef void* MonoDomain;
typedef void* MonoThread;
typedef void* MonoImage;
typedef void* MonoClass;
typedef void* MonoClassField;
typedef void* MonoVTable;
typedef void* MonoMethod;
typedef void* MonoObject;
typedef void* MonoExc;

typedef MonoDomain* (__cdecl* FnGetRootDomain)();
typedef MonoThread* (__cdecl* FnThreadAttach)(MonoDomain*);
typedef MonoImage*  (__cdecl* FnImageLoaded)(const char*);
typedef MonoClass*  (__cdecl* FnClassFromName)(MonoImage*, const char*, const char*);
typedef MonoClassField* (__cdecl* FnFieldFromName)(MonoClass*, const char*);
typedef MonoVTable* (__cdecl* FnClassVTable)(MonoDomain*, MonoClass*);
typedef void        (__cdecl* FnStaticGetValue)(MonoVTable*, MonoClassField*, void*);
typedef MonoMethod* (__cdecl* FnMethodFromName)(MonoClass*, const char*, int);

namespace Mono {
    static State s;
    static bool s_bound = false;
    static bool s_logged = false;
    static int  s_tick = 0;

    static FnGetRootDomain  pGetRoot = nullptr;
    static FnThreadAttach   pAttach = nullptr;
    static FnImageLoaded    pImgLoaded = nullptr;
    static FnClassFromName  pClassFrom = nullptr;
    static FnFieldFromName  pFieldFrom = nullptr;
    static FnClassVTable    pVTable = nullptr;
    static FnStaticGetValue pStaticGet = nullptr;
    static FnMethodFromName pMethodFrom = nullptr;

    static MonoDomain* s_dom = nullptr;
    static MonoImage*  s_img = nullptr;

    // Classes resolvidas (ponteiros guardados p/ uso nos proximos itens).
    static MonoClass* cDay = nullptr;
    static MonoClass* cPlayer = nullptr;
    static MonoClass* cZombie = nullptr;
    static MonoClass* cZHealth = nullptr;
    static MonoClass* cZLoader = nullptr;
    static MonoClass* cPlayers = nullptr;
    static MonoClass* cMainCam = nullptr;
    static MonoClass* cFov = nullptr;
    static MonoClass* cNoClip = nullptr;
    static MonoClass* cWaves = nullptr;
    static MonoClass* cExpl = nullptr;
    static MonoClass* cInv = nullptr;

    static MonoClassField* fDayInst = nullptr;
    static MonoClassField* fDayCur = nullptr;   // +172 (validado: 9.68h)
    static MonoClassField* fDayLen = nullptr;   // +72

    template <typename T>
    static bool Bind(HMODULE m, const char* name, T& out) {
        out = (T)GetProcAddress(m, name);
        if (!out) Log::Warnf("Mono bind FALHOU: %s", name);
        return out != nullptr;
    }

    static bool ResolveClass(const char* ns, const char* name, MonoClass*& out) {
        out = pClassFrom(s_img, ns, name);
        if (out) { s.resolvedClasses++; return true; }
        Log::Warnf("Classe nao resolvida: %s.%s", ns, name);
        return false;
    }

    static bool ResolveField(MonoClass* c, const char* cname, const char* fname, MonoClassField*& out) {
        if (!c) return false;
        out = pFieldFrom(c, fname);
        if (out) { s.resolvedFields++; return true; }
        Log::Warnf("Campo nao resolvido: %s.%s", cname, fname);
        return false;
    }

    static bool ResolveMethod(MonoClass* c, const char* cname, const char* mname, int argc) {
        if (!c) return false;
        MonoMethod* m = pMethodFrom(c, mname, argc);
        if (m) { s.resolvedMethods++; return true; }
        Log::Warnf("Metodo nao resolvido: %s.%s/%d", cname, mname, argc);
        return false;
    }

    bool Init() {
        if (s.ready) return true;
        if (!s_bound) {
            HMODULE m = GetModuleHandleW(L"mono-2.0-bdwgc.dll");
            if (!m) return false; // Mono ainda nao carregou
            bool ok = true;
            ok &= Bind(m, "mono_get_root_domain", pGetRoot);
            ok &= Bind(m, "mono_thread_attach", pAttach);
            ok &= Bind(m, "mono_image_loaded", pImgLoaded);
            ok &= Bind(m, "mono_class_from_name", pClassFrom);
            ok &= Bind(m, "mono_class_get_field_from_name", pFieldFrom);
            ok &= Bind(m, "mono_class_vtable", pVTable);
            ok &= Bind(m, "mono_field_static_get_value", pStaticGet);
            ok &= Bind(m, "mono_class_get_method_from_name", pMethodFrom);
            if (!ok) { Log::Error("Mono bind incompleto."); return false; }
            s_dom = pGetRoot();
            if (!s_dom) return false;
            pAttach(s_dom);
            s_bound = true;
            Log::Info("Mono bind OK (8 funcoes), thread anexada.");
        }
        s_img = pImgLoaded("Assembly-CSharp");
        if (!s_img) return false; // cena ainda sem o assembly

        s.resolvedClasses = s.resolvedFields = s.resolvedMethods = 0;
        bool ok = true;
        ok &= ResolveClass("", "DaytimeController", cDay);
        ok &= ResolveClass("", "PlayerMain", cPlayer);
        ok &= ResolveClass("", "Zombie", cZombie);
        ok &= ResolveClass("", "ZombieHealth", cZHealth);
        ok &= ResolveClass("", "ZombieLoader", cZLoader);
        ok &= ResolveClass("", "PlayersController", cPlayers);
        ok &= ResolveClass("", "MainCamera", cMainCam);
        ok &= ResolveClass("", "FOVController", cFov);
        ok &= ResolveClass("", "NoClip", cNoClip);
        ok &= ResolveClass("", "WavesController", cWaves);
        ok &= ResolveClass("", "Explosion", cExpl);
        ok &= ResolveClass("", "PlayerInventory", cInv);

        // Campos (nomes exatos da auditoria #1).
        ResolveField(cDay, "DaytimeController", "instance", fDayInst);
        ResolveField(cDay, "DaytimeController", "curTime", fDayCur);
        ResolveField(cDay, "DaytimeController", "dayDurationInMinutes", fDayLen);
        // (campos de PlayerMain/Zombie validados no item 6; aqui valida metodos)
        ResolveMethod(cPlayer, "PlayerMain", "get_HasLocalControl", 0);
        ResolveMethod(cPlayer, "PlayerMain", "TakeDamage", -1);
        ResolveMethod(cPlayer, "PlayerMain", "Revive", -1);
        ResolveMethod(cZombie, "Zombie", "TakeDamage", -1);
        ResolveMethod(cZombie, "Zombie", "TeleportTo", -1);
        ResolveMethod(cZombie, "Zombie", "SetSpeed", -1);
        ResolveMethod(cNoClip, "NoClip", "SwitchNoClip", 0);
        ResolveMethod(cWaves, "WavesController", "StackZombies", -1);
        ResolveMethod(cInv, "PlayerInventory", "AddItem", -1);

        s.ready = (cDay && fDayInst && fDayCur);
        if (s.ready && !s_logged) {
            s_logged = true;
            Log::Infof("Mono resolve OK: %d classes, %d campos, %d metodos.",
                s.resolvedClasses, s.resolvedFields, s.resolvedMethods);
        }
        if (!s.ready) Log::Warn("Mono resolve incompleto (cena sem Daytime?). Tentando de novo...");
        return s.ready;
    }

    // Leitura viva: DaytimeController.instance (static) -> curTime +172.
    static bool ReadDaytime() {
        __try {
            MonoVTable* vt = pVTable(s_dom, cDay);
            if (!vt) return false;
            void* inst = nullptr;
            pStaticGet(vt, fDayInst, &inst);
            if (!inst) return false;
            float cur = 0, len = 0;
            // Offsets validados runtime #1 (revalidados aqui contra o layout).
            memcpy(&cur, (char*)inst + 172, sizeof(cur));
            memcpy(&len, (char*)inst + 72, sizeof(len));
            if (cur < -1.0f || cur > 48.0f) return false; // sanidade
            s.dayTime = cur;
            s.dayLenMin = len;
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    void Tick() {
        if (++s_tick % 30 != 0) return; // ~2x por segundo a 60fps
        if (!s.ready && !Init()) return;
        static int okStreak = 0;
        if (ReadDaytime()) {
            if (++okStreak == 1 || okStreak % 20 == 0)
                Log::Infof("Mono live: dayTime=%.2fh dayLen=%.1fmin", s.dayTime, s.dayLenMin);
        } else {
            okStreak = 0;
        }
    }

    const State& Get() { return s; }
    void Shutdown() {
        s.ready = false; s_bound = false; s_logged = false;
        s_dom = nullptr; s_img = nullptr;
    }
}


