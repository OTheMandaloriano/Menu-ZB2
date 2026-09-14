#include "mono.h"
#include "config.h"
#include "log.h"
#include <Windows.h>
#include <math.h>

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
    static MonoMethod* mGetGO = nullptr; // Component.get_gameObject (auditoria ossos)
    static bool s_boneLogged = false;
    static bool s_jointLogged = false; // auditoria juntas (1x por sessao)
    static DWORD WINAPI EspThread(LPVOID); // forward (definida apos BuildEsp)
    static MonoImage*  s_unity = nullptr;
    static MonoClass*  cCamU = nullptr;
    static MonoClass*  cTrans = nullptr;
    static MonoMethod* mGetPos = nullptr;
    static MonoMethod* mW2S = nullptr;
    static MonoMethod* mGetTrans = nullptr;
    static EspEntry s_esp[128];
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
            if (cComp) ResolveMethod(cComp, "Component", "get_transform", 0, mGetTrans);
            if (cComp) ResolveMethod(cComp, "Component", "get_gameObject", 0, mGetGO);
            MonoClass* cObj = pClassFrom(s_unity, "UnityEngine", "Object");
            if (cObj) { s.resolvedClasses++; ResolveMethod(cObj, "Object", "get_name", 0, mGetName); }
            MonoClass* cRend = pClassFrom(s_unity, "UnityEngine", "Renderer");
            if (cRend) { s.resolvedClasses++; ResolveMethod(cRend, "Renderer", "get_bounds", 0, mGetBounds); }
        } else Log::Warn("Imagem UnityEngine.CoreModule nao carregada.");

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

    void SetViewport(float w, float h) {
        if (w > 100 && h > 100) { s_vpW = w; s_vpH = h; }
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
        __try {
            MonoObject* exc = nullptr;
            MonoObject* ret = pInvoke(mGetBounds, rend, nullptr, &exc);
            if (exc || !ret) return false;
            memcpy(&out, pUnbox(ret), sizeof(out));
            if (!(out.extents.x > 0.05f && out.extents.x < 6.0f)) return false;
            if (!(out.extents.y > 0.05f && out.extents.y < 6.0f)) return false;
            if (!(out.extents.z > 0.05f && out.extents.z < 6.0f)) return false;
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }

    static bool Fin(float v) { return v == v && v > -3.4028235e38f && v < 3.4028235e38f; }

    // P1: o W2S do motor pode retornar x=50000 com z>0 (w~0+ por dentro do motor).
    // Unica defesa: sanidade no ESPACO DE TELA. NaN falha nas comparacoes e cai aqui.
    static bool Sane2(float x, float y) {
        return x > -10000.0f && x < 10000.0f && y > -10000.0f && y < 10000.0f;
    }
    static int s_glitchLogged = 0; // log diagnostico (Passo 7), sem spam

    // Nome via Object.get_name (GameObject). Fallback "Zumbi".
    static void GetName(void* obj, char* out, size_t cap) {
        strncpy_s(out, cap, "Zumbi", _TRUNCATE);
        if (!mGetName || !obj || !pStrUtf8 || !pFree) return;
        __try {
            MonoObject* exc = nullptr;
            MonoObject* ret = pInvoke(mGetName, obj, nullptr, &exc);
            if (exc || !ret) return;
            char* u = pStrUtf8(ret);
            if (u) {
                strncpy_s(out, cap, u, _TRUNCATE);
                pFree(u);
                // "ZombiePrefab(Clone)" -> "Zombie" (legivel no ESP)
                char* p;
                while ((p = strstr(out, "(Clone)")) != nullptr) memmove(p, p + 7, strlen(p + 7) + 1);
                while ((p = strstr(out, "Prefab")) != nullptr) memmove(p, p + 6, strlen(p + 6) + 1);
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // Item 12 Skeleton real: juntas pelos indices auditados ([BONE] 14/09).
    // Ordem SkJoint: head12 neck11 sp3-10 sp2-9 sp1-8 | perna L: hl1 l1l2 l2l3 fl4
    // perna R: l1r5 l2r6 fr7 (topo = sp1) | braco L: sl13 a1l14 a2l15 | R: sr16 a1r17 a2r18.
    static const int kBoneIdx[SkJoint::SK_COUNT] = {
        12, 11, 10, 9, 8, 1, 2, 3, 4, 5, 6, 7, 13, 14, 15, 16, 17, 18
    };
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
        for (int k = 0; k < SkJoint::SK_COUNT; ++k) {
            void* bone = nullptr;
            __try { memcpy(&bone, (char*)arr + Off::A_data + (size_t)kBoneIdx[k] * 8, 8); }
            __except (EXCEPTION_EXECUTE_HANDLER) { continue; }
            if (!bone) continue;
            Vec3 w, s3;
            if (!GetPos(bone, w)) continue;
            if (!Fin(w.x) || !Fin(w.y) || !Fin(w.z)) continue;
            if (!W2S(cam, w, s3)) continue;
            if (!Sane2(s3.x, s3.y)) continue;
            out.skX[k] = s3.x; out.skY[k] = s3.y; out.skV[k] = true;
        }
        // Auditoria juntas (1x/sessao): posicao de mundo + direcao ate o pai.
        // Resultado 14/09: braco real = ombro(12)+antebraco(0.21m); sem mao no rig.
        // Mao estimada = ponta do antebraco (padrao grandes cheats p/ rig sem falange).
        if (!s_jointLogged) {
            Vec3 wp[SkJoint::SK_COUNT];
            bool okp[SkJoint::SK_COUNT] = { false };
            for (int k = 0; k < SkJoint::SK_COUNT; ++k) {
                void* bone = nullptr;
                __try { memcpy(&bone, (char*)arr + Off::A_data + (size_t)kBoneIdx[k] * 8, 8); }
                __except (EXCEPTION_EXECUTE_HANDLER) { continue; }
                if (bone && GetPos(bone, wp[k]) && Fin(wp[k].x)) okp[k] = true;
            }
            static const char* JN[SkJoint::SK_COUNT] = { "HEAD","NECK","SP3","SP2","SP1","HL","L1L","L2L","FL","L1R","L2R","FR","SL","A1L","A2L","SR","A1R","A2R" };
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
        Vec3 camW = { 0, 0, 0 };
        bool hasCamW = false;
        if (mGetTrans) {
            MonoObject* exc = nullptr;
            MonoObject* tr = nullptr;
            __try { tr = pInvoke(mGetTrans, cam, nullptr, &exc); } __except (EXCEPTION_EXECUTE_HANDLER) { tr = nullptr; exc = (MonoObject*)1; }
            if (tr && !exc) hasCamW = GetPos(tr, camW);
        }
        float maxD = Config::fMaxDistance;
        float maxD2 = maxD * maxD;
        LARGE_INTEGER t0, t1;
        QueryPerformanceCounter(&t0);
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
            void* zo = ReadP(e, Off::Z_obj);
            if (!zo) return;
            EspEntry tmpEn = {}; // nome antes dos Transforms (barato, 1 invoke)
            GetName(zo, tmpEn.name, sizeof(tmpEn.name));
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
                    // Mao: ponta do antebraco em MUNDO (antes da projecao = escala certa).
                    // v = a2 + (a2-a1).norm * 0.22m (antebraco mede 0.21m na auditoria).
                    CollectJoints(zo, cam, tmpEn);
                    {
                        float exL[3] = { 0 }, exR[3] = { 0 };
                        bool okL = false, okR = false;
                        void* arr = ReadP(zo, Off::ZO_armature);
                        if (arr) {
                            Vec3 p1, p2;
                            void* b1 = nullptr, *b2 = nullptr;
                            __try {
                                memcpy(&b1, (char*)arr + Off::A_data + (size_t)14 * 8, 8);
                                memcpy(&b2, (char*)arr + Off::A_data + (size_t)15 * 8, 8);
                            } __except (EXCEPTION_EXECUTE_HANDLER) { b1 = b2 = nullptr; }
                            if (b1 && b2 && GetPos(b1, p1) && GetPos(b2, p2)) {
                                float dx = p2.x - p1.x, dy = p2.y - p1.y, dz = p2.z - p1.z;
                                float l = sqrtf(dx * dx + dy * dy + dz * dz);
                                if (l > 0.01f) {
                                    exL[0] = p2.x + dx / l * 0.22f; exL[1] = p2.y + dy / l * 0.22f; exL[2] = p2.z + dz / l * 0.22f;
                                    okL = true;
                                }
                            }
                            b1 = b2 = nullptr;
                            __try {
                                memcpy(&b1, (char*)arr + Off::A_data + (size_t)17 * 8, 8);
                                memcpy(&b2, (char*)arr + Off::A_data + (size_t)18 * 8, 8);
                            } __except (EXCEPTION_EXECUTE_HANDLER) { b1 = b2 = nullptr; }
                            if (b1 && b2 && GetPos(b1, p1) && GetPos(b2, p2)) {
                                float dx = p2.x - p1.x, dy = p2.y - p1.y, dz = p2.z - p1.z;
                                float l = sqrtf(dx * dx + dy * dy + dz * dz);
                                if (l > 0.01f) {
                                    exR[0] = p2.x + dx / l * 0.22f; exR[1] = p2.y + dy / l * 0.22f; exR[2] = p2.z + dz / l * 0.22f;
                                    okR = true;
                                }
                            }
                        }
                        if (okL || okR) {
                            if (tmpEn.skN < SkJoint::SK_COUNT) tmpEn.skN = SkJoint::SK_COUNT;
                            Vec3 w, s3;
                            if (okL) {
                                w.x = exL[0]; w.y = exL[1]; w.z = exL[2];
                                if (W2S(cam, w, s3) && Sane2(s3.x, s3.y)) {
                                    tmpEn.skX[SkJoint::SK_HL2L] = s3.x; tmpEn.skY[SkJoint::SK_HL2L] = s3.y; tmpEn.skV[SkJoint::SK_HL2L] = true;
                                }
                            }
                            if (okR) {
                                w.x = exR[0]; w.y = exR[1]; w.z = exR[2];
                                if (W2S(cam, w, s3) && Sane2(s3.x, s3.y)) {
                                    tmpEn.skX[SkJoint::SK_HL2R] = s3.x; tmpEn.skY[SkJoint::SK_HL2R] = s3.y; tmpEn.skV[SkJoint::SK_HL2R] = true;
                                }
                            }
                        }
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
            if (!Sane2(sh.x, sh.y) || !Sane2(sf.x, sf.y)) { // P1: 2D aborta inteiro
                if (s_glitchLogged < 5) { s_glitchLogged++; Log::Infof("[ESP-GLITCH] ent=0x%p head=(%.0f,%.0f) foot=(%.0f,%.0f)", e, (double)sh.x, (double)sh.y, (double)sf.x, (double)sf.y); }
                return;
            }
            if (n == 0) { s_dbgEyeY = wh.y; s_dbgFootY = wf.y; }
            CollectJoints(zo, cam, tmpEn);
            EspEntry& en = tmp[n++];
            memcpy(en.name, tmpEn.name, sizeof(en.name));
            en.dist = dist;
            en.headX = sh.x; en.headY = sh.y;
            en.footX = sf.x; en.footY = sf.y;
            en.skN = tmpEn.skN;
            memcpy(en.skX, tmpEn.skX, sizeof(en.skX));
            memcpy(en.skY, tmpEn.skY, sizeof(en.skY));
            memcpy(en.skV, tmpEn.skV, sizeof(en.skV));
            en.ent = e; en.ex = en.ey = en.ez = 0;
            en.hp = hp; en.maxHp = mx;
            en.onScreen = true; en.isAlly = false;
        });
        // Diagnostico P1: transicoes add/remove com identidade (causa raiz, nao supressao).
        EnterCriticalSection(&s_espCS);
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
            s_espN = s_lastN; // publica o snapshot (lock ja adquirido acima)
            s.espShown = s_espN;
            for (int i = 0; i < s_espN; ++i) s_esp[i] = tmp[i];
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

    static DWORD WINAPI EspThread(LPVOID) {
        pAttach(s_dom); // worker precisa do proprio attach no Mono
        Log::Info("Thread ESP iniciada (30Hz, fora do Present).");
        while (s_espRun) {
            if (s.ready && Config::bZombieEsp) BuildEsp();
            else { EnterCriticalSection(&s_espCS); s_espN = 0; LeaveCriticalSection(&s_espCS); }
            Sleep(33); // ~30Hz: boxes a 30fps parecem grudadas; 20Hz parecia "queda de FPS"
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
        EnterCriticalSection(&s_espCS);
        int n = s_espN < max ? s_espN : max;
        for (int i = 0; i < n; ++i) out[i] = s_esp[i];
        LeaveCriticalSection(&s_espCS);
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




















