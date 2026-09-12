#include "mono.h"
#include "config.h"
#include "log.h"
#include <Windows.h>

// ============================================================================
// MONO.CPP - Binding dinamico do Mono embedding (Fase 2 itens 5+6)
// OBJETIVO: leitura real do jogo via reflection: daytime, players (local via
//   get_HasLocalControl) e zumbis (lista + HP chain). Sem CE, sem heap fixo.
// ORIGEM: Mono embedding API + auditoria runtime #1 (OFFSETS.md).
// TESTES: overlay mostra LOCAL HP / ZUMBIS / DAY; log confirma contagens.
// HISTORICO: v0.5.0 bind+daytime. v0.6.0 entidades List<> + invoke.
//   Layout List<T> Mono x64: _items@16, _size@24; vetor: bounds@16, length@24, dados@32.
//   Validado por cruzamento com ZombieLoader.totalRealZombies@184.
// ============================================================================

typedef void* MonoDomain;
typedef void* MonoThread;
typedef void* MonoImage;
typedef void* MonoClass;
typedef void* MonoClassField;
typedef void* MonoVTable;
typedef void* MonoMethod;
typedef void* MonoObject;

typedef MonoDomain* (__cdecl* FnGetRootDomain)();
typedef MonoThread* (__cdecl* FnThreadAttach)(MonoDomain*);
typedef MonoImage*  (__cdecl* FnImageLoaded)(const char*);
typedef MonoClass*  (__cdecl* FnClassFromName)(MonoImage*, const char*, const char*);
typedef MonoClassField* (__cdecl* FnFieldFromName)(MonoClass*, const char*);
typedef MonoVTable* (__cdecl* FnClassVTable)(MonoDomain*, MonoClass*);
typedef void        (__cdecl* FnStaticGetValue)(MonoVTable*, MonoClassField*, void*);
typedef MonoMethod* (__cdecl* FnMethodFromName)(MonoClass*, const char*, int);
typedef MonoObject* (__cdecl* FnRuntimeInvoke)(MonoMethod*, void*, void**, MonoObject**);
typedef void*       (__cdecl* FnObjectUnbox)(MonoObject*);

// Offsets validados (auditoria #1). Nao adivinhar: tudo veio de CE MCP.
namespace Off {
    // PlayersController
    constexpr int PCS_players = 48;
    // PlayerMain (instancia)
    constexpr int PM_healthFast = 204;
    constexpr int PM_staminaFast = 228;
    // ZombieLoader
    constexpr int ZL_zombies = 88;
    constexpr int ZL_totalReal = 184;
    // Zombie (instancia)
    constexpr int Z_health = 168;
    // ZombieObject (instancia)
    constexpr int ZO_eye = 88;
    constexpr int ZO_foot = 96;
    constexpr int Z_obj = 16;
    // MainCamera (instancia)
    constexpr int MC_cam = 32;
    // ZombieHealth (instancia)
    constexpr int ZH_max = 16;
    constexpr int ZH_amount = 32;
    // DaytimeController (instancia)
    constexpr int DT_cur = 172;
    constexpr int DT_len = 72;
    // List<T>
    constexpr int L_items = 16;
    constexpr int L_size = 24;
    // Vetor Mono (T[])
    constexpr int A_len = 24;
    constexpr int A_data = 32;
}

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
    static FnRuntimeInvoke  pInvoke = nullptr;
    static FnObjectUnbox    pUnbox = nullptr;

    static MonoDomain* s_dom = nullptr;
    static MonoImage*  s_img = nullptr;

    static MonoClass* cDay = nullptr;
    static MonoClass* cPlayer = nullptr;
    static MonoClass* cZombie = nullptr;
    static MonoClass* cZLoader = nullptr;
    static MonoClass* cPlayers = nullptr;

    static MonoClassField* fDayInst = nullptr;
    static MonoClassField* fZLInst = nullptr;
    static MonoClassField* fPCInst = nullptr;
    static MonoMethod* mHasLocal = nullptr;
    static MonoImage*  s_unity = nullptr;
    static MonoClass*  cCamU = nullptr;
    static MonoClass*  cTrans = nullptr;
    static MonoMethod* mGetPos = nullptr;
    static MonoMethod* mW2S = nullptr;
    static MonoMethod* mGetTrans = nullptr;
    static EspEntry s_esp[128];
    static int s_espN = 0;

    template <typename T>
    static bool Bind(HMODULE m, const char* name, T& out) {
        out = (T)GetProcAddress(m, name);
        if (!out) Log::Warnf("Mono bind FALHOU: %s", name);
        return out != nullptr;
    }

    static bool ResolveClass(const char* name, MonoClass*& out) {
        out = pClassFrom(s_img, "", name);
        if (out) { s.resolvedClasses++; return true; }
        Log::Warnf("Classe nao resolvida: %s", name);
        return false;
    }

    static bool ResolveField(MonoClass* c, const char* cname, const char* fname, MonoClassField*& out) {
        if (!c) return false;
        out = pFieldFrom(c, fname);
        if (out) { s.resolvedFields++; return true; }
        Log::Warnf("Campo nao resolvido: %s.%s", cname, fname);
        return false;
    }

    static bool ResolveMethod(MonoClass* c, const char* cname, const char* mname, int argc, MonoMethod*& out) {
        if (!c) return false;
        out = pMethodFrom(c, mname, argc);
        if (out) { s.resolvedMethods++; return true; }
        Log::Warnf("Metodo nao resolvido: %s.%s/%d", cname, mname, argc);
        return false;
    }

    static bool StaticInstance(MonoClass* c, MonoClassField* f, void*& out) {
        out = nullptr;
        if (!c || !f) return false;
        __try {
            MonoVTable* vt = pVTable(s_dom, c);
            if (!vt) return false;
            pStaticGet(vt, f, &out);
            return out != nullptr;
        } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }

    static float ReadF(const void* base, int off, float def = 0.0f) {
        float v = def;
        __try { memcpy(&v, (const char*)base + off, sizeof(v)); }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        return v;
    }

    static void* ReadP(const void* base, int off) {
        void* v = nullptr;
        __try { memcpy(&v, (const char*)base + off, sizeof(v)); }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        return v;
    }

    static int ReadI(const void* base, int off, int def = 0) {
        int v = def;
        __try { memcpy(&v, (const char*)base + off, sizeof(v)); }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        return v;
    }

    // Caminha List<T>: valida _size contra o teto e cada ponteiro antes de usar.
    template <typename Fn>
    static int WalkList(void* list, int expectMax, Fn fn) {
        if (!list) return 0;
        int n = 0;
        __try {
            int size = 0;
            memcpy(&size, (char*)list + Off::L_size, sizeof(size));
            if (size <= 0 || size > expectMax) return 0;
            void* arr = nullptr;
            memcpy(&arr, (char*)list + Off::L_items, sizeof(arr));
            if (!arr) return 0;
            // sanity do vetor: length coerente com size
            long long len = 0;
            memcpy(&len, (char*)arr + Off::A_len, sizeof(len));
            if (len < size || len > expectMax) return 0;
            for (int i = 0; i < size; ++i) {
                void* e = nullptr;
                memcpy(&e, (char*)arr + Off::A_data + (size_t)i * 8, 8);
                if (!e) continue;
                // prova de leitura do elemento
                volatile char probe = 0;
                memcpy((void*)&probe, e, 1);
                fn(e, i);
                n++;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) { return n; }
        return n;
    }

    static bool InvokeBool(MonoMethod* m, void* obj) {
        if (!m || !obj) return false;
        __try {
            MonoObject* exc = nullptr;
            MonoObject* ret = pInvoke(m, obj, nullptr, &exc);
            if (exc || !ret) return false;
            unsigned char v = *(unsigned char*)pUnbox(ret);
            return v != 0;
        } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }

    bool Init() {
        if (s.ready) return true;
        if (!s_bound) {
            HMODULE m = GetModuleHandleW(L"mono-2.0-bdwgc.dll");
            if (!m) return false;
            bool ok = true;
            ok &= Bind(m, "mono_get_root_domain", pGetRoot);
            ok &= Bind(m, "mono_thread_attach", pAttach);
            ok &= Bind(m, "mono_image_loaded", pImgLoaded);
            ok &= Bind(m, "mono_class_from_name", pClassFrom);
            ok &= Bind(m, "mono_class_get_field_from_name", pFieldFrom);
            ok &= Bind(m, "mono_class_vtable", pVTable);
            ok &= Bind(m, "mono_field_static_get_value", pStaticGet);
            ok &= Bind(m, "mono_class_get_method_from_name", pMethodFrom);
            ok &= Bind(m, "mono_runtime_invoke", pInvoke);
            ok &= Bind(m, "mono_object_unbox", pUnbox);
            if (!ok) { Log::Error("Mono bind incompleto."); return false; }
            s_dom = pGetRoot();
            if (!s_dom) return false;
            pAttach(s_dom);
            s_bound = true;
            Log::Info("Mono bind OK (10 funcoes), thread anexada.");
        }
        s_img = pImgLoaded("Assembly-CSharp");
        if (!s_img) return false;

        s.resolvedClasses = s.resolvedFields = s.resolvedMethods = 0;
        ResolveClass("DaytimeController", cDay);
        ResolveClass("PlayerMain", cPlayer);
        ResolveClass("Zombie", cZombie);
        ResolveClass("ZombieLoader", cZLoader);
        ResolveClass("PlayersController", cPlayers);
        ResolveField(cDay, "DaytimeController", "instance", fDayInst);
        ResolveField(cZLoader, "ZombieLoader", "Instance", fZLInst);
        ResolveField(cPlayers, "PlayersController", "instance", fPCInst);
        ResolveMethod(cPlayer, "PlayerMain", "get_HasLocalControl", 0, mHasLocal);
        s_unity = pImgLoaded("UnityEngine.CoreModule");
        if (s_unity) {
            s.resolvedClasses++;
            cCamU = pClassFrom(s_unity, "UnityEngine", "Camera");
            cTrans = pClassFrom(s_unity, "UnityEngine", "Transform");
            MonoClass* cComp = pClassFrom(s_unity, "UnityEngine", "Component");
            if (cCamU) s.resolvedClasses++;
            if (cTrans) s.resolvedClasses++;
            if (cCamU) { MonoMethod* t = pMethodFrom(cCamU, "WorldToScreenPoint", 1); if (t) { mW2S = t; s.resolvedMethods++; } else Log::Warn("Metodo nao resolvido: Camera.WorldToScreenPoint/1"); }
            if (cTrans) ResolveMethod(cTrans, "Transform", "get_position", 0, mGetPos);
            if (cComp) ResolveMethod(cComp, "Component", "get_transform", 0, mGetTrans);
        } else Log::Warn("Imagem UnityEngine.CoreModule nao carregada.");

        s.ready = (cDay && cPlayer && cZombie && cZLoader && cPlayers
            && fDayInst && fZLInst && fPCInst && mHasLocal);
        if (s.ready && !s_logged) {
            s_logged = true;
            Log::Infof("Mono resolve OK: %d classes, %d campos, %d metodos.",
                s.resolvedClasses, s.resolvedFields, s.resolvedMethods);
        }
        return s.ready;
    }

    // invoke Transform.get_position -> mundo. Retorna false se falhar.
    static bool GetPos(void* trans, Vec3& out) {
        if (!mGetPos || !trans) return false;
        __try {
            MonoObject* exc = nullptr;
            MonoObject* ret = pInvoke(mGetPos, trans, nullptr, &exc);
            if (exc || !ret) return false;
            memcpy(&out, pUnbox(ret), sizeof(out));
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }

    // invoke Camera.WorldToScreenPoint(mundo) -> pixels Unity (y de baixo p/ cima).
    static bool W2S(void* cam, const Vec3& w, Vec3& out) {
        if (!mW2S || !cam) return false;
        __try {
            void* args[1] = { (void*)&w };
            MonoObject* exc = nullptr;
            MonoObject* ret = pInvoke(mW2S, cam, args, &exc);
            if (exc || !ret) return false;
            memcpy(&out, pUnbox(ret), sizeof(out));
            return out.z > 0.0f;
        } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }

    // Monta snapshot do ESP (zumbis). Roda no Tick (2Hz), nao por frame.
    static void BuildEsp() {
        s_espN = 0;
        if (!Config::bZombieEsp || !mGetPos || !mW2S || !mGetTrans) return;
        // MainCamera.instance (static) -> cam@32 (UnityEngine.Camera).
        // Re-resolve aqui (barato, 2Hz) para pegar a Camera viva.
        MonoClass* cMC = pClassFrom(s_img, "", "MainCamera");
        if (!cMC) return;
        MonoClassField* fInst = pFieldFrom(cMC, "instance");
        if (!fInst) return;
        void* mcObj = nullptr;
        if (!StaticInstance(cMC, fInst, mcObj)) return;
        void* cam = ReadP(mcObj, Off::MC_cam);
        if (!cam) return;
        void* zl = nullptr;
        if (!cZLoader) return;
        MonoClassField* fZL = pFieldFrom(cZLoader, "Instance");
        if (!fZL || !StaticInstance(cZLoader, fZL, zl)) return;
        void* list = ReadP(zl, Off::ZL_zombies);
        WalkList(list, 512, [&](void* e, int) {
            if (s_espN >= 128) return;
            void* h = ReadP(e, Off::Z_health);
            if (!h) return;
            float hp = ReadF(h, Off::ZH_amount);
            float mx = ReadF(h, Off::ZH_max);
            if (hp <= 0 || mx <= 0 || hp > mx) return; // so vivos
            void* zo = ReadP(e, Off::Z_obj);
            if (!zo) return;
            void* eye = ReadP(zo, Off::ZO_eye);
            void* foot = ReadP(zo, Off::ZO_foot);
            if (!eye || !foot) return;
            Vec3 wh, wf, sh, sf;
            if (!GetPos(eye, wh) || !GetPos(foot, wf)) return;
            if (!W2S(cam, wh, sh) || !W2S(cam, wf, sf)) return;
            EspEntry& en = s_esp[s_espN++];
            en.headX = sh.x; en.headY = sh.y;
            en.footX = sf.x; en.footY = sf.y;
            en.hp = hp; en.maxHp = mx;
            en.onScreen = true; en.isAlly = false;
        });
    }

    static void ReadAll() {
        // Daytime (prova de leitura viva simples).
        void* day = nullptr;
        if (StaticInstance(cDay, fDayInst, day)) {
            s.dayTime = ReadF(day, Off::DT_cur);
            s.dayLenMin = ReadF(day, Off::DT_len);
        }
        // Players: local via invoke, resto = aliados.
        void* pcs = nullptr;
        s.players = 0;
        bool gotLocal = false, gotAlly = false;
        if (StaticInstance(cPlayers, fPCInst, pcs)) {
            void* list = ReadP(pcs, Off::PCS_players);
            s.players = WalkList(list, 16, [&](void* e, int) {
                bool local = InvokeBool(mHasLocal, e);
                float hp = ReadF(e, Off::PM_healthFast);
                if (hp < 0 || hp > 100000) return;
                if (local && !gotLocal) {
                    gotLocal = true;
                    s.localHp = hp;
                    s.localStam = ReadF(e, Off::PM_staminaFast);
                } else if (!local && !gotAlly) {
                    gotAlly = true;
                    s.allyHp = hp;
                }
            });
            if (!gotLocal) s.localHp = 0;
        }
        // Zumbis: conta vivos + HP do primeiro vivo; cruza com totalRealZombies.
        void* zl = nullptr;
        s.zombies = 0;
        s.zHp0 = 0;
        if (StaticInstance(cZLoader, fZLInst, zl)) {
            int total = ReadI(zl, Off::ZL_totalReal, -1);
            void* list = ReadP(zl, Off::ZL_zombies);
            int alive = 0;
            WalkList(list, 512, [&](void* e, int) {
                void* h = ReadP(e, Off::Z_health);
                if (!h) return;
                float hp = ReadF(h, Off::ZH_amount);
                float mx = ReadF(h, Off::ZH_max);
                if (hp > 0 && hp <= mx && mx > 0 && mx < 1000000) {
                    if (alive == 0) s.zHp0 = hp;
                    alive++;
                }
            });
            s.zombies = alive;
            if (total >= 0 && (alive > total + 64))
                s.zombies = 0; // layout divergiu: nao reporta lixo
        }
    }

    void Tick() {
        // ESP a ~20Hz (a cada 3 frames): snapshot lento atrasa as boxes quando
        // a camera gira. Leituras de texto do overlay seguem a 2Hz (30 frames).
        ++s_tick;
        if (!s.ready && !Init()) return;
        if (s_tick % 3 == 0) BuildEsp();
        if (s_tick % 30 != 0) return;
        static int n = 0;
        ReadAll();
        if (++n == 1 || n % 20 == 0)
            Log::Infof("Mono live: localHP=%.0f stam=%.0f players=%d zombies=%d zHp0=%.0f day=%.2fh",
                s.localHp, s.localStam, s.players, s.zombies, s.zHp0, s.dayTime);
    }

    const State& Get() { return s; }
    int GetEsp(EspEntry* out, int max) {
        if (!out || max <= 0) return 0;
        int n = s_espN < max ? s_espN : max;
        for (int i = 0; i < n; ++i) out[i] = s_esp[i];
        return n;
    }
    void Shutdown() {
        s = State();
        s_bound = false; s_logged = false;
        s_dom = nullptr; s_img = nullptr;
    }
}






