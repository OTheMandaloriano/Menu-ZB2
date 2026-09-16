#include "mono.h"
#include "config.h"
#include "log.h"
#include <Windows.h>
#include <math.h>
#include <string.h>

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
typedef void* (__cdecl* FnMethodDescNew)(const char*, int);
typedef void* (__cdecl* FnMethodDescSearch)(void*, void*); // (desc, klass)
typedef void (__cdecl* FnMethodDescFree)(void*);
typedef void* (__cdecl* FnClassGetMethods)(void*, void**);
typedef void* (__cdecl* FnMethodSig)(void*);
typedef const char* (__cdecl* FnMethodName)(void*);
typedef int (__cdecl* FnSigParamCount)(void*);
typedef void* (__cdecl* FnSigGetParam)(void*, void**); // (sig, iter) — GeoArray-like, iter avanza
typedef int (__cdecl* FnTypeGetType)(void*);
typedef const char* (__cdecl* FnTypeGetName)(void*); // mono_type_get_name (auditoria [SIG])
typedef void* (__cdecl* FnClassGetFields)(void*, void**); // mono_class_get_fields (auditoria [FIELDS])
typedef const char* (__cdecl* FnFieldGetName)(void*); // mono_field_get_name
typedef int (__cdecl* FnFieldGetOff)(void*); // mono_field_get_offset (pode nao existir)
typedef MonoObject* (__cdecl* FnRuntimeInvoke)(MonoMethod*, void*, void**, MonoObject**);
typedef void*       (__cdecl* FnObjectUnbox)(MonoObject*);
typedef char*       (__cdecl* FnStringUtf8)(MonoObject*);
typedef void        (__cdecl* FnFree)(void*);

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
    constexpr int ZO_armature = 72; // Transform[] (item 12 Skeleton real)
    constexpr int ZO_mesh = 56; // SkinnedMeshRenderer do corpo (bounds p/ Box 3D)
    constexpr int Z_obj = 16;
    // MainCamera (instancia)
    constexpr int MC_cam = 32;
    // ZombieHealth (instancia)
    constexpr int ZH_max = 16;
    constexpr int ZH_amount = 32;
    constexpr int ZH_alive = 28; // bool isAlive (anti-fantasma: morte dura ~1s com HP>0)
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
    static FnMethodDescNew pDescNew = nullptr;
    static FnMethodDescSearch pDescSearch = nullptr;
    static FnMethodDescFree pDescFree = nullptr;
    static FnClassGetMethods pClassMethods = nullptr;
    static FnMethodSig pSigOf = nullptr;
    static FnSigParamCount pSigCount = nullptr;
    static FnSigGetParam pSigParam = nullptr;
    static FnTypeGetType pTypeKind = nullptr;
    static FnTypeGetName pTypeName = nullptr;
    static FnMethodName pMethodGetName = nullptr;
    static FnClassGetFields pClassFields = nullptr;
    static FnFieldGetName pFieldGetName = nullptr;
    static FnFieldGetOff pFieldGetOff = nullptr;
    static FnRuntimeInvoke  pInvoke = nullptr;
    static FnObjectUnbox    pUnbox = nullptr;
    static FnStringUtf8     pStrUtf8 = nullptr;
    static FnFree           pFree = nullptr;
    static MonoMethod* mGetName = nullptr;
    static MonoMethod* mGetBounds = nullptr; // Renderer.get_bounds (Box 3D real)
    static MonoMethod* mGetViewMat = nullptr; // Camera.get_worldToCameraMatrix (VP proprio)
    static MonoMethod* mGetProjMat = nullptr; // Camera.get_projectionMatrix (VP proprio)
    static float s_vp[16];       // VP = P*V (column-major, padrao Unity)
    static bool  s_vpOk = false;
    static float s_vpW = 1920.0f, s_vpH = 1080.0f;
    static int s_ghostDead = 0;   // HP>0 mas isAlive=false (animacao de morte)
    static int s_ghostBad = 0;    // centro/pos nao-finito, absurdo ou origem
    static void* s_lastEnts[128]; // snapshot anterior (log de transicoes P1)
    static int s_lastN = 0;

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
    // Telemetria por site de pInvoke (Fase 1, SEM mudar logica): conta chamadas
    // e falhas por ciclo na worker; o agregado sai 1x/5s no [PI-CALL], e so
    // loga se fail>0 OU media>500us. Leitura atômica nao precisa (worker unica
    // escreve, Present so le via GetEsp com TryEnter).
    // Fase 1b: fail discriminado por sub-causa (exc = exc||!ret, hd = hd
    // NaN/<=0, seh = __except). Budget negado NAO conta (nao e falha do jogo).
    struct PiSite { long n; long fail; long exc; long hd; long seh; long long dt_us; };
    static PiSite s_piPos, s_piRot, s_piBnd, s_piTrC, s_piRay, s_piHas;
    static long long s_piWinStart;
    static inline void PiAdd(PiSite& s, long long dt, bool ok) {
        s.n++; s.dt_us += dt; if (!ok) s.fail++;
    }
    // Discriminador: 0=exc, 1=hd, 2=seh. Mesma assinatura, zero custo extra.
    static inline void PiAddEx(PiSite& s, long long dt, int kind) {
        s.n++; s.dt_us += dt; s.fail++;
        if (kind == 0) s.exc++;
        else if (kind == 1) s.hd++;
        else s.seh++;
    }
    static inline long long PiNow() {
        LARGE_INTEGER t, f; QueryPerformanceCounter(&t); QueryPerformanceFrequency(&f);
        return t.QuadPart * 1000000LL / f.QuadPart;
    }
    static void PiFlush(bool force) {
        long long now = PiNow();
        if (!force && now - s_piWinStart < 5000000LL) return;
        s_piWinStart = now;
        const struct { const char* nm; PiSite* s; } sites[6] = {
            { "getPos", &s_piPos }, { "getRot", &s_piRot }, { "getBounds", &s_piBnd },
            { "getTransCam", &s_piTrC }, { "raycast", &s_piRay }, { "hasLocal", &s_piHas },
        };
        for (int i = 0; i < 6; ++i) {
            PiSite* s = sites[i].s;
            if (!s->n) continue;
            long long avg = s->dt_us / s->n;
            if (force || s->fail > 0 || avg > 500)
                Log::Infof("[PI-CALL] site=%s n=%ld fail=%ld exc=%ld hd=%ld seh=%ld dt_avg_us=%lld",
                    sites[i].nm, s->n, s->fail, s->exc, s->hd, s->seh, avg);
            s->n = 0; s->fail = 0; s->exc = 0; s->hd = 0; s->seh = 0; s->dt_us = 0;
        }
    }
    // Orçamento de invokes por ciclo de BuildEsp (anti-crash em horda).
    // Cada invoke cruza para o Mono e compete com o PhysX/LOD do jogo; com
    // 50+ zumbis, centenas de invokes por ciclo de 33ms viram corrida com o
    // LODController:UpdatePhysics (ver crash 15/09 15:51). Estoura o teto?
    // O resto do ciclo usa o último valor conhecido (fail-open, sem flicker).
    // Camada 1 (broadphase, padrao UC): perto (<45m) = LOS real todo ciclo;
    // longe = 1 de 3 ciclos (rodizio). Teto 96 ent/ciclo. Ritmo 33-66ms.
    static int  s_budgetLeft = 0;
    static int  s_budgetMax = 200; // max_raycasts_padrao ~= 200 (briefing §6)
    static bool s_budgetLogged = false;
    static int  s_losSkipped = 0; // telemetria: quantos pontos o teto pulou
    static int  s_losCursor = 0; // legado: rodizio removido (piscava em porta/janela).
    // Manter o campo evita diff gigante; o ciclo so o incrementa.
    static inline bool BudgetTake(int n = 1) {
        if (s_budgetLeft < n) return false;
        s_budgetLeft -= n;
        return true;
    }
    static MonoMethod* mGetGO = nullptr; // Component.get_gameObject (auditoria ossos)
    static MonoMethod* mGetRot = nullptr; // Transform.get_rotation -> Matrix4x4 (fix bug 4)
    static MonoMethod* mLinecast = nullptr; // Physics.Linecast alternativa (sem ambiguidade Ray)
    static int s_lineArgs = 0;
    static bool s_boneLogged = false;
    static bool s_jointLogged = false; // auditoria juntas (1x por sessao)
    static bool s_skelLogged = false; // diagnostico SKEL (1x: mascara + tela dos bracos)
    static bool s_handLogged = false; // diagnostico HAND (1x: ponta da mao em mundo)
    static bool s_handLogged2 = false; // diagnostico HAND2 (1x: maos vivas pos-fix)
    static DWORD WINAPI EspThread(LPVOID); // forward (definida apos BuildEsp)
    static MonoImage*  s_unity = nullptr;
    static MonoClass*  cCamU = nullptr;
    static MonoClass*  cTrans = nullptr;
    static MonoMethod* mGetPos = nullptr;
    static MonoMethod* mW2S = nullptr;
    static MonoMethod* mGetTrans = nullptr;
    static Vec3 s_camW = { 0, 0, 0 }; // posicao da camera do ciclo (gate skeleton)
    static bool s_camWok = false;
    static float s_skDist2 = -1.0f; // dist2 da entidade atual (gate skeleton, sem invoke)
    // Item 14 LOS: Physics dota o teste de oclusao sem custo de disposicao.
    typedef unsigned int (__cdecl* FnFieldOffset)(void*); // mono_field_get_offset
    static MonoMethod* mRaycast = nullptr; // Physics.Raycast(Vector3,Vector3,RaycastHit&,Single,Int32)
    static MonoClass*  cRayHit = nullptr;  // UnityEngine.RaycastHit (struct p/ out)
    static MonoClassField* fHitDist = nullptr; // RaycastHit.distance (offset real via API)
    static MonoClassField* fHitCol = nullptr;  // RaycastHit.collider (p/ log layer)
    static MonoClass*  cCollider = nullptr; // UnityEngine.Collider (gameObject/layer)
    static MonoMethod* mGetHitGO = nullptr; // Collider.get_gameObject (log HIT)
    static MonoMethod* mGetLayer = nullptr; // GameObject.get_layer (log HIT)
    static FnFieldOffset pFieldOff = nullptr; // mono_field_get_offset
    static int s_hitDistOff = 28; // RAW (sem header MonoObject). Era 20 (chute) e
    // 44 (api COM header +16) — ambos errados p/ o hitBuf cru; ver [FIELDS].
    static bool s_distRawOk = false; // true quando o raw foi derivado na enumeracao
    static void* s_hystEnt[256];      // histerese LOS: entidade -> cor exibida estavel
    static signed char s_hystStreak[256]; // +exposto / -ocluido (satura em +/-5)
    static bool  s_hystShown[256];    // cor exibida atual (vira após ~3 ciclos iguais)
    static int s_losLogN = 0; // log [LOS] por entidade (1x cada, sem spam)
    static int s_geomMask = -1; // layer mask (auditoria Passo 2; default = tudo)
    static bool      s_losOk = false; // Physics.Raycast resolvido e funcional
    static int       s_rayArgs = 0; // aridade resolvida (2-5)
    // Snapshot double-buffer sem lock no frame (item 14b, Rodada 1 inocentou
    // a worker): Present NUNCA toca em CS — le o ponteiro do buffer pronto
    // (troca atomica). Worker publica no back e vira o ponteiro sob 1 CS curto.
    // memcpy de ~38KB sob lock + Map/Unmap concorrendo = hang em iGPU (23:25).
    static EspEntry s_espA[128];
    static EspEntry s_espB[128];
    static EspEntry* s_espFront = s_espA; // lido pelo Present (sem lock)
    static EspEntry* s_espBack = s_espB;  // escrito pela worker (sob CS curto)
    static int s_espNFront = 0;
    static int s_espNBack = 0;
    static EspEntry s_esp[128]; // legado: mantido p/ diff minimo (nao usado)
    static int s_espN = 0;
    static CRITICAL_SECTION s_espCS;
    static bool s_csInit = false;
    static HANDLE s_espThread = nullptr;
    static volatile bool s_espRun = false;
    static float s_dbgEyeY = 0, s_dbgFootY = 0; // medida real p/ calibrar a box

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

        // Formata "atual/limite" p/ log (sem printf no caminho quente).
        static void BudgetFmt(char* out, size_t cap) {
            int used = s_budgetMax - (s_budgetLeft < 0 ? 0 : s_budgetLeft);
            if (!out || cap < 16) return;
            int v = used, m = s_budgetMax, i = 0;
            char tmp[16]; int tn = 0;
            if (v == 0) tmp[tn++] = '0';
            else { char r[12]; int rn = 0; while (v > 0 && rn < 11) { r[rn++] = (char)('0' + v % 10); v /= 10; } while (rn > 0) tmp[tn++] = r[--rn]; }
            tmp[tn++] = '/';
            if (m == 0) tmp[tn++] = '0';
            else { char r[12]; int rn = 0; while (m > 0 && rn < 11) { r[rn++] = (char)('0' + m % 10); m /= 10; } while (rn > 0) tmp[tn++] = r[--rn]; }
            tmp[tn] = 0;
            for (i = 0; i <= tn && (size_t)i < cap; ++i) out[i] = tmp[i];
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
        long long t0 = PiNow();
        bool out = false;
        __try {
            MonoObject* exc = nullptr;
            MonoObject* ret = pInvoke(m, obj, nullptr, &exc);
            if (exc || !ret) { PiAdd(s_piHas, PiNow() - t0, false); return false; }
            unsigned char v = *(unsigned char*)pUnbox(ret);
            out = v != 0;
        } __except (EXCEPTION_EXECUTE_HANDLER) { PiAdd(s_piHas, PiNow() - t0, false); return false; }
        PiAdd(s_piHas, PiNow() - t0, true);
        return out;
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
            // Auditoria overloads: enumeracao + assinatura (nao-fatal se ausente).
            Bind(m, "mono_method_desc_new", pDescNew);
            Bind(m, "mono_method_desc_search_in_class", pDescSearch);
            Bind(m, "mono_method_desc_free", pDescFree);
            Bind(m, "mono_class_get_methods", pClassMethods);
            Bind(m, "mono_method_signature", pSigOf);
            Bind(m, "mono_signature_get_param_count", pSigCount);
            Bind(m, "mono_signature_get_params", pSigParam);
            Bind(m, "mono_type_get_type", pTypeKind);
            Bind(m, "mono_type_get_name", pTypeName);
            Bind(m, "mono_method_get_name", pMethodGetName);
            Bind(m, "mono_class_get_fields", pClassFields);
            Bind(m, "mono_field_get_name", pFieldGetName);
            Bind(m, "mono_field_get_offset", pFieldGetOff); // pode nao existir no Unity 6 (nao-fatal)
            ok &= Bind(m, "mono_runtime_invoke", pInvoke);
            ok &= Bind(m, "mono_object_unbox", pUnbox);
            ok &= Bind(m, "mono_string_to_utf8", pStrUtf8);
            ok &= Bind(m, "mono_free", pFree);
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
            if (cCamU) { MonoMethod* t = pMethodFrom(cCamU, "get_worldToCameraMatrix", 0); if (t) { mGetViewMat = t; s.resolvedMethods++; } else Log::Warn("Metodo nao resolvido: Camera.get_worldToCameraMatrix/0"); }
            if (cCamU) { MonoMethod* t = pMethodFrom(cCamU, "get_projectionMatrix", 0); if (t) { mGetProjMat = t; s.resolvedMethods++; } else Log::Warn("Metodo nao resolvido: Camera.get_projectionMatrix/0"); }
            if (cTrans) ResolveMethod(cTrans, "Transform", "get_position", 0, mGetPos);
            if (cTrans) ResolveMethod(cTrans, "Transform", "get_rotation", 0, mGetRot);
            if (cComp) ResolveMethod(cComp, "Component", "get_transform", 0, mGetTrans);
            if (cComp) ResolveMethod(cComp, "Component", "get_gameObject", 0, mGetGO);
            MonoClass* cObj = pClassFrom(s_unity, "UnityEngine", "Object");
            if (cObj) { s.resolvedClasses++; ResolveMethod(cObj, "Object", "get_name", 0, mGetName); }
            MonoClass* cRend = pClassFrom(s_unity, "UnityEngine", "Renderer");
            if (cRend) { s.resolvedClasses++; ResolveMethod(cRend, "Renderer", "get_bounds", 0, mGetBounds); }
            // Item 14: Physics.Raycast p/ LOS camera->peito (visivel/invisivel).
            // AUDITORIA: mono_class_get_method_from_name("Raycast",N) retorna o
            // PRIMEIRO overload com N params — sem checar assinatura. Com 6+
            // overloads de Raycast, /5 pode devolver (Ray,...) em vez de
            // (origin,dir,hit,...). Por isso: enumera TODOS os Raycast via
            // mono_class_get_methods + assinatura, e casa por TIPOS. Fallback:
            // Linecast (start,end[,mask]) e descritor ":Raycast(...)".
            // NOTA: mLinecast/s_lineArgs sao statics de namespace (visiveis em BuildEsp).
            // BUG 1 fix: Physics NAO esta em CoreModule no Unity 6 — esta em
            // UnityEngine.PhysicsModule. Tenta PhysicsModule 1o (log prova qual veio),
            // fallback p/ CoreModule (builds antigas). Idem RaycastHit/Collider.
            MonoImage* s_physmod = pImgLoaded("UnityEngine.PhysicsModule");
            if (!s_physmod) s_physmod = pImgLoaded("UnityEngine.PhysicsModule.dll");
            Log::Infof("[LOS-AUDIT] PhysicsModule=%s img=0x%p.",
                s_physmod ? "OK" : "AUSENTE", s_physmod);
            MonoImage* physImg = s_physmod ? s_physmod : s_unity;
            MonoClass* cPhys = pClassFrom(physImg, "UnityEngine", "Physics");
            if (cPhys) {
                s.resolvedClasses++;
                // 1) Auditoria [SIG]: enumera TODOS os overloads de Raycast/Linecast
                // com os TIPOS de cada parametro (nao so aridade). Regra: NUNCA
                // confiar em pMethodFrom por aridade em metodos com overloads de
                // mesma aridade (Unity 6 tem 2x Linecast/4: A=(V3,V3,out,int) e
                // B=(V3,V3,int,QueryTrigger)). Invocar o errado = crash.
                // Guarda o MonoMethod* exato de cada assinatura desejada.
                static MonoMethod* s_rayV3 = nullptr; // (V3,V3,RaycastHit&,Single,Int32)
                static MonoMethod* s_lineV3 = nullptr; // Linecast (V3,V3,RaycastHit&,Int32)
                if (pClassMethods && pSigOf && pSigCount && pMethodGetName && pSigParam && pTypeName) {
                    void* iter = nullptr;
                    int nTotal = 0, nRay = 0, nLine = 0;
                    while (true) {
                        MonoMethod* mm = (MonoMethod*)pClassMethods(cPhys, &iter);
                        if (!mm) break;
                        if (++nTotal > 256) break;
                        const char* nm = pMethodGetName(mm);
                        if (!nm) continue;
                        bool isRay = !strcmp(nm, "Raycast");
                        bool isLine = !strcmp(nm, "Linecast");
                        if (!isRay && !isLine) continue;
                        void* sg = pSigOf(mm);
                        if (!sg) continue;
                        int ac = pSigCount(sg);
                        char p0[64] = "?", p1[64] = "?", p2[64] = "?", p3[64] = "?", p4[64] = "?", p5[64] = "?";
                        char* slots[6] = { p0, p1, p2, p3, p4, p5 };
                        void* piter = nullptr;
                        for (int pi = 0; pi < ac && pi < 6; ++pi) {
                            void* pt = nullptr;
                            __try { pt = pSigParam(sg, &piter); } __except (EXCEPTION_EXECUTE_HANDLER) { pt = nullptr; }
                            if (!pt) break;
                            const char* tn = nullptr;
                            __try { tn = pTypeName(pt); } __except (EXCEPTION_EXECUTE_HANDLER) { tn = nullptr; }
                            if (tn) strncpy_s(slots[pi], 64, tn, _TRUNCATE);
                        }
                        Log::Infof("[SIG] %s argc=%d p0=%s p1=%s p2=%s p3=%s p4=%s p5=%s",
                            nm, ac, p0, p1, p2, p3, p4, p5);
                        // Casa pela assinatura STRING (byref = '&' no fim, padrao Mono):
                        // A) Raycast(V3,V3,RaycastHit&,Single,Int32)
                        // B) Linecast(V3,V3,RaycastHit&,Int32)
                        if (isRay && ac == 5 && !strcmp(p0, "UnityEngine.Vector3")
                            && !strcmp(p1, "UnityEngine.Vector3")
                            && strstr(p2, "RaycastHit") && !strcmp(p4, "System.Int32")) {
                            if (!s_rayV3) { s_rayV3 = mm; nRay += 100; Log::Info("[SIG] Raycast(V3,V3,Hit&,f,i) CONFIRMADO."); }
                        }
                        if (isLine && ac == 4 && !strcmp(p0, "UnityEngine.Vector3")
                            && !strcmp(p1, "UnityEngine.Vector3")
                            && strstr(p2, "RaycastHit") && !strcmp(p3, "System.Int32")) {
                            if (!s_lineV3) { s_lineV3 = mm; nLine += 100; Log::Info("[SIG] Linecast(V3,V3,Hit&,i) CONFIRMADO."); }
                        }
                        if (isRay) nRay++;
                        if (isLine) nLine++;
                    }
                    Log::Infof("[LOS-AUDIT] Physics declarados=%d Raycast~%d Linecast~%d (100+=assinatura confirmada).", nTotal, nRay, nLine);
                } else Log::Warn("LOS-AUDIT sem API de assinatura completa (pClassMethods/pSigOf/pSigCount/pMethodGetName/pSigParam/pTypeName).");
                // mRaycast = assinatura confirmada OU descritor exato; aridade sozinha = PROIBIDO.
                mRaycast = s_rayV3;
                if (mRaycast) { s_rayArgs = 5; s.resolvedMethods++; Log::Info("LOS-SIG Raycast/5 via assinatura confirmada [SIG]."); }
                // 2) Tenta por descritor exato (origin,dir,hit,maxDist,mask).
                // BUG 1 fix: pDescSearch(desc, klass) — estava invertido.
                // BUG 2 fix: descritor precisa do nome da classe "Physics:Raycast(...)".
                if (pDescNew && pDescSearch && pDescFree) {
                    void* dd = pDescNew("Physics:Raycast(UnityEngine.Vector3,UnityEngine.Vector3,UnityEngine.RaycastHit&,System.Single,System.Int32)", 1);
                    if (dd) {
                        MonoMethod* t = (MonoMethod*)pDescSearch(dd, cPhys);
                        if (t) { mRaycast = t; s_rayArgs = 5; s.resolvedMethods++; Log::Info("LOS-SIG Raycast/5 via descritor exato."); }
                        else Log::Warn("LOS-SIG descritor nao casou (fallback por aridade).");
                        pDescFree(dd);
                    } else Log::Warn("LOS-SIG mono_method_desc_new retornou null.");
                }
                // 3) Linecast DESABILITADO (PASSO 1 — parar o crash).
                // Motivo: 2 overloads /4 com mesma aridade; pMethodFrom nao
                // distingue (V3,V3,out,int) de (V3,V3,int,QueryTrigger). Invocar
                // o B corrompe a pilha Mono. Reabilitar so apos [SIG] confirmar
                // s_lineV3 com assinatura exata.
                mLinecast = nullptr; s_lineArgs = 0;
                if (s_lineV3) {
                    mLinecast = s_lineV3; s_lineArgs = 4; s.resolvedMethods++;
                    Log::Info("LOS-SIG Physics.Linecast/4 via assinatura confirmada [SIG].");
                } else Log::Warn("Linecast DESABILITADO (assinatura (V3,V3,Hit&,i) nao confirmada — sem crash).");
                if (mRaycast) Log::Infof("LOS-SIG Physics.Raycast/%d resolvido.", s_rayArgs);
                else Log::Warn("Metodo nao resolvido: Physics.Raycast (2-5 + descritor)");
                if (mRaycast && !mLinecast) Log::Warn("Linecast ausente; Raycast e a unica via.");
                if (!cPhys) Log::Warn("Physics NAO encontrado em PhysicsModule nem CoreModule (BUG 1 persiste).");
            }
            cRayHit = pClassFrom(physImg, "UnityEngine", "RaycastHit");
            if (!cRayHit) cRayHit = pClassFrom(s_unity, "UnityEngine", "RaycastHit");
            if (cRayHit) {
                s.resolvedClasses++;
                // PASSO 3: auditoria [FIELDS] — enumera TODOS os campos com nome
                // + offset (nome pode ser m_Distance; offset pode != 20).
                if (pClassFields && pFieldGetName) {
                    void* fiter = nullptr;
                    int nF = 0;
                    while (true) {
                        MonoClassField* ff = (MonoClassField*)pClassFields(cRayHit, &fiter);
                        if (!ff) break;
                        if (++nF > 64) break;
                        const char* fn = nullptr;
                        __try { fn = pFieldGetName(ff); } __except (EXCEPTION_EXECUTE_HANDLER) { fn = nullptr; }
                        int fo = -1;
                        if (pFieldGetOff) { __try { fo = pFieldGetOff(ff); } __except (EXCEPTION_EXECUTE_HANDLER) { fo = -1; } }
                        Log::Infof("[FIELDS] RaycastHit.%s @ %d", fn ? fn : "?", fo);
                        // Registra o handle exato do campo de distancia (qualquer nome).
                        // CORRECAO offset-16: mono_field_get_offset inclui o header
                        // MonoObject (16 bytes no x64: vtable+sync). Nosso hitBuf e
                        // cru (sem header), entao RAW = API - 16. Ex: 44 -> 28.
                        if (fn && (strstr(fn, "istance") || strstr(fn, "ISTANCE"))) {
                            if (!fHitDist) { fHitDist = ff; s.resolvedFields++; }
                            if (fo >= 16) {
                                s_hitDistOff = fo - 16;
                                s_distRawOk = true;
                                Log::Infof("[FIELDS] distance api=%d raw=%d (hitBuf cru, sem header).", fo, s_hitDistOff);
                            } else if (fo >= 0) { s_hitDistOff = fo; Log::Infof("[FIELDS] distance offset=%d (via enumeracao).", fo); }
                        }
                    }
                }
                if (!fHitDist) {
                    fHitDist = pFieldFrom(cRayHit, "distance");
                    if (fHitDist) s.resolvedFields++;
                    else Log::Warn("Campo nao resolvido: RaycastHit.distance (nem via [FIELDS])");
                }
                fHitCol = pFieldFrom(cRayHit, "m_Collider");
                if (!fHitCol) fHitCol = pFieldFrom(cRayHit, "collider");
                if (fHitCol) s.resolvedFields++;
                else Log::Warn("Campo nao resolvido: RaycastHit.collider");
            }
            cCollider = pClassFrom(physImg, "UnityEngine", "Collider");
            if (!cCollider) cCollider = pClassFrom(s_unity, "UnityEngine", "Collider");
            if (cCollider && cComp) {
                MonoMethod* t = pMethodFrom(cCollider, "get_gameObject", 0);
                if (t) { mGetHitGO = t; s.resolvedMethods++; }
            }
            MonoClass* cGO = pClassFrom(s_unity, "UnityEngine", "GameObject");
            if (cGO) {
                MonoMethod* t = pMethodFrom(cGO, "get_layer", 0);
                if (t) { mGetLayer = t; s.resolvedMethods++; }
            }
            // Offset REAL do campo via mono_field_get_offset.
            // NOTA: mono-2.0-bdwgc.dll do Unity 6 NAO exporta esta funcao.
            // Workaround: LosCalibrate descobre o slot comparando o buffer do
            // hit com a distancia conhecida (auto-calibracao no 1o OCC real).
            // BUG 3 doc: fallback +20 e chute; LosCalibrate corrige em runtime.
            {
                HMODULE mm = GetModuleHandleW(L"mono-2.0-bdwgc.dll");
                if (mm) pFieldOff = (FnFieldOffset)GetProcAddress(mm, "mono_field_get_offset");
                if (pFieldOff && fHitDist) {
                    int apiOff = (int)pFieldOff(fHitDist);
                    s_hitDistOff = (apiOff >= 16) ? apiOff - 16 : apiOff; // raw: sem header
                    s_distRawOk = true;
                    Log::Infof("LOS RaycastHit.distance api=%d raw=%d.", apiOff, s_hitDistOff);
                } else {
                    Log::Warnf("mono_field_get_offset ausente; distance usa fallback +%d (LosCalibrate corrige).", s_hitDistOff);
                }
            }
            s_losOk = (mRaycast && cRayHit && fHitDist);
            Log::Infof("LOS %s (mask=0x%X).", s_losOk ? "OK (Raycast/5 + HitGO/layer)" : "INDISPONIVEL (tudo visivel)", (unsigned)s_geomMask);
        }                     else Log::Warn("Imagem UnityEngine.CoreModule nao carregada.");
        // Passo 2: auditoria de layers via CE MCP (preencher GEOMETRY_MASK apos ler o log [LOS-HIT]).

        s.ready = (cDay && cPlayer && cZombie && cZLoader && cPlayers
            && fDayInst && fZLInst && fPCInst && mHasLocal);
        if (s.ready && !s_csInit) {
            InitializeCriticalSection(&s_espCS);
            s_csInit = true;
            s_espRun = true;
            s_espThread = CreateThread(nullptr, 0, EspThread, nullptr, 0, nullptr);
            Log::Infof("Worker ESP %s.", s_espThread ? "criada" : "FALHOU");
        }
        if (s.ready && !s_logged) {
            s_logged = true;
            Log::Infof("Mono resolve OK: %d classes, %d campos, %d metodos.",
                s.resolvedClasses, s.resolvedFields, s.resolvedMethods);
        }
        return s.ready;
    }

    // Item 14 LOS multi-bone: 1 ponto por osso (cabeca/peito/quadril/coxas).
    // maxDist = ate o osso - 0.15m (nao acerta o proprio zumbi). Hit = ocluido.
    // Retorna true=ponto exposto. Fail-open: falha = exposto (nunca some ESP).
    // DIAG 16/09: Physics.Raycast/5 NAO resolveu nesta build (s_losOk=false desde
    // o init) — todo LosPoint retorna true pelo gate acima. Quando resolver,
    // a calibracao do offset de distance sai do proprio [LOS-CAL] abaixo.
    static int s_calDone = 0; // calibracao RaycastHit.distance (1x, ver LosCalibrate)
    // Histerese LOS: a cor exibida so vira apos ~3 ciclos iguais da mesma
    // entidade (anti-flicker). Retorna a cor ESTAVEL; atualiza o streak.
    // skip=true (ciclo pulado por orcamento/rodizio): NAO mexe no streak,
    // devolve a cor exibida atual (ou a crua p/ entidade nova). Sem isso, o
    // rodizio apagava o vermelho: pular ciclo virava verde (bug 16/09).
    static bool LosStable(void* ent, bool rawVis, bool skip = false) {
        for (int i = 0; i < 256; ++i) {
            if (s_hystEnt[i] == ent) {
                if (skip) return s_hystShown[i];
                int s = (int)s_hystStreak[i] + (rawVis ? 1 : -1);
                if (s > 5) s = 5; if (s < -5) s = -5;
                s_hystStreak[i] = (signed char)s;
                if (s >= 3) s_hystShown[i] = true;      // 3x exposto -> vira verde
                else if (s <= -3) s_hystShown[i] = false; // 3x ocluido -> vira vermelho
                return s_hystShown[i];
            }
            if (!s_hystEnt[i]) { // slot livre: nasce na cor crua
                s_hystEnt[i] = ent;
                s_hystStreak[i] = rawVis ? 1 : -1;
                s_hystShown[i] = rawVis;
                return rawVis;
            }
        }
        return rawVis; // tabela cheia: sem histerese
    }
    static void LosCalibrate(const unsigned char* hitBuf, float knownDist) {
        if (s_calDone || !(knownDist > 1.0f)) return;
        if (s_distRawOk) { s_calDone = 1; return; } // raw ja derivado: nao sobrescrever
        // Procura o slot float cujo valor ~= knownDist (hit confirmado pelo bool).
        for (int off = 0; off + 4 <= 128; off += 4) {
            float v = 0;
            __try { memcpy(&v, hitBuf + off, sizeof(v)); }
            __except (EXCEPTION_EXECUTE_HANDLER) { return; }
            if (v == v && v > 0.5f && v < knownDist && (knownDist - v) < knownDist * 0.5f + 1.0f) {
                // Candidato: hit antes do alvo. Exige estabilidade 2x antes de travar.
                static int candOff = -1, candHits = 0;
                if (off == candOff) {
                    if (++candHits >= 2) {
                        s_hitDistOff = off;
                        s_calDone = 1;
                        Log::Infof("[LOS-CAL] distance offset=%d (hit=%.1f alvo=%.1f).", off, (double)v, (double)knownDist);
                    }
                } else { candOff = off; candHits = 1; }
                return;
            }
        }
    }
    // Linecast tri-estado (fia de verdade: 1=exposto, 0=ocluido, -1=sem dado).
    // /4 le o RaycastHit e ignora o corpo do proprio alvo (hd ~= dist); /2 sem
    // hitInfo e fail-conservador. Sem dado NUNCA vira verde (igual LosPointV).
    static int LosPointLineV(const Vec3& from, const Vec3& to) {
        if (!mLinecast || s_lineArgs == 0) return -1;
        float dx = to.x - from.x, dy = to.y - from.y, dz = to.z - from.z;
        float dist = sqrtf(dx * dx + dy * dy + dz * dz);
        if (!(dist > 0.5f) || !(dist < 10000.0f)) return -1;
        long long t0 = PiNow();
        __try {
            int mask = s_geomMask;
            MonoObject* exc = nullptr;
            MonoObject* ret = nullptr;
            if (s_lineArgs == 4) {
                unsigned char hitBuf[128] = { 0 };
                void* args[4];
                args[0] = (void*)&from;
                args[1] = (void*)&to;
                args[2] = (void*)hitBuf;
                args[3] = (void*)&mask;
                ret = pInvoke(mLinecast, nullptr, args, &exc);
                if (exc || !ret) { PiAddEx(s_piRay, PiNow() - t0, 0); return -1; }
                bool hit = (*(unsigned char*)pUnbox(ret)) != 0;
                if (!hit) { PiAdd(s_piRay, PiNow() - t0, true); return 1; }
                float hd = 0;
                __try { memcpy(&hd, hitBuf + s_hitDistOff, sizeof(hd)); }
                __except (EXCEPTION_EXECUTE_HANDLER) { PiAddEx(s_piRay, PiNow() - t0, 2); return -1; }
                if (!(hd == hd) || hd <= 0) { PiAddEx(s_piRay, PiNow() - t0, 1); return -1; }
                if (hd >= dist - 0.15f) { PiAdd(s_piRay, PiNow() - t0, true); return 1; }
                PiAdd(s_piRay, PiNow() - t0, true);
                return 0;
            }
            // s_lineArgs == 2: (start,end) — hit de qualquer coisa = ocluido.
            void* args[2];
            args[0] = (void*)&from;
            args[1] = (void*)&to;
            ret = pInvoke(mLinecast, nullptr, args, &exc);
            if (exc || !ret) { PiAddEx(s_piRay, PiNow() - t0, 0); return -1; }
            bool hit = (*(unsigned char*)pUnbox(ret)) != 0;
            PiAdd(s_piRay, PiNow() - t0, true);
            return !hit ? 1 : 0;
        } __except (EXCEPTION_EXECUTE_HANDLER) { PiAddEx(s_piRay, PiNow() - t0, 2); return -1; }
    }
    // Tri-estado do ponto (briefing §6 Camada 2/3): 1=exposto, 0=ocluido,
    // -1=sem dado (sem saldo, distancia invalida, invoke falhou). O chamador
    // decide: sem dado em TODOS os pontos = mantem a cor anterior (decay),
    // nunca verde forcado. Verde forcado era o pisca-pisca em porta/janela.
    static int LosPointV(const Vec3& from, const Vec3& to, float* outHit, int* outLayer = nullptr) {
        if (outHit) *outHit = 0;
        if (!s_losOk) return -1;
        if (!BudgetTake(1)) { s_losSkipped++; return -1; }
        float dx = to.x - from.x, dy = to.y - from.y, dz = to.z - from.z;
        float dist = sqrtf(dx * dx + dy * dy + dz * dz);
        if (!(dist > 0.5f) || !(dist < 10000.0f)) return -1;
        long long t0 = PiNow();
        bool piOk = true;
        Vec3 dir = { dx / dist, dy / dist, dz / dist };
        unsigned char hitBuf[128] = { 0 };
        void* args[5];
        args[0] = (void*)&from;
        args[1] = (void*)&dir;
        __try {
            // Assinaturas Unity (ordens testadas no init: s_rayArgs):
            // /2: (ray, maxDistance) | /3: +layerMask | /4: +hitInfo | /5: +hitInfo+mask.
            float maxD = dist - 0.15f;
            int mask = s_geomMask;
            MonoObject* exc = nullptr;
            MonoObject* ret = nullptr;
            bool hit = false;
            if (s_rayArgs == 5) {
                args[2] = (void*)hitBuf;
                args[3] = (void*)&maxD;
                args[4] = (void*)&mask;
                ret = pInvoke(mRaycast, nullptr, args, &exc);
                if (exc || !ret) { PiAddEx(s_piRay, PiNow() - t0, 0); return -1; }
                hit = (*(unsigned char*)pUnbox(ret)) != 0;
            } else if (s_rayArgs == 4) {
                args[2] = (void*)hitBuf;
                args[3] = (void*)&maxD;
                ret = pInvoke(mRaycast, nullptr, args, &exc);
                if (exc || !ret) { PiAddEx(s_piRay, PiNow() - t0, 0); return -1; }
                hit = (*(unsigned char*)pUnbox(ret)) != 0;
            } else if (s_rayArgs == 3) {
                args[2] = (void*)&maxD;
                args[3] = (void*)&mask;
                ret = pInvoke(mRaycast, nullptr, args, &exc);
                if (exc || !ret) { PiAddEx(s_piRay, PiNow() - t0, 0); return -1; }
                hit = (*(unsigned char*)pUnbox(ret)) != 0;
                if (hit) { if (outHit) *outHit = maxD; PiAdd(s_piRay, PiNow() - t0, true); return 0; }
                PiAdd(s_piRay, PiNow() - t0, true); return 1;
            } else if (s_rayArgs == 2) {
                args[2] = (void*)&maxD;
                ret = pInvoke(mRaycast, nullptr, args, &exc);
                if (exc || !ret) { PiAddEx(s_piRay, PiNow() - t0, 0); return -1; }
                hit = (*(unsigned char*)pUnbox(ret)) != 0;
                if (hit) { if (outHit) *outHit = maxD; PiAdd(s_piRay, PiNow() - t0, true); return 0; }
                PiAdd(s_piRay, PiNow() - t0, true); return 1;
            } else { PiAddEx(s_piRay, PiNow() - t0, 0); return -1; }
            if (!hit) { PiAdd(s_piRay, PiNow() - t0, true); return 1; } // sem hit = exposto
            if (!s_calDone) LosCalibrate(hitBuf, dist);
            float hd = 0;
            __try { memcpy(&hd, hitBuf + s_hitDistOff, sizeof(hd)); }
            __except (EXCEPTION_EXECUTE_HANDLER) { PiAddEx(s_piRay, PiNow() - t0, 2); return -1; }
            if (outHit) *outHit = hd;
            if (!(hd == hd) || hd <= 0) { PiAddEx(s_piRay, PiNow() - t0, 1); return -1; }
            if (hd >= dist - 0.15f) { PiAdd(s_piRay, PiNow() - t0, true); return 1; } // encosto no corpo
            // Log HIT DESABILITADO (P0 crash em aproximacao 15/09): invocar
            // Collider.get_gameObject / GameObject.get_layer num collider que o
            // jogo pode estar destruindo (LODController.SetColliding/GameObject.
            // SetActive na mesma janela — ver crash dump 10:57) = AV dentro do
            // runtime Mono. A decisao visivel/invisivel NAO usa layer, so distancia.
            if (outLayer) *outLayer = -1;
            PiAdd(s_piRay, PiNow() - t0, true);
            return 0;
        } __except (EXCEPTION_EXECUTE_HANDLER) { PiAddEx(s_piRay, PiNow() - t0, 2); return -1; }
    }
    // Compat: LosPoint antigo (bool) vira LosPointV (o chamador termico ignora -1).
    static bool LosPoint(const Vec3& from, const Vec3& to, float* outHit, int* outLayer) {
        float hd = 0; int dumL = -1;
        int v = LosPointV(from, to, outHit ? outHit : &hd);
        if (outLayer) *outLayer = dumL;
        return v >= 0 ? (v == 1) : true;
    }
    // REGRA PROPORCIONAL: 5 pontos (cabeca/peito/quadril/coxaL/coxaR).
    // visivel = hits >= 2 (40%) OU cabeca exposta (headshot viavel).
    // Log [LOS] por entidade 1x (Passo 1/7): id/dist/hits/visivel.
    static bool LosMulti(const Vec3& from, const Vec3 pts[5], float dist, void* ent) {
        if (!s_losOk || !Config::bVisibleCheck) return true;
        int hits = 0;
        bool headExp = false;
        int layers[5] = { -1,-1,-1,-1,-1 };
        float hd[5] = { 0,0,0,0,0 };
        for (int i = 0; i < 5; ++i) {
            if (LosPoint(from, pts[i], &hd[i], &layers[i])) {
                hits++;
                if (i == 0) headExp = true;
            }
        }
        bool vis = (hits >= 2) || headExp;
        if (s_losLogN < 40) {
            s_losLogN++;
            Log::Infof("[LOS] id=0x%p dist=%.1f hits=%d/5 visivel=%d (lay=%d,%d,%d,%d,%d)",
                ent, (double)dist, hits, vis ? 1 : 0,
                layers[0], layers[1], layers[2], layers[3], layers[4]);
        }
        return vis;
    }

    // invoke Transform.get_position -> mundo. Retorna false se falhar.
    // GetPos fora do orçamento (posicao e dado vital: box/skeleton/LOS
    // dependem dela; sem posicao a entidade some). O teto que protege a
    // horda e o de entidades/ciclo + LOS em rodizio, nao este.
    static bool GetPos(void* trans, Vec3& out) {
        if (!mGetPos || !trans) return false;
        long long t0 = PiNow();
        bool ok = false;
        __try {
            MonoObject* exc = nullptr;
            MonoObject* ret = pInvoke(mGetPos, trans, nullptr, &exc);
            if (exc || !ret) { PiAdd(s_piPos, PiNow() - t0, false); return false; }
            memcpy(&out, pUnbox(ret), sizeof(out));
            ok = true;
        } __except (EXCEPTION_EXECUTE_HANDLER) { ok = false; }
        PiAdd(s_piPos, PiNow() - t0, ok);
        return ok;
    }

    void SetViewport(float w, float h) {
        if (w > 100 && h > 100) { s_vpW = w; s_vpH = h; }
    }

    // Item 14/Passo 5: NDC de profundidade de um ponto de MUNDO via VP proprio.
    // Mesma matematica do W2S, mas retorna o Z clip (0=perto, 1=longe) em vez do pixel.
    // Comparado com o depth buffer da cena: osso mais fundo que a cena = ocluido.
    static bool BoneNdc(const Vec3& w, float& outNdc, float& outU, float& outV) {
        if (!s_vpOk || s_vpW < 64 || s_vpH < 64) return false;
        float cx = s_vp[0] * w.x + s_vp[4] * w.y + s_vp[8] * w.z + s_vp[12];
        float cy = s_vp[1] * w.x + s_vp[5] * w.y + s_vp[9] * w.z + s_vp[13];
        float cz = s_vp[2] * w.x + s_vp[6] * w.y + s_vp[10] * w.z + s_vp[14];
        float cw = s_vp[3] * w.x + s_vp[7] * w.y + s_vp[11] * w.z + s_vp[15];
        if (!(cw > 0.05f)) return false;
        float inv = 1.0f / cw;
        float nx = cx * inv, ny = cy * inv, nz = cz * inv;
        if (!(nx == nx && ny == ny && nz == nz)) return false;
        if (nx < -1.2f || nx > 1.2f || ny < -1.2f || ny > 1.2f) return false;
        outNdc = nz * 0.5f + 0.5f; // clip [-1,1] -> depth [0,1] (D3D)
        outU = nx * 0.5f + 0.5f;
        outV = 1.0f - (ny * 0.5f + 0.5f); // NDC y-up -> UV y-down (texel)
        return true;
    }
    static int s_depthOk = 0, s_depthMiss = 0; // diagnostico depth (amostragem)
    static Vec3 s_depthProbe[5]; // pontos do ultimo ciclo p/ diagnostico DEPTH-ROW
    static bool s_depthProbeOn = false;
    // Ponto exposto? Compara NDC do osso com a cena. Epsilon 0.001 + margem de
    // 0.5m em profundidade (converte: margem relativa a distancia do osso).
    static bool DepthExposed(const Vec3& w, float distToBone) {
        float ndc = 0, u = 0, v = 0;
        if (!BoneNdc(w, ndc, u, v)) return true; // fora da tela = nao decide
        float scene = 0;
        if (!DepthVisShim::Sample(u, v, scene)) {
            if (s_depthMiss < 3) { s_depthMiss++; Log::Warn("Depth sample indisponivel (MSAA/staging?)."); }
            return true; // fail-open: sem depth, exposto
        }
        if (s_depthOk < 2) { s_depthOk++; Log::Infof("Depth OK: osso=%.4f cena=%.4f u=%.2f v=%.2f.", (double)ndc, (double)scene, (double)u, (double)v); }
        // Cena no far (1.0) = ceu: osso sempre exposto.
        if (scene >= 0.999f) return true;
        // Margem: osso ate ~0.5m atras da superficie ainda conta como exposto
        // (espessura do corpo + jitter). Em NDC a margem encolhe com a distancia;
        // aproxima com 0.5m convertido via derivada: eps = 0.5 / dist^2 * k.
        float eps = 0.5f / (distToBone * distToBone + 1.0f) + 0.001f;
        return ndc <= scene + eps;
    }
    // Diagnostico DEPTH-ROW (1x/sessao): NDC do osso vs cena amostrada.
    // Se osso< cena mas marca verde => o sample esta lendo a textura errada.
    static void DepthDiagRow(const Vec3 pts[5], float dist) {
        static bool done = false;
        if (done) return;
        done = true;
        for (int i = 0; i < 5; ++i) {
            float ndc = 0, u = 0, v = 0;
            if (!BoneNdc(pts[i], ndc, u, v)) {
                Log::Infof("[DEPTH-ROW] pt=%d sem NDC.", i);
                continue;
            }
            float scene = 0;
            bool ok = DepthVisShim::Sample(u, v, scene);
            Log::Infof("[DEPTH-ROW] pt=%d osso=%.4f cena=%.4f u=%.3f v=%.3f sample=%d dist=%.1f.",
                i, (double)ndc, (double)scene, (double)u, (double)v, ok ? 1 : 0, (double)dist);
        }
    }

    // Le matriz 4x4 da camera (64 bytes, column-major Unity).
    static bool GetMat(MonoMethod* m, void* cam, float out[16]) {
        if (!m || !cam) return false;
        __try {
            MonoObject* exc = nullptr;
            MonoObject* ret = pInvoke(m, cam, nullptr, &exc);
            if (exc || !ret) return false;
            memcpy(out, pUnbox(ret), 64);
            for (int i = 0; i < 16; ++i) if (!(out[i] == out[i])) return false;
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }

    static void MulVP(const float P[16], const float V[16], float O[16]) {
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                O[i + 4 * j] = P[i] * V[4 * j] + P[i + 4] * V[1 + 4 * j] + P[i + 8] * V[2 + 4 * j] + P[i + 12] * V[3 + 4 * j];
    }

    // invoke Camera.WorldToScreenPoint(mundo) -> pixels Unity (y de baixo p/ cima).
    // P1 (glitch): VP proprio com guard real de clip.w. Fallback = motor (z>1m).
    static bool W2S(void* cam, const Vec3& w, Vec3& out) {
        if (s_vpOk) {
            // Caminho proprio: controle total do clip.w (padrao da industria).
            float cx = s_vp[0] * w.x + s_vp[4] * w.y + s_vp[8] * w.z + s_vp[12];
            float cy = s_vp[1] * w.x + s_vp[5] * w.y + s_vp[9] * w.z + s_vp[13];
            float cw = s_vp[3] * w.x + s_vp[7] * w.y + s_vp[11] * w.z + s_vp[15];
            if (!(cw > 0.05f)) return false; // melee (~1m) ainda projeta; sanidade barra o lixo
            float inv = 1.0f / cw;
            float nx = cx * inv, ny = cy * inv;
            if (!(nx == nx && ny == ny)) return false;
            out.x = (nx * 0.5f + 0.5f) * s_vpW;
            out.y = (ny * 0.5f + 0.5f) * s_vpH;
            out.z = cw;
            return true;
        }
        if (!mW2S || !cam) return false;
        __try {
            void* args[1] = { (void*)&w };
            MonoObject* exc = nullptr;
            MonoObject* ret = pInvoke(mW2S, cam, args, &exc);
            if (exc || !ret) return false;
            memcpy(&out, pUnbox(ret), sizeof(out));
            return out.z > 1.0f;
        } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }

    struct Bnd { Vec3 center, extents; }; // UnityEngine.Bounds (24 bytes)

    // invoke Renderer.get_bounds -> AABB de MUNDO (cobre o corpo todo: pernas,
    // bracos e cabeca, qualquer tamanho/tipo). Com validacao de sanidade.
    static bool GetBounds(void* rend, Bnd& out) {
        if (!mGetBounds || !rend) return false;
        long long t0 = PiNow();
        bool ok = false;
        __try {
            MonoObject* exc = nullptr;
            MonoObject* ret = pInvoke(mGetBounds, rend, nullptr, &exc);
            if (exc || !ret) { PiAdd(s_piBnd, PiNow() - t0, false); return false; }
            memcpy(&out, pUnbox(ret), sizeof(out));
            ok = true;
            if (!(out.extents.x > 0.05f && out.extents.x < 6.0f)) ok = false;
            else if (!(out.extents.y > 0.05f && out.extents.y < 6.0f)) ok = false;
            else if (!(out.extents.z > 0.05f && out.extents.z < 6.0f)) ok = false;
            PiAdd(s_piBnd, PiNow() - t0, ok);
            return ok;
        } __except (EXCEPTION_EXECUTE_HANDLER) { PiAdd(s_piBnd, PiNow() - t0, false); return false; }
    }

    static bool Fin(float v) { return v == v && v > -3.4028235e38f && v < 3.4028235e38f; }

    // P1: o W2S do motor pode retornar x=50000 com z>0 (w~0+ por dentro do motor).
    // Unica defesa: sanidade no ESPACO DE TELA. NaN falha nas comparacoes e cai aqui.
    static bool Sane2(float x, float y) {
        return x > -10000.0f && x < 10000.0f && y > -10000.0f && y < 10000.0f;
    }
    static int s_glitchLogged = 0; // log diagnostico (Passo 7), sem spam

    // Nome via Object.get_name (GameObject). Fallback "Zumbi".
    // DESATIVADA (P0 crash pos-kill no soco). Caminho quente usa "Zombie" fixo.
    static void GetName(void* obj, char* out, size_t cap) {
        (void)obj;
        strncpy_s(out, cap, "Zombie", _TRUNCATE);
    }

    // Item 12 Skeleton real: juntas pelos indices auditados ([BONE] 14/09).
    // Ordem SkJoint: head12 neck11 sp3-10 sp2-9 sp1-8 | perna L: hl1 l1l2 l2l3 fl4
    // perna R: l1r5 l2r6 fr7 (topo = sp1) | braco L: sl13 a1l14 a2l15 | R: sr16 a1r17 a2r18.
    // FIX Bug 3: array com SK_PHYS (18) entradas — iterar so ossos fisicos e
    // calcular HL2L/HL2R explicitamente (fix bugs 1-2: mGetRot + unificado 2D/3D).
    static const int kBoneIdx[SkJoint::SK_PHYS] = {
        12, 11, 10, 9, 8, 1, 2, 3, 4, 5, 6, 7, 13, 14, 15, 16, 17, 18
    };
    // Rotacao do Transform: get_rotation retorna QUATERNION (x,y,z,w — 16 bytes),
    // nao matriz. FIX Bug 4 (causa raiz): copiar 36 bytes do quat lia lixo alem
    // do objeto e a mao ia para dentro do peito. Converte quat->matriz 3x3 aqui.
    static bool GetQuat(void* trans, float q[4]) {
        if (!mGetRot || !trans) return false;
        if (!BudgetTake(1)) return false; // maos viram fallback colinear sem saldo
        long long t0 = PiNow();
        bool ok = false;
        __try {
            MonoObject* exc = nullptr;
            MonoObject* ret = pInvoke(mGetRot, trans, nullptr, &exc);
            if (exc || !ret) { PiAdd(s_piRot, PiNow() - t0, false); return false; }
            memcpy(q, pUnbox(ret), sizeof(float) * 4);
            for (int i = 0; i < 4; ++i) if (!(q[i] == q[i])) { PiAdd(s_piRot, PiNow() - t0, false); return false; }
            float n = sqrtf(q[0]*q[0]+q[1]*q[1]+q[2]*q[2]+q[3]*q[3]);
            if (n < 0.001f) { PiAdd(s_piRot, PiNow() - t0, false); return false; }
            ok = true;
        } __except (EXCEPTION_EXECUTE_HANDLER) { ok = false; }
        PiAdd(s_piRot, PiNow() - t0, ok);
        return ok;
    }
    static void QuatToMat3(const float q[4], float R[9]) {
        float x=q[0], y=q[1], z=q[2], w=q[3];
        float n = sqrtf(x*x+y*y+z*z+w*w);
        if (n > 0.001f) { x/=n; y/=n; z/=n; w/=n; }
        float xx=x*x, yy=y*y, zz=z*z, xy=x*y, xz=x*z, yz=y*z, wx=w*x, wy=w*y, wz=w*z;
        R[0]=1-2*(yy+zz); R[1]=2*(xy-wz);   R[2]=2*(xz+wy);
        R[3]=2*(xy+wz);   R[4]=1-2*(xx+zz); R[5]=2*(yz-wx);
        R[6]=2*(xz-wy);   R[7]=2*(yz+wx);   R[8]=1-2*(xx+yy);
    }
    static bool s_rotWarned = false;

    static bool GetRot(void* trans, float R[9]) {
        float q[4] = { 0 };
        if (!GetQuat(trans, q)) return false;
        QuatToMat3(q, R);
        return true;
    }

    // Calcula o vetor frontal anatomico do torax a partir dos ombros e coluna:
    // right = SR - SL, up = NECK - HL, fwd = normalize(cross(right, up)).
    static bool GetBodyForward(const Vec3 wp[SkJoint::SK_PHYS], const bool wok[SkJoint::SK_PHYS], Vec3& outFwd) {
        if (!wok[SkJoint::SK_SL] || !wok[SkJoint::SK_SR] || !wok[SkJoint::SK_HL] || !wok[SkJoint::SK_NECK])
            return false;
        Vec3 r = { wp[SkJoint::SK_SR].x - wp[SkJoint::SK_SL].x,
                   wp[SkJoint::SK_SR].y - wp[SkJoint::SK_SL].y,
                   wp[SkJoint::SK_SR].z - wp[SkJoint::SK_SL].z };
        Vec3 u = { wp[SkJoint::SK_NECK].x - wp[SkJoint::SK_HL].x,
                   wp[SkJoint::SK_NECK].y - wp[SkJoint::SK_HL].y,
                   wp[SkJoint::SK_NECK].z - wp[SkJoint::SK_HL].z };
        Vec3 f = { r.y * u.z - r.z * u.y,
                   r.z * u.x - r.x * u.z,
                   r.x * u.y - r.y * u.x };
        float len = sqrtf(f.x * f.x + f.y * f.y + f.z * f.z);
        if (len < 0.001f) return false;
        outFwd.x = f.x / len;
        outFwd.y = f.y / len;
        outFwd.z = f.z / len;
        return true;
    }

    // Direcao do antebraco: eixo longitudinal do osso a partir da rotacao do cotovelo.
    // Avalia os eixos locais para selecionar o sentido que projeta para frente (bodyFwd)
    // e/ou continua a extensao do membro (ombro->cotovelo), sem heuristica de trava estatica
    // ou inversoes artificiais sy>0.3 (fix bugs 1-4).
    static bool ForearmFwd(void* elbowBone, const Vec3& shoulderW, const Vec3& elbowW, const Vec3& bodyFwd, bool hasBodyFwd, Vec3& out, int side /*0=L,1=R*/) {
        float R[9] = { 0 };
        if (GetRot(elbowBone, R)) {
            Vec3 bDir = { elbowW.x - shoulderW.x, elbowW.y - shoulderW.y, elbowW.z - shoulderW.z };
            float bLen = sqrtf(bDir.x * bDir.x + bDir.y * bDir.y + bDir.z * bDir.z);
            if (bLen > 0.001f) { bDir.x /= bLen; bDir.y /= bLen; bDir.z /= bLen; }
            else { bDir.x = 0; bDir.y = -1.0f; bDir.z = 0; }

            Vec3 cols[3] = {
                { R[0], R[3], R[6] }, // Local X
                { R[1], R[4], R[7] }, // Local Y (eixo auditado longitudinal do rig ZB2)
                { R[2], R[5], R[8] }  // Local Z
            };

            int bestAxis = 1;
            float bestScore = -9999.0f;
            float bestSign = 1.0f;

            for (int i = 0; i < 3; ++i) {
                float len = sqrtf(cols[i].x * cols[i].x + cols[i].y * cols[i].y + cols[i].z * cols[i].z);
                if (len < 0.001f) continue;
                Vec3 axis = { cols[i].x / len, cols[i].y / len, cols[i].z / len };

                for (float s = 1.0f; s >= -1.0f; s -= 2.0f) {
                    Vec3 cand = { axis.x * s, axis.y * s, axis.z * s };
                    float dotFwd = hasBodyFwd ? (cand.x * bodyFwd.x + cand.y * bodyFwd.y + cand.z * bodyFwd.z) : 0.0f;
                    float dotExt = (cand.x * bDir.x + cand.y * bDir.y + cand.z * bDir.z);
                    // Prioriza sentido para a frente (corrida/ataque) e continuacao do braco
                    float score = (hasBodyFwd ? (dotFwd * 1.5f) : 0.0f) + dotExt;
                    if (score > bestScore) {
                        bestScore = score;
                        bestAxis = i;
                        bestSign = s;
                    }
                }
            }

            Vec3 chosen = { cols[bestAxis].x * bestSign, cols[bestAxis].y * bestSign, cols[bestAxis].z * bestSign };
            float clen = sqrtf(chosen.x * chosen.x + chosen.y * chosen.y + chosen.z * chosen.z);
            if (clen > 0.001f) {
                out.x = chosen.x / clen;
                out.y = chosen.y / clen;
                out.z = chosen.z / clen;
                return true;
            }
        }

        if (!s_rotWarned) { s_rotWarned = true; Log::Warn("get_rotation falhou; mao usa fallback frontal/colinear."); }

        // Fallback robusto sem rotacao: projeta para frente do zumbi
        if (hasBodyFwd) {
            Vec3 fb = { bodyFwd.x * 0.85f, bodyFwd.y * 0.85f - 0.15f, bodyFwd.z * 0.85f };
            float fl = sqrtf(fb.x * fb.x + fb.y * fb.y + fb.z * fb.z);
            if (fl > 0.001f) {
                out.x = fb.x / fl; out.y = fb.y / fl; out.z = fb.z / fl;
                return true;
            }
        }

        Vec3 d = { elbowW.x - shoulderW.x, elbowW.y - shoulderW.y, elbowW.z - shoulderW.z };
        float l = sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);
        if (l < 0.01f) return false;
        out.x = d.x / l; out.y = d.y / l; out.z = d.z / l;
        return true;
    }

    static void CollectJoints(void* zo, void* cam, EspEntry& out) {
        out.skN = SkJoint::SK_COUNT;
        for (int k = 0; k < SkJoint::SK_COUNT; ++k) { out.skV[k] = false; out.skX[k] = out.skY[k] = 0; }
        if (!Config::bZombieSkeleton) { out.skN = 0; return; }
        void* arr = ReadP(zo, Off::ZO_armature);
        if (!arr) { out.skN = 0; return; }
        long long len = 0;
        __try { memcpy(&len, (char*)arr + Off::A_len, sizeof(len)); }
        __except (EXCEPTION_EXECUTE_HANDLER) { out.skN = 0; return; }
        if (len < 19) { out.skN = 0; return; } // rig incompleto: sem skeleton
        void* bones[SkJoint::SK_PHYS] = { nullptr };
        for (int k = 0; k < SkJoint::SK_PHYS; ++k) {
            __try { memcpy(&bones[k], (char*)arr + Off::A_data + (size_t)kBoneIdx[k] * 8, 8); }
            __except (EXCEPTION_EXECUTE_HANDLER) { bones[k] = nullptr; }
        }
        Vec3 wp[SkJoint::SK_PHYS];
        bool wok[SkJoint::SK_PHYS] = { false };
        // Causa A (item 14b): skeleton longe vira box 2D leve. 18 get_position
        // por zumbi x 96 = ~1700 invokes/ciclo — e o LOD mexe nesses mesmos
        // Transforms. Longe (>50m) nao precisa de osso: pula o loop inteiro.
        // A flag bZombieSkeleton continua mandando (respeita o menu).
        // Probe com distancia JA conhecida no ciclo (eye/foot do 2D ou center
        // da AABB do 3D) — nunca invoke extra (o probe com GetPos batia justo
        // no objeto mais fragil: armature se formando no spawn).
        if (s_skDist2 >= 0 && s_skDist2 > 50.0f * 50.0f) { out.skN = 0; return; }
        for (int k = 0; k < SkJoint::SK_PHYS; ++k) {
            if (!bones[k]) continue;
            Vec3 w, s3;
            if (!GetPos(bones[k], w)) continue;
            if (!Fin(w.x) || !Fin(w.y) || !Fin(w.z)) continue;
            wp[k] = w; wok[k] = true;
            if (!W2S(cam, w, s3)) continue;
            if (!Sane2(s3.x, s3.y)) continue;
            out.skX[k] = s3.x; out.skY[k] = s3.y; out.skV[k] = true;
        }
        // Maos estimadas via rotacao do antebraco — unificado p/ 2D e 3D (fix bugs 1-4).
        Vec3 bodyFwd = { 0, 0, 0 };
        bool hasBodyFwd = GetBodyForward(wp, wok, bodyFwd);
        {
            Vec3 fwd;
            if (wok[SkJoint::SK_A1L] && wok[SkJoint::SK_A2L] &&
                ForearmFwd(bones[SkJoint::SK_A2L], wp[SkJoint::SK_A1L], wp[SkJoint::SK_A2L], bodyFwd, hasBodyFwd, fwd, 0)) {
                Vec3 hw = { wp[SkJoint::SK_A2L].x + fwd.x * 0.25f,
                            wp[SkJoint::SK_A2L].y + fwd.y * 0.25f,
                            wp[SkJoint::SK_A2L].z + fwd.z * 0.25f };
                Vec3 s3;
                if (W2S(cam, hw, s3) && Sane2(s3.x, s3.y)) {
                    out.skX[SkJoint::SK_HL2L] = s3.x; out.skY[SkJoint::SK_HL2L] = s3.y;
                    out.skV[SkJoint::SK_HL2L] = true;
                }
            }
            if (wok[SkJoint::SK_A1R] && wok[SkJoint::SK_A2R] &&
                ForearmFwd(bones[SkJoint::SK_A2R], wp[SkJoint::SK_A1R], wp[SkJoint::SK_A2R], bodyFwd, hasBodyFwd, fwd, 1)) {
                Vec3 hw = { wp[SkJoint::SK_A2R].x + fwd.x * 0.25f,
                            wp[SkJoint::SK_A2R].y + fwd.y * 0.25f,
                            wp[SkJoint::SK_A2R].z + fwd.z * 0.25f };
                Vec3 s3;
                if (W2S(cam, hw, s3) && Sane2(s3.x, s3.y)) {
                    out.skX[SkJoint::SK_HL2R] = s3.x; out.skY[SkJoint::SK_HL2R] = s3.y;
                    out.skV[SkJoint::SK_HL2R] = true;
                }
            }
        }
        // Auditoria juntas (1x/sessao): posicao de mundo + direcao ate o pai.
        // Resultado 14/09: braco real = ombro(12)+antebraco(0.21m); sem mao no rig.
        // Mao estimada = ponta do antebraco (padrao grandes cheats p/ rig sem falange).
        if (!s_jointLogged) {
            Vec3 wp[SkJoint::SK_PHYS];
            bool okp[SkJoint::SK_PHYS] = { false };
            for (int k = 0; k < SkJoint::SK_PHYS; ++k) {
                void* bone = nullptr;
                __try { memcpy(&bone, (char*)arr + Off::A_data + (size_t)kBoneIdx[k] * 8, 8); }
                __except (EXCEPTION_EXECUTE_HANDLER) { continue; }
                if (bone && GetPos(bone, wp[k]) && Fin(wp[k].x)) okp[k] = true;
            }
            static const char* JN[SkJoint::SK_PHYS] = { "HEAD","NECK","SP3","SP2","SP1","HL","L1L","L2L","FL","L1R","L2R","FR","SL","A1L","A2L","SR","A1R","A2R" };
            using MJ = SkJoint;
            static const int SEGJ[][2] = {
                { MJ::SK_SP3, MJ::SK_SL }, { MJ::SK_SL, MJ::SK_A1L }, { MJ::SK_A1L, MJ::SK_A2L },
                { MJ::SK_SP3, MJ::SK_SR }, { MJ::SK_SR, MJ::SK_A1R }, { MJ::SK_A1R, MJ::SK_A2R }
            };
            for (int s = 0; s < 6; ++s) {
                int a = SEGJ[s][0], b = SEGJ[s][1];
                if (!okp[a] || !okp[b]) { Log::Infof("[JOINT] %s-%s: leitura falhou", JN[a], JN[b]); continue; }
                float dx = wp[b].x - wp[a].x, dy = wp[b].y - wp[a].y, dz = wp[b].z - wp[a].z;
                float d = sqrtf(dx * dx + dy * dy + dz * dz);
                Log::Infof("[JOINT] %s->%s len=%.2fm dir=(%.2f,%.2f,%.2f) paio=(%.1f,%.1f,%.1f) filho=(%.1f,%.1f,%.1f)",
                    JN[a], JN[b], (double)d, (double)(d > 0 ? dx / d : 0), (double)(d > 0 ? dy / d : 0), (double)(d > 0 ? dz / d : 0),
                    (double)wp[a].x, (double)wp[a].y, (double)wp[a].z, (double)wp[b].x, (double)wp[b].y, (double)wp[b].z);
            }
            // Referencia: distancia pescoco->quadril (escala do corpo na cena).
            if (okp[MJ::SK_NECK] && okp[MJ::SK_HL]) {
                float dx = wp[MJ::SK_HL].x - wp[MJ::SK_NECK].x;
                float dy = wp[MJ::SK_HL].y - wp[MJ::SK_NECK].y;
                float dz = wp[MJ::SK_HL].z - wp[MJ::SK_NECK].z;
                Log::Infof("[JOINT] NECK->HL (tronco) len=%.2fm", (double)sqrtf(dx * dx + dy * dy + dz * dz));
            }
            s_jointLogged = true;
            Log::Info("[JOINT] auditoria juntas concluida.");
        }
    }

    // Monta snapshot do ESP (zumbis). Roda na worker 30Hz, nao por frame.
    static void BuildEsp() {
        EspEntry tmp[128] = {};
        int n = 0;
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
        // FIX P0-2 (crash em transicao de cena 15/09): valida o wrapper da camera
        // antes de qualquer invoke. Se a cena trocou (morte/troca de mapa), o
        // MainCamera.instance pode apontar p/ objeto destruido. Probe de 1 byte
        // com SEH: wrapper morto = AV capturado aqui, fora do JIT do Mono.
        { volatile char probe = 0;
          __try { memcpy((void*)&probe, cam, 1); }
          __except (EXCEPTION_EXECUTE_HANDLER) { return; } }
        // VP proprio 1x por ciclo (2 invokes): todas as projecoes do ciclo usam a mesma matriz.
        s_vpOk = false;
        {
            float V[16], P[16];
            if (GetMat(mGetViewMat, cam, V) && GetMat(mGetProjMat, cam, P)) {
                MulVP(P, V, s_vp);
                s_vpOk = true;
            }
        }
        // Posicao da camera 1x por ciclo p/ cull por distancia (poupa 2 invokes de W2S nos longe).
        // s_camW global: CollectJoints usa p/ gate de skeleton longe (Causa A).
        Vec3 camW = { 0, 0, 0 };
        bool hasCamW = false;
        if (mGetTrans) {
            long long t0c = PiNow();
            MonoObject* exc = nullptr;
            MonoObject* tr = nullptr;
            __try { tr = pInvoke(mGetTrans, cam, nullptr, &exc); } __except (EXCEPTION_EXECUTE_HANDLER) { tr = nullptr; exc = (MonoObject*)1; }
            if (tr && !exc) hasCamW = GetPos(tr, camW);
            PiAdd(s_piTrC, PiNow() - t0c, hasCamW);
        }
        s_camW = camW; s_camWok = hasCamW;
        float maxD = Config::fMaxDistance;
        float maxD2 = maxD * maxD;
        LARGE_INTEGER t0, t1;
        QueryPerformanceCounter(&t0);
        // Orçamento do ciclo: reseta a cada BuildEsp. Sem orçamento, LOS e
        // skeleton viram leitura barata (sem invoke) em vez de travar o jogo.
        s_budgetLeft = s_budgetMax;
        s_losCursor = (s_losCursor + 1) & 0x7fffffff;
        void* zl = nullptr;
        if (!cZLoader) return;
        MonoClassField* fZL = pFieldFrom(cZLoader, "Instance");
        if (!fZL || !StaticInstance(cZLoader, fZL, zl)) return;
        void* list = ReadP(zl, Off::ZL_zombies);
        WalkList(list, 512, [&](void* e, int) {
            if (n >= 128) return;
            void* h = ReadP(e, Off::Z_health);
            if (!h) return;
            float hp = ReadF(h, Off::ZH_amount);
            float mx = ReadF(h, Off::ZH_max);
            if (hp <= 0 || mx <= 0 || hp > mx) return; // so vivos
            unsigned char alive = 0; // Fix A: isAlive (morte dura ~1s com HP>0 = fantasma)
            __try { memcpy(&alive, (char*)h + Off::ZH_alive, 1); } __except (EXCEPTION_EXECUTE_HANDLER) {}
            if (!alive) { s_ghostDead++; return; }
            // Kill-window (item 14b): HP/isAlive lidos no topo do ciclo, mas o
            // zumbi pode morrer ENTRE a leitura e o invoke (soco = 1 frame).
            // Revalida imediatamente antes de qualquer invoke na entidade:
            // mudou de estado = skip, sem tocar no objeto (wrapper pode estar
            // em Destroy). Custo: 2 memcpy com SEH, zero invoke.
            float hp2 = 0; unsigned char alive2 = 0;
            __try {
                memcpy(&hp2, (char*)h + Off::ZH_amount, sizeof(hp2));
                memcpy(&alive2, (char*)h + Off::ZH_alive, 1);
            } __except (EXCEPTION_EXECUTE_HANDLER) { return; }
            if (!alive2 || hp2 <= 0 || hp2 != hp) { s_ghostDead++; return; }
            void* zo = ReadP(e, Off::Z_obj);
            if (!zo) return;
            // Anti-horda: teto de 96 entidades por ciclo. O resto fica p/ o
            // proximo ciclo (o snapshot segura as cores). Sem isso, horda de
            // 200+ entidades x ~8 invokes = corrida com o LOD (crash 15/09).
            if (n >= 96) return;
            EspEntry tmpEn = {}; // FIX P0 soco: sem invoke de nome (wrapper pode estar morto)
            strncpy_s(tmpEn.name, sizeof(tmpEn.name), "Zombie", _TRUNCATE);
            for (int k = 0; k < 8; ++k) { tmpEn.pv[k] = false; tmpEn.px[k] = tmpEn.py[k] = 0; }
            tmpEn.has3d = false;
            // Box 3D real: AABB de mundo do corpo (sem hardcode de tamanho).
            if (Config::iZombieBox == 1 && mGetBounds) {
                void* mesh = ReadP(zo, Off::ZO_mesh);
                // NOTA: sem filtro Renderer.enabled aqui - o LOD desliga renderers
                // longe e filtrar sumiria com zumbis distantes. Spawn sem mesh cai
                // no bounds invalido (extents 0/NaN rejeitado no GetBounds).
                Bnd bb;
                if (mesh && GetBounds(mesh, bb)) {
                    if (!Fin(bb.center.x) || !Fin(bb.center.y) || !Fin(bb.center.z)) { s_ghostBad++; return; } // Fix C
                    float gl = bb.center.x * bb.center.x + bb.center.y * bb.center.y + bb.center.z * bb.center.z;
                    if (gl > 10000.0f * 10000.0f) { s_ghostBad++; return; }
                    if (fabsf(bb.center.x) < 0.5f && fabsf(bb.center.y) < 0.5f && fabsf(bb.center.z) < 0.5f) { s_ghostBad++; return; } // spawn em (0,0,0)
                    bb.extents.x *= 0.7f; bb.extents.z *= 0.7f; // P2 opcao 3: AABB pega bracos/arma nas laterais
                    float dist = 0;
                    if (hasCamW) {
                        float dx = bb.center.x - camW.x, dy = bb.center.y - camW.y, dz = bb.center.z - camW.z;
                        dist = sqrtf(dx * dx + dy * dy + dz * dz);
                        if (dist * dist > maxD2) return;
                    }
                    for (int k = 0; k < 8; ++k) {
                        Vec3 w = { bb.center.x + ((k & 1) ? bb.extents.x : -bb.extents.x),
                                   bb.center.y + ((k & 2) ? bb.extents.y : -bb.extents.y),
                                   bb.center.z + ((k & 4) ? bb.extents.z : -bb.extents.z) };
                        Vec3 s3;
                        if (W2S(cam, w, s3)) {
                            if (!Sane2(s3.x, s3.y)) { // P1: coordenada explodida (w~0) nao entra
                                if (s_glitchLogged < 5) { s_glitchLogged++; Log::Infof("[ESP-GLITCH] ent=0x%p corner=%d scr=(%.0f,%.0f)", e, k, (double)s3.x, (double)s3.y); }
                                continue;
                            }
                            tmpEn.px[k] = s3.x; tmpEn.py[k] = s3.y; tmpEn.pv[k] = true;
                        }
                    }
                    int nv = 0; // Fix B: cantos atras da camera nao desenham (sem fragmentos)
                    for (int k = 0; k < 8; ++k) if (tmpEn.pv[k]) nv++;
                    if (nv < 6) return;
                    // Maos ja calculadas em CollectJoints (fix bugs 1-2, 4) — vale p/ 2D e 3D.
                    // s_skDist2 alimenta o gate de skeleton longe (sem invoke extra).
                    s_skDist2 = hasCamW ? (dist * dist) : -1.0f;
                    CollectJoints(zo, cam, tmpEn);
                    // Item 14/Passo 5: depth buffer nos 5 pontos (cabeca/peito/quadril/coxas).
                    // Raycast (LosMulti) = fallback se depth indisponivel.
                    // Auditoria: DEPTH-ROW 1x no caminho 3D + UV vs viewport real.
                    {
                        Vec3 dpts[5];
                        dpts[0] = { bb.center.x, bb.center.y + bb.extents.y, bb.center.z };
                        dpts[1] = bb.center;
                        dpts[2] = { bb.center.x, bb.center.y - bb.extents.y * 0.35f, bb.center.z };
                        dpts[3] = { bb.center.x - bb.extents.x * 0.5f, bb.center.y - bb.extents.y * 0.7f, bb.center.z };
                        dpts[4] = { bb.center.x + bb.extents.x * 0.5f, bb.center.y - bb.extents.y * 0.7f, bb.center.z };
                        DepthDiagRow(dpts, dist);
                        // UV fora de [0,1] = BoneNdc divergindo da viewport do jogo.
                        for (int di = 0; di < 5; ++di) {
                            float nn = 0, uu = 0, vv = 0;
                            if (BoneNdc(dpts[di], nn, uu, vv) && (uu < 0 || uu > 1 || vv < 0 || vv > 1))
                                Log::Infof("[DEPTH-UV] pt=%d u=%.3f v=%.3f fora [0,1] (viewport divergente).", di, (double)uu, (double)vv);
                        }
                    }
                    tmpEn.losVis = true;
                    tmpEn.losHits = 5;
                    // DIAG 2.1A (temporario): qual gate falha? 1x por entidade seria ideal,
                    // mas aqui vale 1x por sessao (sem spam): mostra bVisibleCheck/hasCamW/s_losOk.
                    {
                        static bool s_gateLogged = false;
                        if (!s_gateLogged) {
                            s_gateLogged = true;
                            Log::Infof("[LOS-GATE] 3D bVisibleCheck=%d hasCamW=%d s_losOk=%d mLinecast=0x%p s_rayArgs=%d",
                                Config::bVisibleCheck ? 1 : 0, hasCamW ? 1 : 0, s_losOk ? 1 : 0,
                                (void*)mLinecast, s_rayArgs);
                        }
                    }
                    // Camada 1 (broadphase): distancia invalida = sem LOS (entidade
                    // descartada; snapshot segura a cor anterior no render).
                    if (Config::bVisibleCheck && hasCamW && dist > 0.5f && dist < 10000.0f) {
                        Vec3 pts[5];
                        pts[0] = { bb.center.x, bb.center.y + bb.extents.y, bb.center.z }; // cabeca
                        pts[1] = bb.center;                                                 // peito
                        pts[2] = { bb.center.x, bb.center.y - bb.extents.y * 0.35f, bb.center.z }; // quadril
                        pts[3] = { bb.center.x - bb.extents.x * 0.5f, bb.center.y - bb.extents.y * 0.7f, bb.center.z }; // coxaL
                        pts[4] = { bb.center.x + bb.extents.x * 0.5f, bb.center.y - bb.extents.y * 0.7f, bb.center.z }; // coxaR
                        // Camada 2+3 (briefing §6): multi-point tri-estado + decay.
                        // h=expostos, o=ocluidos, u=sem dado. Regra:
                        // - tem ocluido (o>0) e nenhum exposto -> vermelho;
                        // - tem exposto (h>=2 ou cabeca) -> verde;
                        // - so sem-dado (u==5) -> mantem cor anterior (skip).
                        // Sem dado NUNCA vira verde: era o pisca-pisca/churn.
                        {
                            int h = 0, o = 0, u = 0; bool he = false;
                            float dummy;
                            for (int pi = 0; pi < 5; ++pi) {
                                int v = -1;
                                if (s_losOk) v = LosPointV(camW, pts[pi], &dummy);
                                else if (mLinecast && s_lineArgs >= 2) v = LosPointLineV(camW, pts[pi]);
                                else {
                                    float ndc = 0, uu = 0, vv = 0;
                                    bool hasNdc = BoneNdc(pts[pi], ndc, uu, vv);
                                    float scene = 0;
                                    bool hasDepth = hasNdc && DepthVisShim::Sample(uu, vv, scene);
                                    if (!hasDepth) v = -1;
                                    else if (scene >= 0.999f) v = 1;
                                    else {
                                        float eps = 0.5f / (dist * dist + 1.0f) + 0.001f;
                                        v = (ndc <= scene + eps) ? 1 : 0;
                                    }
                                }
                                if (v > 0) { h++; if (pi == 0) he = true; }
                                else if (v == 0) o++;
                                else u++;
                            }
                            tmpEn.losHits = (u == 5) ? -1 : h;
                            if (u == 5) {
                                tmpEn.losVis = LosStable(e, true, true); // skip: mantem cor
                            } else {
                                bool rawVis = (h >= 2) || he;
                                // Ocluido sem exposto: forca vermelho estavel (bypass
                                // da histerese p/ dentro de casa nao ficar verde).
                                if (o > 0 && h == 0 && !he) {
                                    for (int i = 0; i < 256; ++i) {
                                        if (s_hystEnt[i] == e || !s_hystEnt[i]) {
                                            s_hystEnt[i] = e;
                                            s_hystStreak[i] = -5;
                                            s_hystShown[i] = false;
                                            break;
                                        }
                                    }
                                    tmpEn.losVis = false;
                                    rawVis = false;
                                } else tmpEn.losVis = LosStable(e, rawVis);
                                // DIAG 2.1C (temporario): resultado sempre (1x/sessao).
                                {
                                    static bool s_resLogged = false;
                                    if (!s_resLogged) {
                                        s_resLogged = true;
                                        Log::Infof("[LOS-RESULT] 3D ent=0x%p losVis=%d losHits=%d",
                                            e, tmpEn.losVis ? 1 : 0, h);
                                    }
                                }
                                if (s_losLogN < 40) {
                                    s_losLogN++;
                                    const char* via = s_losOk ? "ray" : (mLinecast ? "line" : "depth");
                                    Log::Infof("[LOS] id=0x%p dist=%.1f hits=%d/5 visivel=%d(raw=%d) via=%s",
                                        e, (double)dist, h, tmpEn.losVis ? 1 : 0, rawVis ? 1 : 0, via);
                                }
                            }
                        }
                    }
                    if (!s_skelLogged && tmpEn.skN == SkJoint::SK_COUNT) {
                        s_skelLogged = true;
                        unsigned m = 0;
                        for (int k = 0; k < SkJoint::SK_COUNT; ++k) if (tmpEn.skV[k]) m |= (1u << k);
                        Log::Infof("[SKEL] mask=0x%05X dist=%.1f (3D)", m, (double)dist);
                        static const char* JN[SkJoint::SK_COUNT] = { "HEAD","NECK","SP3","SP2","SP1","HL","L1L","L2L","FL","L1R","L2R","FR","SL","A1L","A2L","SR","A1R","A2R","HL2L","HL2R" };
                        for (int k = 0; k < SkJoint::SK_COUNT; ++k)
                            Log::Infof("[SKEL] %s v=%d scr=(%.0f,%.0f)", JN[k], tmpEn.skV[k] ? 1 : 0, (double)tmpEn.skX[k], (double)tmpEn.skY[k]);
                    }
                    if (!s_handLogged2 && (tmpEn.skV[SkJoint::SK_HL2L] || tmpEn.skV[SkJoint::SK_HL2R])) {
                        s_handLogged2 = true;
                        Log::Infof("[HAND2] maos vivas L=%d R=%d (Bug 1 corrigido)",
                            tmpEn.skV[SkJoint::SK_HL2L] ? 1 : 0, tmpEn.skV[SkJoint::SK_HL2R] ? 1 : 0);
                    }
            EspEntry& en = tmp[n++];
            memcpy(en.name, tmpEn.name, sizeof(en.name));
            en.has3d = false;
                    en.dist = dist;
                    memcpy(en.px, tmpEn.px, sizeof(en.px));
                    memcpy(en.py, tmpEn.py, sizeof(en.py));
                    memcpy(en.pv, tmpEn.pv, sizeof(en.pv));
                    en.skN = tmpEn.skN;
                    memcpy(en.skX, tmpEn.skX, sizeof(en.skX));
                    memcpy(en.skY, tmpEn.skY, sizeof(en.skY));
                    memcpy(en.skV, tmpEn.skV, sizeof(en.skV));
                    en.has3d = true;
                    en.losVis = tmpEn.losVis;
                    en.losHits = tmpEn.losHits;
                    en.ent = e; en.ex = bb.extents.x; en.ey = bb.extents.y; en.ez = bb.extents.z;
                    en.headX = en.headY = en.footX = en.footY = 0;
                    en.hp = hp; en.maxHp = mx;
                    en.onScreen = true; en.isAlly = false;
                    return;
                }
                // bounds falhou: cai no caminho eye/foot abaixo (fallback documentado no log 1x).
                static bool s_bndWarned = false;
                if (!s_bndWarned) { s_bndWarned = true; Log::Warn("get_bounds falhou; Box 3D usando fallback eye/foot."); }
            }
            void* eye = ReadP(zo, Off::ZO_eye);
            void* foot = ReadP(zo, Off::ZO_foot);
            if (!eye || !foot) return;
            Vec3 wh, wf, sh, sf;
            if (!GetPos(eye, wh) || !GetPos(foot, wf)) return;
            if (!Fin(wh.x) || !Fin(wh.y) || !Fin(wh.z) || !Fin(wf.x) || !Fin(wf.y) || !Fin(wf.z)) { s_ghostBad++; return; }
            float dist = 0;
            if (hasCamW) {
                float dx = wh.x - camW.x, dy = wh.y - camW.y, dz = wh.z - camW.z;
                dist = sqrtf(dx * dx + dy * dy + dz * dz);
                if (dist * dist > maxD2) return;
            }
            wh.y += 0.45f; // cabeca cubo grande: margem maior (print 02:30)
            wf.y -= 0.35f; // footRef alto: margem generosa ate calibrar pelo print (v0.7.1)
            if (!W2S(cam, wh, sh) || !W2S(cam, wf, sf)) return;
                    // Cull de tela: fora da viewport nao entra (anti-horda: menos
                    // invokes em zumbi que nem aparece; o snapshot segura o resto).
                    {
                        float vw = s_vpW > 64 ? s_vpW : 1280.0f;
                        float vh = s_vpH > 64 ? s_vpH : 768.0f;
                        bool inScr = (sh.x > -80 && sh.x < vw + 80 && sf.x > -80 && sf.x < vw + 80 &&
                                      sh.y > -80 && sh.y < vh + 80 && sf.y > -80 && sf.y < vh + 80);
                        if (!inScr) return;
                    }
                    if (!Sane2(sh.x, sh.y) || !Sane2(sf.x, sf.y)) { // P1: 2D aborta inteiro
                        if (s_glitchLogged < 5) { s_glitchLogged++; Log::Infof("[ESP-GLITCH] ent=0x%p head=(%.0f,%.0f) foot=(%.0f,%.0f)", e, (double)sh.x, (double)sh.y, (double)sf.x, (double)sf.y); }
                        return;
                    }
            if (n == 0) { s_dbgEyeY = wh.y; s_dbgFootY = wf.y; }
            s_skDist2 = hasCamW ? (dist * dist) : -1.0f;
            CollectJoints(zo, cam, tmpEn);
            // Item 14/Passo 5 (fallback 2D): mesma logica do 3D —
            // Raycast primario, Linecast secundario, Depth ultimo recurso.
            tmpEn.losVis = true;
            tmpEn.losHits = 5;
            // DIAG 2.1A-2D (temporario): gate do caminho 2D, 1x/sessao.
            {
                static bool s_gateLogged2 = false;
                if (!s_gateLogged2) {
                    s_gateLogged2 = true;
                    Log::Infof("[LOS-GATE] 2D bVisibleCheck=%d hasCamW=%d s_losOk=%d",
                        Config::bVisibleCheck ? 1 : 0, hasCamW ? 1 : 0, s_losOk ? 1 : 0);
                }
            }
            // Broadphase 2D: sem distancia valida nao ha LOS (igual ao 3D).
            if (Config::bVisibleCheck && hasCamW && dist > 0.5f && dist < 10000.0f) {
                Vec3 pts[5];
                pts[0] = wh; // cabeca (olho)
                pts[1] = { (wh.x + wf.x) * 0.5f, wh.y + (wf.y - wh.y) * 0.25f, (wh.z + wf.z) * 0.5f }; // peito
                pts[2] = { (wh.x + wf.x) * 0.5f, wh.y + (wf.y - wh.y) * 0.5f, (wh.z + wf.z) * 0.5f };  // quadril
                pts[3] = { (wh.x + wf.x) * 0.5f, wh.y + (wf.y - wh.y) * 0.75f, (wh.z + wf.z) * 0.5f }; // coxa
                pts[4] = wf; // pes
                // Tri-estado igual ao 3D (h=exposto, o=ocluido, u=sem dado).
                int h = 0, o = 0, u = 0; bool he = false;
                float dummy;
                for (int pi = 0; pi < 5; ++pi) {
                    int v = -1;
                    if (s_losOk) v = LosPointV(camW, pts[pi], &dummy);
                    else if (mLinecast && s_lineArgs >= 2) v = LosPointLineV(camW, pts[pi]);
                    else v = DepthExposed(pts[pi], dist) ? 1 : 0;
                    if (v > 0) { h++; if (pi == 0) he = true; }
                    else if (v == 0) o++;
                    else u++;
                    float ndc = 0, uu = 0, vv = 0;
                    tmpEn.losDepth[pi] = BoneNdc(pts[pi], ndc, uu, vv) ? ndc : -1.0f;
                }
                DepthDiagRow(pts, dist);
                tmpEn.losHits = (u == 5) ? -1 : h;
                if (u == 5) tmpEn.losVis = LosStable(e, true, true); // skip: mantem cor
                else if (o > 0 && h == 0 && !he) {
                    for (int i = 0; i < 256; ++i) {
                        if (s_hystEnt[i] == e || !s_hystEnt[i]) {
                            s_hystEnt[i] = e; s_hystStreak[i] = -5; s_hystShown[i] = false; break;
                        }
                    }
                    tmpEn.losVis = false;
                }
                else tmpEn.losVis = LosStable(e, (h >= 2) || he);
                // DIAG 2.1C-2D (temporario): resultado do 2D, 1x/sessao.
                {
                    static bool s_resLogged2 = false;
                    if (!s_resLogged2) {
                        s_resLogged2 = true;
                        Log::Infof("[LOS-RESULT] 2D ent=0x%p losVis=%d losHits=%d", e, tmpEn.losVis ? 1 : 0, h);
                    }
                }
            }
            EspEntry& en = tmp[n++];
            memcpy(en.name, tmpEn.name, sizeof(en.name));
            en.dist = dist;
            en.losVis = tmpEn.losVis;
            en.losHits = tmpEn.losHits;
            en.headX = sh.x; en.headY = sh.y;
            en.footX = sf.x; en.footY = sf.y;
            // Diagnostico SKEL (1x/sessao, 1a entidade): mascara de juntas + tela.
            if (!s_skelLogged && tmpEn.skN == SkJoint::SK_COUNT) {
                s_skelLogged = true;
                unsigned m = 0;
                for (int k = 0; k < SkJoint::SK_COUNT; ++k) if (tmpEn.skV[k]) m |= (1u << k);
                Log::Infof("[SKEL] mask=0x%05X dist=%.1f", m, (double)dist);
                static const char* JN[SkJoint::SK_COUNT] = { "HEAD","NECK","SP3","SP2","SP1","HL","L1L","L2L","FL","L1R","L2R","FR","SL","A1L","A2L","SR","A1R","A2R","HL2L","HL2R" };
                for (int k = 0; k < SkJoint::SK_COUNT; ++k)
                    Log::Infof("[SKEL] %s v=%d scr=(%.0f,%.0f)", JN[k], tmpEn.skV[k] ? 1 : 0, (double)tmpEn.skX[k], (double)tmpEn.skY[k]);
            }
            en.skN = tmpEn.skN;
            memcpy(en.skX, tmpEn.skX, sizeof(en.skX));
            memcpy(en.skY, tmpEn.skY, sizeof(en.skY));
            memcpy(en.skV, tmpEn.skV, sizeof(en.skV));
            en.losVis = tmpEn.losVis;
            en.ent = e; en.ex = en.ey = en.ez = 0;
            en.hp = hp; en.maxHp = mx;
            en.onScreen = true; en.isAlly = false;
        });
        // Publicacao double-buffer (item 14b): worker escreve no back, vira o
        // ponteiro sob 1 CS curto. Present le o front SEM lock (ponteiro).
        // TryEnter aqui so protege a virada; quem perde a virada tenta no
        // proximo ciclo — nunca trava, nunca perde dado.
        EnterCriticalSection(&s_espCS);
        // Telemetria de orcamento (1x/sessao): prova que o teto segura a horda.
        if (!s_budgetLogged) {
            s_budgetLogged = true;
            char bb[32] = { 0 };
            BudgetFmt(bb, sizeof(bb));
            Log::Infof("[BUDGET] ciclo invocacoes=%s entidades=%d.", bb, n);
        }
        {
            int logged = 0;
            for (int i = 0; i < n && logged < 6; ++i) {
                bool known = false;
                for (int j = 0; j < s_lastN; ++j) if (s_lastEnts[j] == tmp[i].ent) { known = true; break; }
                if (!known) {
                    Log::Infof("[ESP+] ent=0x%p nome=%s hp=%.0f/%.0f dist=%.1f ext=(%.2f,%.2f,%.2f)",
                        tmp[i].ent, tmp[i].name, (double)tmp[i].hp, (double)tmp[i].maxHp,
                        (double)tmp[i].dist, (double)tmp[i].ex, (double)tmp[i].ey, (double)tmp[i].ez);
                    logged++;
                }
            }
            for (int j = 0; j < s_lastN && logged < 8; ++j) {
                bool gone = true;
                for (int i = 0; i < n; ++i) if (tmp[i].ent == s_lastEnts[j]) { gone = false; break; }
                if (gone) { Log::Infof("[ESP-] ent=0x%p", s_lastEnts[j]); logged++; }
            }
            s_lastN = n > 128 ? 128 : n;
            for (int i = 0; i < s_lastN; ++i) s_lastEnts[i] = tmp[i].ent;
            s_espNBack = s_lastN;
            s.espShown = s_espNBack;
            for (int i = 0; i < s_espNBack; ++i) s_espBack[i] = tmp[i];
            // Vira o ponteiro: front novo = back cheio (troca atomica de ptr).
            EspEntry* t = s_espFront; s_espFront = s_espBack; s_espBack = t;
            s_espNFront = s_espNBack;
            s_espN = s_espNFront; // legado: espelho p/ debug
        }
        LeaveCriticalSection(&s_espCS);
        // Métrica de custo do ciclo (fora do lock).
        QueryPerformanceCounter(&t1);
        {
            LARGE_INTEGER fr;
            QueryPerformanceFrequency(&fr);
            float ms = (float)(t1.QuadPart - t0.QuadPart) * 1000.0f / (float)fr.QuadPart;
            s.espMs = s.espMs * 0.9f + ms * 0.1f;
        }
    }

    // Watchdog anti-hang no respawn: se a cena trocou (morte do player) o
    // Mono pode estar recarregando o domain enquanto a worker invoca. Sinais:
    // imagem C# sumiu, ZombieLoader morto, ou camera sem posicao. Nesses casos
    // a worker dorme 500ms e tenta de novo — nunca invoca no escuro.
    static bool SceneAlive() {
        __try {
            if (!s_img || !s_dom) return false;
            if (!cZLoader || !fZLInst) return true; // init ainda nao rodou: deixa passar
            MonoVTable* vt = pVTable(s_dom, cZLoader);
            if (!vt) return false;
            void* zl = nullptr;
            pStaticGet(vt, fZLInst, &zl);
            if (!zl) return false;
            volatile char probe = 0;
            memcpy((void*)&probe, zl, 1);
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
    // Ritmo adaptativo anti-crash: a worker mede o custo do BuildEsp e ajusta
    // o intervalo. Ciclo caro (>25ms, horda densa) = respira 66ms; ciclo leve
    // (<12ms) = volta pros 33ms. O jogo nunca recebe rajada de invokes: quanto
    // mais pesado o ciclo, mais folga o PhysX/LOD ganha antes do proximo.
    static int s_sleepMs = 33;
    static DWORD WINAPI EspThread(LPVOID) {
        pAttach(s_dom); // worker precisa do proprio attach no Mono
        Log::Info("Thread ESP iniciada (30Hz, fora do Present).");
        static int s_deadN = 0;
        while (s_espRun) {
            if (s.ready && Config::bZombieEsp) {
                if (!SceneAlive()) {
                    // Cena morta/trocando (respawn): zera o snapshot e espera.
                    // TryEnter: se o Present estiver lendo, pula em vez de travar.
                    if (TryEnterCriticalSection(&s_espCS)) { s_espN = 0; s_lastN = 0; LeaveCriticalSection(&s_espCS); }
                    if (++s_deadN == 1) Log::Warn("[SCENE] loader morto — worker em espera (respawn?).");
                    Sleep(500);
                    continue;
                }
                if (s_deadN > 0) { s_deadN = 0; Log::Info("[SCENE] loader vivo — worker retomada."); }
                BuildEsp();
                PiFlush(false); // agregado [PI-CALL] 1x/5s (so sai se fail>0 ou >500us)
                // Adapta pelo custo medido no ciclo (s.espMs, media movel).
                int want = (s.espMs > 25.0f) ? 66 : (s.espMs < 12.0f ? 33 : 50);
                if (want != s_sleepMs) {
                    s_sleepMs = want;
                    Log::Infof("[PERF] ciclo %.1fms -> intervalo %dms.", (double)s.espMs, want);
                }
            }
            else { EnterCriticalSection(&s_espCS); s_espN = 0; LeaveCriticalSection(&s_espCS); }
            Sleep(s_sleepMs);
        }
        return 0;
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

    // Auditoria ossos (item 12): registra 1x o nome de cada Transform do
    // armatureBone do primeiro zumbi vivo. Com os nomes, mapeio cabeca, bracos,
    // pernas e fio as juntas certas (padrao dos grandes: bone IDs, nao chute).
    static void AuditBones() {
        if (s_boneLogged || !mGetGO) return;
        void* zl = nullptr;
        if (!StaticInstance(cZLoader, fZLInst, zl)) return;
        void* list = ReadP(zl, Off::ZL_zombies);
        if (!list) return;
        bool done = false;
        WalkList(list, 512, [&](void* e, int) {
            if (done) return;
            void* h = ReadP(e, Off::Z_health);
            if (!h) return;
            if (ReadF(h, Off::ZH_amount) <= 0) return;
            void* zo = ReadP(e, Off::Z_obj);
            if (!zo) return;
            void* arr = ReadP(zo, Off::ZO_armature);
            if (!arr) return;
            long long len = 0;
            __try { memcpy(&len, (char*)arr + Off::A_len, sizeof(len)); }
            __except (EXCEPTION_EXECUTE_HANDLER) { return; }
            if (len <= 0 || len > 64) return;
            Log::Infof("[BONE] armature len=%d ent=0x%p", (int)len, e);
            for (long long i = 0; i < len && i < 40; ++i) {
                void* bone = nullptr;
                __try { memcpy(&bone, (char*)arr + Off::A_data + (size_t)i * 8, 8); }
                __except (EXCEPTION_EXECUTE_HANDLER) { break; }
                if (!bone) { Log::Infof("[BONE] idx=%d (null)", (int)i); continue; }
                char nm[64] = { 0 };
                __try {
                    MonoObject* exc = nullptr;
                    MonoObject* go = pInvoke(mGetGO, bone, nullptr, &exc);
                    if (exc || !go) { Log::Infof("[BONE] idx=%d (sem gameObject)", (int)i); continue; }
                    MonoObject* exc2 = nullptr;
                    MonoObject* ret = pInvoke(mGetName, go, nullptr, &exc2);
                    if (exc2 || !ret) { Log::Infof("[BONE] idx=%d (sem nome)", (int)i); continue; }
                    char* u = pStrUtf8(ret);
                    if (u) { strncpy_s(nm, u, _TRUNCATE); pFree(u); }
                } __except (EXCEPTION_EXECUTE_HANDLER) { continue; }
                Log::Infof("[BONE] idx=%d name=%s bone=0x%p", (int)i, nm[0] ? nm : "?", bone);
            }
            done = true;
        });
        if (done) { s_boneLogged = true; Log::Info("[BONE] auditoria concluida."); }
    }

    void Tick() {
        // ESP roda em worker thread (30Hz): Present nunca bloqueia em invoke.
        // a camera gira. Leituras de texto do overlay seguem a 2Hz (30 frames).
        ++s_tick;
        if (!s.ready && !Init()) return;
        if (s_tick % 30 != 0) return;
        static int n = 0;
        ReadAll();
        AuditBones();
        if (++n == 1 || n % 20 == 0)
            Log::Infof("Mono live: localHP=%.0f stam=%.0f players=%d zombies=%d zHp0=%.0f day=%.2fh eyeY=%.2f footY=%.2f fantasma(morta=%d ruim=%d)",
                s.localHp, s.localStam, s.players, s.zombies, s.zHp0, s.dayTime, s_dbgEyeY, s_dbgFootY, s_ghostDead, s_ghostBad);
    }

    const State& Get() { return s; }
    int GetEsp(EspEntry* out, int max) {
        if (!out || max <= 0) return 0;
        // Present SEM lock (item 14b): le o front via ponteiro. A virada do
        // ponteiro e atomica no x64; o pior caso e 1 frame com o buffer
        // anterior — nunca trava, nunca memcpy sob lock no frame.
        EspEntry* f = s_espFront;
        int n = s_espNFront < max ? s_espNFront : max;
        if (n < 0) n = 0;
        if (n > 128) n = 128;
        for (int i = 0; i < n; ++i) out[i] = f[i];
        return n;
    }
    void Shutdown() {
        s_espRun = false;
        if (s_espThread) { WaitForSingleObject(s_espThread, 1000); CloseHandle(s_espThread); s_espThread = nullptr; }
        if (s_csInit) { DeleteCriticalSection(&s_espCS); s_csInit = false; }
        s = State();
        s_bound = false; s_logged = false;
        s_dom = nullptr; s_img = nullptr;
    }
}





















