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
    // PlayerMain (instancia) — offsets validados OFFSETS.md (CE MCP 12/09).
    // healthFast/healthSlow: float, 100.0 cheio. stamina*: 100.0 cheio.
    constexpr int PM_healthFast = 204;
    constexpr int PM_healthSlow = 208;
    constexpr int PM_maxStamina = 224;
    constexpr int PM_staminaFast = 228;
    constexpr int PM_staminaSlow = 232;
    // ZombieLoader
    constexpr int ZL_zombies = 88;
    constexpr int ZL_totalReal = 184;
    // Zombie (instancia)
    constexpr int Z_health = 168;
    // Zombie.identity+192 -> ZombieIdentity; type+20 = ZombieType enum.
    // [PENDENTE CE MCP]: validar via readInt(z+192 -> zi+20) em partida antes
    // de confiar (CLASSES_UTEIS: Zombie identity+192, ZombieIdentity type+20).
    constexpr int Z_identity = 192;
    constexpr int ZI_type = 20;
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
    // Municao infinita (cadeia via dnlib/dnSpy estatico + IL do ShootGun):
    // PlayerMain.inventory -> PlayerInventory.equippedItems ->
    // PlayerEquippedItems.GetEquipment(selectedItem) -> InventoryItem.ammo.
    // Offsets via mono_field_get_offset em runtime (nunca hardcode).
    static MonoClass* cPInv = nullptr;   // PlayerInventory
    static MonoClass* cPEq = nullptr;    // PlayerEquippedItems
    static MonoClass* cItem = nullptr;   // InventoryItem
    static MonoClass* cDbGun = nullptr;  // DatabaseGun (maxAmmo/ammoConsumption)
    static MonoClassField* fInv = nullptr;      // PlayerMain.inventory
    static MonoClassField* fEq = nullptr;       // PlayerInventory.equippedItems
    static MonoClassField* fSel = nullptr;      // PlayerArms.selectedItem
    static MonoClassField* fAmmo = nullptr;     // InventoryItem.ammo
    static MonoClassField* fMaxAmmo = nullptr;  // DatabaseGun.maxAmmo
    // Itens/pilhas (regra universal do IL: stackMax==1 -> ammo; senao stackCount):
    static MonoClassField* fStack = nullptr;    // InventoryItem.stackCount
    static MonoClassField* fDbStack = nullptr;  // DatabaseItem.stackMax
    static MonoClassField* fStorage = nullptr;  // PlayerInventory.storage
    static MonoClassField* fItems = nullptr;    // ItemContainer.items
    static MonoClassField* fId = nullptr;       // InventoryItem.id
    static MonoMethod* mGetEq = nullptr;   // PlayerEquippedItems.GetEquipment(EquipmentIndex)
    static MonoMethod* mGetDb = nullptr;   // InventoryItem.GetDataBaseItem()
    // Dinheiro (Currency singleton: Dollar/Silver/Gold -> CurrencyData.amount).
    static MonoClass* cCur = nullptr;        // Currency
    static MonoClassField* fCurInst = nullptr; // Currency.Instance
    static MonoClassField* fDollar = nullptr;  // Currency.<Dollar>
    static MonoClassField* fAmount = nullptr;  // CurrencyData.amount
    static MonoMethod* mAddCur = nullptr;    // Currency.AddCurrency(CurrencyID,int)
    static MonoClass* cCurId = nullptr;      // CurrencyID (enum: Dollars=0?)
    static bool s_moneyLogged = false;
    static bool s_moneyOk = true;
    static bool s_moneyGiven = false; // AddCurrency 1x por sessao
    static bool s_ammoLogged = false; // diagnostico da cadeia 1x/sessao

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
    static PiSite s_piPos, s_piRot, s_piBnd, s_piTrC, s_piHas;
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
        const struct { const char* nm; PiSite* s; } sites[5] = {
            { "getPos", &s_piPos }, { "getRot", &s_piRot }, { "getBounds", &s_piBnd },
            { "getTransCam", &s_piTrC }, { "hasLocal", &s_piHas },
        };
        for (int i = 0; i < 5; ++i) {
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
    // Cada invoke cruza para o Mono e compete com o jogo; com 50+ zumbis,
    // centenas de invokes por ciclo de 33ms viram corrida com o LOD
    // (ver crash 15/09 15:51). Estoura o teto? O resto do ciclo usa o
    // último valor conhecido (fail-open, sem flicker).
    static int  s_budgetLeft = 0;
    static int  s_budgetMax = 64; // 64 invokes/ciclo p/ maos do skeleton.
    static bool s_budgetLogged = false;
    static inline bool BudgetTake(int n = 1) {
        if (s_budgetLeft < n) return false;
        s_budgetLeft -= n;
        return true;
    }
    static MonoMethod* mGetGO = nullptr; // Component.get_gameObject (auditoria ossos)
    static MonoMethod* mGetRot = nullptr; // Transform.get_rotation -> quaternio (maos)
    static bool s_boneLogged = false;
    static bool s_jointLogged = false; // auditoria juntas (1x por sessao)
    static bool s_skelLogged = false; // diagnostico SKEL (1x: mascara + tela dos bracos)
    static bool s_handLogged = false; // diagnostico HAND (1x: ponta da mao em mundo)
    static bool s_handLogged2 = false; // diagnostico HAND2 (1x: maos vivas pos-fix)
    static DWORD WINAPI EspThread(LPVOID); // forward (definida apos BuildEsp)
    static void ApplyMoney(); // forward (dinheiro infinito, worker)
    static void ApplyDefense(void* local); // forward (defesa rapida, worker)
    static void TopAmmo(void* item, int oAmmo, int oMax); // forward (teto do ammo)
    static void TopStacks(void* local, int oInv, int oStorage, int oItems,
        int oStack, int oDbStack, int oId, bool items); // forward (reserva+pilhas)
    static void ApplyAmmo(void* local); // forward (municao infinita, worker)
    static int FieldOff(MonoClassField* f); // forward (offset via API, -1 se falhar)
    static void ReadAll(); // forward (chamada na worker, fora do Present)
    static void AuditBones(); // forward (chamada na worker, fora do Present)
    static MonoImage*  s_unity = nullptr;
    static MonoClass*  cCamU = nullptr;
    static MonoClass*  cTrans = nullptr;
    static MonoMethod* mGetPos = nullptr;
    static MonoMethod* mW2S = nullptr;
    static MonoMethod* mGetTrans = nullptr;
    static Vec3 s_camW = { 0, 0, 0 }; // posicao da camera do ciclo (gate skeleton)
    static bool s_camWok = false;
    static float s_skDist2 = -1.0f; // dist2 da entidade atual (gate skeleton, sem invoke)
    // Snapshot double-buffer sem lock no frame (item 14b, Rodada 1 inocentou
    // a worker): Present NUNCA toca em CS — le o ponteiro do buffer pronto
    // (troca atomica). Worker publica no back e vira o ponteiro sob 1 CS curto.
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

    // Escrita direta com SEH (espelho do ReadF). Regras:
    // - so escreve no LOCAL player (HasLocalControl==1), nunca em aliado/zumbi;
    // - so quando a flag do menu esta ligada (custo zero desligado);
    // - valida faixa antes (HP 0..1000, stamina 0..1000) p/ nao plantar NaN/lixo;
    // - roda na worker (nunca no Present), junto do ReadAll (1x/2s).
    static bool WriteF(void* base, int off, float v) {
        if (!base) return false;
        if (!(v >= -1000000.0f && v <= 1000000.0f)) return false; // rejeita NaN/inf
        __try { memcpy((char*)base + off, &v, sizeof(v)); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
        return true;
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

    // Offset de campo via API do Mono (layout decidido em runtime; nunca
    // hardcode). Retorna bytes do inicio do objeto, ou -1 se indisponivel.
    static int FieldOff(MonoClassField* f) {
        if (!f || !pFieldGetOff) return -1;
        int off = -1;
        __try { off = pFieldGetOff(f); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
        return (off >= 0 && off < 4096) ? off : -1;
    }

    // Escrita de int com SEH (espelho do WriteF), p/ InventoryItem.ammo.
    static bool WriteI(void* base, int off, int v) {
        if (!base || off < 0) return false;
        __try { memcpy((char*)base + off, &v, sizeof(v)); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
        return true;
    }

    // Invoke generico que retorna objeto (GetEquipment/GetDataBaseItem).
    // Sem telemetria Pi (fora do caminho do ESP); SEH total.
    static void* InvokeObj(MonoMethod* m, void* obj, void** args) {
        if (!m || !obj) return nullptr;
        void* out = nullptr;
        __try {
            MonoObject* exc = nullptr;
            MonoObject* ret = pInvoke(m, obj, args, &exc);
            if (!exc && ret) out = ret;
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
        return out;
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
        // Municao: classes/campos/metodos (falha = nao-fatal; ApplyAmmo desliga sozinho).
        // selectedItem fica no PlayerArms (nao no PlayerMain): resolve via cPlayer? nao —
        // via classe PlayerArms separada. Como arms eh field de PlayerMain, resolve a classe
        // PlayerArms direto pelo nome.
        ResolveClass("PlayerInventory", cPInv);
        ResolveClass("PlayerEquippedItems", cPEq);
        ResolveClass("InventoryItem", cItem);
        ResolveClass("DatabaseGun", cDbGun);
        ResolveField(cPlayer, "PlayerMain", "inventory", fInv);
        ResolveField(cPInv, "PlayerInventory", "equippedItems", fEq);
        ResolveField(cItem, "InventoryItem", "ammo", fAmmo);
        ResolveField(cDbGun, "DatabaseGun", "maxAmmo", fMaxAmmo);
        // Pilhas/reserva: stackCount x stackMax (+storage/items/id p/ reserva por tipo).
        ResolveField(cItem, "InventoryItem", "stackCount", fStack);
        ResolveField(cItem, "InventoryItem", "id", fId);
        ResolveField(cPInv, "PlayerInventory", "storage", fStorage);
        {
            MonoClass* cDbItem = nullptr;
            if (ResolveClass("DatabaseItem", cDbItem))
                ResolveField(cDbItem, "DatabaseItem", "stackMax", fDbStack);
            MonoClass* cCont = nullptr;
            if (ResolveClass("ItemContainer", cCont))
                ResolveField(cCont, "ItemContainer", "items", fItems);
        }
        if (cPEq) {
            // GetEquipment(EquipmentIndex): enum = valuetype; mono_class_get_method_
            // from_name pode exigir argc exato do enum. Tenta 1, depois 0 (fallback
            // diagnostico); se ambos falharem, ApplyAmmo usa caminho sem invoke
            // (caminha a List<InventoryItem>.weapons direto).
            MonoMethod* t = pMethodFrom(cPEq, "GetEquipment", 1);
            if (!t) t = pMethodFrom(cPEq, "GetEquipment", 0);
            if (t) { mGetEq = t; s.resolvedMethods++; }
            else Log::Warn("Metodo nao resolvido: PlayerEquippedItems.GetEquipment/1 (tentado 1 e 0)");
        }
        if (cItem) ResolveMethod(cItem, "InventoryItem", "GetDataBaseItem", 0, mGetDb);
        {
            MonoClass* cArms = nullptr;
            if (ResolveClass("PlayerArms", cArms))
                ResolveField(cArms, "PlayerArms", "selectedItem", fSel);
        }
        // Dinheiro: Currency.Instance -> Dollar/Silver/Gold -> amount.
        // AddCurrency(CurrencyID,int) p/ dar 1x; trava amount todo ciclo.
        ResolveClass("Currency", cCur);
        ResolveClass("CurrencyID", cCurId);
        ResolveField(cCur, "Currency", "Instance", fCurInst);
        ResolveField(cCur, "Currency", "<Dollar>k__BackingField", fDollar);
        {
            MonoClass* cData = nullptr;
            if (ResolveClass("CurrencyData", cData))
                ResolveField(cData, "CurrencyData", "amount", fAmount);
        }
        if (cCur) {
            MonoMethod* t = pMethodFrom(cCur, "AddCurrency", 2);
            if (t) { mAddCur = t; s.resolvedMethods++; }
            else Log::Warn("Metodo nao resolvido: Currency.AddCurrency/2");
        }
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
        }                     else Log::Warn("Imagem UnityEngine.CoreModule nao carregada.");

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
    // GetPos fora do orçamento (posicao e dado vital: box/skeleton
    // dependem dela; sem posicao a entidade some). O teto que protege a
    // horda e o de entidades/ciclo, nao este.
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
        // Ciclo 2 (spec VISUAL §4.2): worker respeita Config::bZombieSkeleton
        // (menu VISUAL manda; ReadLayout nao decide mais sozinho). Aliados usam
        // a mesma flag por enquanto (item 15 define a separacao).
        if (!Config::bZombieSkeleton) { out.skN = 0; return; }
        void* arr = ReadP(zo, Off::ZO_armature);
        if (!arr) { out.skN = 0; return; }
        long long len = 0;
        __try { memcpy(&len, (char*)arr + Off::A_len, sizeof(len)); }
        __except (EXCEPTION_EXECUTE_HANDLER) { out.skN = 0; return; }
        // Boss tem rig diferente (asas/cauda, outro tamanho): aceita rig menor
        // e mapeia por posicao relativa (head=0, pes=fim) em vez de descartar.
        // Comum continua exigindo 19 (indices auditados [BONE] 14/09).
        bool isBossRig = (len > 0 && len < 19);
        if (len < 19 && !isBossRig) { out.skN = 0; return; } // rig vazio: sem skeleton
        if (isBossRig) {
            // Mapeamento relativo: head=primeiro, neck=segundo, pes=ultimos.
            // Desenha o que projetar (skV por junta); sem maos estimadas.
            static const int kRel[6] = { 0, 1, 2, 3, 4, 5 };
            int upto = (int)len < 6 ? (int)len : 6;
            for (int k = 0; k < upto; ++k) {
                void* bone = nullptr;
                __try { memcpy(&bone, (char*)arr + Off::A_data + (size_t)k * 8, 8); }
                __except (EXCEPTION_EXECUTE_HANDLER) { bone = nullptr; }
                if (!bone) continue;
                Vec3 w, s3;
                if (!GetPos(bone, w)) continue;
                if (!Fin(w.x) || !Fin(w.y) || !Fin(w.z)) continue;
                if (!W2S(cam, w, s3)) continue;
                if (!Sane2(s3.x, s3.y)) continue;
                int dst = (k == 0) ? SkJoint::SK_HEAD : (k == 1) ? SkJoint::SK_NECK :
                    (k == 2) ? SkJoint::SK_SP3 : (k == 3) ? SkJoint::SK_SP1 :
                    (k == 4) ? SkJoint::SK_FL : SkJoint::SK_FR;
                out.skX[dst] = s3.x; out.skY[dst] = s3.y; out.skV[dst] = true;
            }
            return; // boss: sem resto do pipeline (indices de comum nao valem)
        }
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
        // Orçamento do ciclo: reseta a cada BuildEsp. Sem orçamento o skeleton
        // vira leitura barata (sem invoke) em vez de travar o jogo.
        s_budgetLeft = s_budgetMax;
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
            tmpEn.isBoss = false;
            // Nome real via ZombieType (memcpy + SEH, zero invoke).
            // Enum mapeado no metadata (dnlib, 17/09): 0=Tier1Civilian,
            // 1=Tier2Worker, 2=Tier3Combatant, 3=Tier4Military,
            // 4=Tier5Primordial, 5=FactoryWorker, 6=BossRiot, 7=BossQueen,
            // 8=BossReaper. Boss = type 6..8 (equivale a IsBoss() do jogo).
            // Caminho 1: Zombie.identity -> ZombieIdentity.type.
            // Caminho 2 (fallback): ZombieObject.zombieType direto (1o field
            // do ZombieObject; identity pode ser nulo em spawn/transicao).
            {
                static const char* kNames[9] = {
                    "Zombie", "Zombie", "Zombie", "Zombie", "Zombie", "Zombie",
                    "Zumbi de Assalto", "Zumbi Rainha", "Zumbi Ceifador"
                };
                static int oZType = -2; // offset de ZombieObject.zombieType (via API)
                if (oZType == -2) {
                    oZType = -1;
                    MonoClass* cZO = nullptr;
                    if (ResolveClass("ZombieObject", cZO) && cZO) {
                        MonoClassField* f = pFieldFrom(cZO, "zombieType");
                        oZType = FieldOff(f);
                    }
                }
                int tp = -999;
                void* zi = ReadP(e, Off::Z_identity);
                __try { if (zi) memcpy(&tp, (char*)zi + Off::ZI_type, sizeof(tp)); }
                __except (EXCEPTION_EXECUTE_HANDLER) { tp = -999; }
                const char* src = "identity";
                if ((tp < 0 || tp > 8) && zo && oZType >= 0) {
                    __try { memcpy(&tp, (char*)zo + oZType, sizeof(tp)); }
                    __except (EXCEPTION_EXECUTE_HANDLER) { tp = -999; }
                    src = "zotype";
                }
                if (tp >= 0 && tp <= 8) {
                    tmpEn.isBoss = (tp >= 6);
                    strncpy_s(tmpEn.name, sizeof(tmpEn.name), kNames[tp], _TRUNCATE);
                    static int s_ztypeSeen[16] = { 0 };
                    if (!s_ztypeSeen[tp]) {
                        s_ztypeSeen[tp] = 1;
                        Log::Infof("[ZTYPE] ent=0x%p type=%d (%s) boss=%d via=%s", e, tp, kNames[tp], tmpEn.isBoss ? 1 : 0, src);
                    }
                } else {
                    strncpy_s(tmpEn.name, sizeof(tmpEn.name), "Zombie", _TRUNCATE);
                    if (tp != -999)
                        Log::Infof("[ZTYPE] ent=0x%p type=%d (desconhecido) boss=0", e, tp);
                }
            }
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
                    // Kill-window melee (item 14b): revalida HP/isAlive apos o
                    // skeleton; mudou = publica box+skeleton (cor unica).
                    s_skDist2 = hasCamW ? (dist * dist) : -1.0f;
                    CollectJoints(zo, cam, tmpEn);
                    {
                        float hp3 = 0; unsigned char alive3 = 0;
                        __try {
                            memcpy(&hp3, (char*)h + Off::ZH_amount, sizeof(hp3));
                            memcpy(&alive3, (char*)h + Off::ZH_alive, 1);
                        } __except (EXCEPTION_EXECUTE_HANDLER) { hp3 = 0; alive3 = 0; }
                        if (!alive3 || hp3 <= 0 || hp3 != hp) {
                            s_ghostDead++;
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
                            en.onScreen = true; en.isAlly = false; en.isBoss = tmpEn.isBoss;
                            return;
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
                    en.ent = e; en.ex = bb.extents.x; en.ey = bb.extents.y; en.ez = bb.extents.z;
                    en.headX = en.headY = en.footX = en.footY = 0;
                    en.hp = hp; en.maxHp = mx;
                    en.onScreen = true; en.isAlly = false; en.isBoss = tmpEn.isBoss;
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
            EspEntry& en = tmp[n++];
            memcpy(en.name, tmpEn.name, sizeof(en.name));
            en.dist = dist;
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
            en.ent = e; en.ex = en.ey = en.ez = 0;
            en.hp = hp; en.maxHp = mx;
            en.onScreen = true; en.isAlly = false; en.isBoss = tmpEn.isBoss;
        });
        // Publicacao double-buffer (item 14b): worker escreve no back, vira o
        // ponteiro sob 1 CS curto. Present le o front SEM lock (ponteiro).
        // = tenta no proximo ciclo (33ms), nunca trava.
        if (!TryEnterCriticalSection(&s_espCS)) return;
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

    // Watchdog anti-hang no respawn/loading (item 14b, 3 modos de crash com o
    // mesmo denominador: invoke sem vivacidade em transicao). Checa TUDO que o
    // ciclo precisa ANTES de qualquer invoke: imagem C#, ZombieLoader vivo,
    // MainCamera.instance + cam+32 nao-nulo, lista de zumbis legivel. Falhou
    // qualquer um = transicao (kill = Destroy, loading = troca de cena):
    // worker dorme 500ms e tenta de novo — nunca invoca no escuro.
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
            // MainCamera.instance + cam+32 (a camera some no loading/respawn).
            MonoClass* cMC = pClassFrom(s_img, "", "MainCamera");
            if (!cMC) return false;
            MonoClassField* fI = pFieldFrom(cMC, "instance");
            if (!fI) return false;
            void* mcObj = nullptr;
            if (!StaticInstance(cMC, fI, mcObj) || !mcObj) return false;
            void* cam = ReadP(mcObj, Off::MC_cam);
            if (!cam) return false;
            memcpy((void*)&probe, cam, 1);
            // Lista de zumbis legivel (size dentro do teto).
            void* list = ReadP(zl, Off::ZL_zombies);
            if (!list) return false;
            int size = 0;
            memcpy(&size, (char*)list + Off::L_size, sizeof(size));
            if (size < 0 || size > 512) return false;
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
    // Ritmo adaptativo anti-crash: a worker mede o custo do BuildEsp e ajusta
    // o intervalo. Ciclo caro (>25ms, horda densa) = respira 66ms; ciclo leve
    // (<12ms) = volta pros 33ms.
    static int s_sleepMs = 33;
    // Gate geracao de mapa (item 14b, crash 02:17): LOD gerando celulas
    // destroi e recria objetos em massa. Sinal observavel sem invoke: a lista de zumbis OSCILA (spawn/despawn em rajada)
    // ou o totalReal diverge. Oscilou = mapa assentando: dorme 500ms, sem invoke.
    static int s_lastListN = -1;
    static int s_unstableN = 0;
    static bool MapSettling() {
        __try {
            if (!cZLoader || !fZLInst || !s_dom) return false;
            MonoVTable* vt = pVTable(s_dom, cZLoader);
            if (!vt) return false;
            void* zl = nullptr;
            pStaticGet(vt, fZLInst, &zl);
            if (!zl) return false;
            void* list = ReadP(zl, Off::ZL_zombies);
            if (!list) return false;
            int size = 0;
            memcpy(&size, (char*)list + Off::L_size, sizeof(size));
            if (size < 0 || size > 512) return true; // lista invalida = transicao
            if (s_lastListN < 0) { s_lastListN = size; return false; }
            int d = size - s_lastListN;
            if (d < 0) d = -d;
            s_lastListN = size;
            // Oscilacao >= 8 em 1 ciclo (~33ms) = spawn/despawn em rajada.
            if (d >= 8) {
                if (++s_unstableN >= 2) return true; // 2 ciclos seguidos = assentando
            } else s_unstableN = 0;
            return false;
        } __except (EXCEPTION_EXECUTE_HANDLER) { return true; }
    }
    static DWORD WINAPI EspThread(LPVOID) {
        pAttach(s_dom); // worker precisa do proprio attach no Mono
        Log::Info("Thread ESP iniciada (30Hz, fora do Present).");
        static int s_deadN = 0;
        static int s_defN = 0; // contador p/ defesa rapida (God/Stamina todo ciclo)
        while (s_espRun) {
            // Defesa rapida: God/Stamina rodam TODO ciclo (~33ms), com ou sem
            // ESP ligado. Leitura barata (1 lista curta + 2 floats); escrita so
            // se a flag ligada E o valor caiu (custo zero no estado estavel).
            // Comeca rapido e so desacelera se o proprio ciclo ficar caro.
            bool wantDef = s.ready && (Config::bGodMode || Config::bInfStamina || Config::bInfAmmo || Config::bInfItems || Config::bInfMoney);
            if (wantDef && SceneAlive()) {
                if (Config::bInfMoney) ApplyMoney(); // singleton, sem player
                void* pcs = nullptr;
                if (StaticInstance(cPlayers, fPCInst, pcs)) {
                    void* list = ReadP(pcs, Off::PCS_players);
                    WalkList(list, 16, [&](void* e, int) {
                        if (!InvokeBool(mHasLocal, e)) return;
                        ApplyDefense(e);
                        if (Config::bInfAmmo || Config::bInfItems) ApplyAmmo(e);
                    });
                }
                if (++s_defN >= 60) {
                    s_defN = 0;
                    ReadAll();
                    AuditBones();
                }
            }
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
                if (MapSettling()) {
                    // Mapa gerando celulas: zero invoke neste ciclo.
                    if (TryEnterCriticalSection(&s_espCS)) { s_espN = 0; LeaveCriticalSection(&s_espCS); }
                    Log::Warn("[SCENE] mapa assentando — ciclo pulado (LOD gerando).");
                    Sleep(500);
                    continue;
                }
                // AUDITORIA 16/09 (hang 02:44, PERF 1438ms): BuildEsp inteiro
                // rodava no MESMO ciclo. Com 86 zumbis, o ciclo estourava 1.4s
                // e o ritmo adaptativo so reagia DEPOIS.
                // Novo ritmo: posicao+skeleton TODO ciclo (barato, ~2ms).
                BuildEsp();
                // Defesa ja rodada no bloco rapido acima: aqui so o lento.
                if (s_defN == 0) { /* ReadAll/AuditBones feitos no ciclo rapido */ }
                else {
                    static int s_slowN = 0;
                    if (++s_slowN >= 60) {
                        s_slowN = 0;
                        ReadAll();
                        AuditBones();
                    }
                }
                PiFlush(false); // agregado [PI-CALL] 1x/5s (so sai se fail>0 ou >500us)
                // Adapta pelo custo medido no ciclo (s.espMs, media movel).
                // Teto duro: ciclo >100ms = respira 200ms (antes o max era 66ms
                // e o hang vinha com ciclo de 1438ms sem freio).
                int want = (s.espMs > 100.0f) ? 200 : (s.espMs > 25.0f) ? 66 : (s.espMs < 12.0f ? 33 : 50);
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

    // Municao infinita (cadeia confirmada no IL do ShootGun via dnlib):
    // local.inventory -> equippedItems -> GetEquipment(selectedItem do arms)
    // -> InventoryItem.ammo. Reescreve ammo=maxAmmo quando cai.
    // Regras: so no local, so se bInfAmmo, offsets via API, falha 1x loga e
    // desliga sozinho (s_ammoOk=false) sem travar a worker.
    // Fallback sem invoke: se GetEquipment nao resolver, caminha a
    // List<InventoryItem>.weapons de PlayerEquippedItems e trava o ammo de
    // TODOS os itens da lista (custo: 2-4 escritas/ciclo, sem invoke).
    static bool s_ammoOk = true;
    static int s_ammoMode = 0; // 0=desconhecido 1=invoke 2=lista
    static int s_ammoIdCur = -1; // ammoID da arma equipada (reserva filtra por ele)
    static void ApplyAmmo(void* local) {
        if (!local || !s_ammoOk) return;
        // Resolve preguiçoso dos offsets (1x; API pode indisponivel no Unity 6).
        static int oInv = -2, oEq = -2, oSel = -2, oAmmo = -2, oMax = -2;
        static int oArms = -2, oWeapons = -2;
        static MonoClassField* fArms = nullptr;
        static MonoClassField* fWeapons = nullptr;
        if (oInv == -2) {
            oInv = FieldOff(fInv); oEq = FieldOff(fEq); oSel = FieldOff(fSel);
            oAmmo = FieldOff(fAmmo); oMax = FieldOff(fMaxAmmo);
            if (cPlayer) fArms = pFieldFrom(cPlayer, "arms");
            oArms = FieldOff(fArms);
            if (cPEq) fWeapons = pFieldFrom(cPEq, "weapons");
            oWeapons = FieldOff(fWeapons);
            s_ammoMode = (mGetEq && oSel >= 0 && oArms >= 0) ? 1 : (oWeapons >= 0 ? 2 : 0);
            if (!s_ammoLogged) {
                s_ammoLogged = true;
                Log::Infof("[AMMO] offs inv=%d eq=%d arms=%d sel=%d ammo=%d max=%d weapons=%d mGetEq=%d mGetDb=%d modo=%d",
                    oInv, oEq, oArms, oSel, oAmmo, oMax, oWeapons, mGetEq ? 1 : 0, mGetDb ? 1 : 0, s_ammoMode);
            }
            if (oInv < 0 || oEq < 0 || oAmmo < 0 || s_ammoMode == 0) {
                Log::Warn("[AMMO] cadeia incompleta — municao infinita desativada (sem crash).");
                s_ammoOk = false;
                return;
            }
        }
        if (oInv < 0) return; // ja desativado acima
        __try {
            void* pinv = ReadP(local, oInv);
            if (!pinv) return;
            void* peq = ReadP(pinv, oEq);
            if (!peq) return;
            if (s_ammoMode == 1) {
                void* arms = ReadP(local, oArms);
                if (!arms) return;
                int sel = ReadI(arms, oSel, -1);
                if (sel < 0 || sel > 32) return; // EquipmentIndex plausivel
                // GetEquipment(EquipmentIndex): enum passa como int32 por valor.
                int selArg = sel;
                void* args[1] = { &selArg };
                void* item = InvokeObj(mGetEq, peq, args);
                if (item) {
                    TopAmmo(item, oAmmo, oMax);
                    // Guarda o ammoID da arma p/ reserva travar so o mesmo tipo.
                    static int oAmmoId = -2;
                    if (oAmmoId == -2) {
                        oAmmoId = -1;
                        if (cDbGun) {
                            MonoClassField* f = pFieldFrom(cDbGun, "ammoID");
                            oAmmoId = FieldOff(f);
                        }
                    }
                    if (oAmmoId >= 0 && mGetDb) {
                        void* db = InvokeObj(mGetDb, item, nullptr);
                        if (db) {
                            int aid = ReadI(db, oAmmoId, -1);
                            if (aid >= 0) s_ammoIdCur = aid;
                        }
                    }
                }
                // invoke falhou (retorno nulo)? cai p/ modo lista no proximo ciclo.
                else if (oWeapons >= 0) s_ammoMode = 2;
            } else {
                // Modo lista (fallback sem invoke): trava o ammo de cada
                // InventoryItem em weapons — so ARMAS (stackMax==1). Pilhas
                // (granada etc.) nao entram aqui (TopStacks cuida, com filtro).
                void* list = ReadP(peq, oWeapons);
                if (list) WalkList(list, 16, [&](void* item, int) {
                    TopAmmo(item, oAmmo, oMax);
                });
            }
            // Reserva (bInfAmmo) + pilhas gerais (bInfItems): offsets resolvidos 1x.
            static int oStorage = -2, oItems = -2, oStack = -2, oDbStack = -2, oId = -2;
            if (oStorage == -2) {
                oStorage = FieldOff(fStorage); oItems = FieldOff(fItems);
                oStack = FieldOff(fStack); oDbStack = FieldOff(fDbStack);
                oId = FieldOff(fId);
            }
            if (Config::bInfAmmo)
                TopStacks(local, oInv, oStorage, oItems, oStack, oDbStack, oId, false);
            if (Config::bInfItems)
                TopStacks(local, oInv, oStorage, oItems, oStack, oDbStack, oId, true);
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // Leva o ammo de um InventoryItem ao teto (maxAmmo via DatabaseGun;
    // fallback: segura o maior valor ja visto). So escreve se caiu.
    // Anti-flood: so mexe se o item for ARMA (stackMax==1 via DatabaseItem).
    // Pilha (granada/bala solta) tem stackMax>1 e eh ignorada aqui.
    static void TopAmmo(void* item, int oAmmo, int oMax) {
        if (!item || oAmmo < 0) return;
        static int oDbStackGun = -2;
        if (oDbStackGun == -2) {
            oDbStackGun = FieldOff(fDbStack);
        }
        if (mGetDb && oDbStackGun >= 0) {
            void* db0 = InvokeObj(mGetDb, item, nullptr);
            if (db0) {
                int sm = ReadI(db0, oDbStackGun, -1);
                if (sm > 1) return; // pilha: nao eh arma, TopStacks cuida
            }
        }
        int cur = ReadI(item, oAmmo, -1);
        if (cur < 0) return;
        int max = -1;
        if (mGetDb && oMax >= 0) {
            void* db = InvokeObj(mGetDb, item, nullptr);
            if (db) max = ReadI(db, oMax, -1);
        }
        if (max <= 0 || max > 100000) {
            static int s_lastAmmo = -1;
            if (cur > s_lastAmmo) s_lastAmmo = cur;
            if (s_lastAmmo > 0 && cur < s_lastAmmo) WriteI(item, oAmmo, s_lastAmmo);
            return;
        }
        if (cur < max) WriteI(item, oAmmo, max);
    }

    // Reserva + pilhas (regra universal do IL Get/SetGenericNumericValue:
    // stackMax==1 -> o numero eh ammo; senao eh stackCount; teto = stackMax).
    // - Com bInfAmmo: trava stackCount=stackMax nas pilhas do storage cujo
    //   id == ammoID da arma (HUD reserva honesto: 30/150, nao 30/0).
    // - Com bInfItems: trava stackCount=stackMax em pilhas PEQUENAS
    //   (granada/dinamite/bandagem: stackMax<=32). Materiais (madeira/sucata,
    //   stack alto) ficam DE FORA: travar material + ProcessItemStacking que
    //   soma pilhas = saldo andando sozinho (flood, 17/09).
    // - Anti-flood: so escreve se 0 <= cur < smax (nunca cria, nunca soma).
    static void TopStacks(void* local, int oInv, int oStorage, int oItems,
        int oStack, int oDbStack, int oId, bool items) {
        if (!local || oInv < 0 || oStorage < 0 || oItems < 0) return;
        if (oStack < 0 || oDbStack < 0) return;
        __try {
            void* pinv = ReadP(local, oInv);
            if (!pinv) return;
            void* cont = ReadP(pinv, oStorage);
            if (!cont) return;
            void* list = ReadP(cont, oItems);
            if (!list) return;
            WalkList(list, 64, [&](void* it, int) {
                if (!mGetDb) return;
                void* db = InvokeObj(mGetDb, it, nullptr);
                if (!db) return;
                int smax = ReadI(db, oDbStack, -1);
                if (smax <= 1) return; // stackMax==1 -> eh arma (ammo), nao pilha
                if (smax > 100000) return;
                if (items && smax > 32) return; // material: fora (anti-flood)
                if (!items) {
                    // Reserva: so o mesmo tipo da arma (id == ammoID equipada).
                    if (oId < 0 || s_ammoIdCur < 0) return;
                    int id = ReadI(it, oId, -1);
                    if (id != s_ammoIdCur) return;
                }
                int cur = ReadI(it, oStack, -1);
                if (cur >= 0 && cur < smax) WriteI(it, oStack, smax);
            });
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // Dinheiro infinito (MISC/Sobrevivencia): 1x AddCurrency(Dollars, 99999)
    // por sessao + trava amount=99999 todo ciclo. Singleton via Instance;
    // offsets via API; falha = desliga sozinho, sem crash.
    static void ApplyMoney() {
        if (!s_moneyOk) return;
        static int oDollar = -2, oAmount = -2;
        if (oDollar == -2) {
            oDollar = FieldOff(fDollar); oAmount = FieldOff(fAmount);
            if (!s_moneyLogged) {
                s_moneyLogged = true;
                Log::Infof("[MONEY] offs dollar=%d amount=%d mAddCur=%d",
                    oDollar, oAmount, mAddCur ? 1 : 0);
            }
            if (oDollar < 0 || oAmount < 0) {
                Log::Warn("[MONEY] cadeia incompleta — desativado.");
                s_moneyOk = false;
                return;
            }
        }
        if (oDollar < 0) return;
        __try {
            void* inst = nullptr;
            if (!StaticInstance(cCur, fCurInst, inst) || !inst) return;
            // 1x por sessao: AddCurrency(Dollars=0, 99999) via invoke.
            if (mAddCur && !s_moneyGiven) {
                s_moneyGiven = true;
                int idDollars = 0, qty = 99999;
                void* args[2] = { &idDollars, &qty };
                MonoObject* exc = nullptr;
                __try { pInvoke(mAddCur, inst, args, &exc); }
                __except (EXCEPTION_EXECUTE_HANDLER) {}
                Log::Info("[MONEY] AddCurrency 1x executado.");
            }
            void* dollar = ReadP(inst, oDollar);
            if (!dollar) return;
            int cur = ReadI(dollar, oAmount, -1);
            if (cur >= 0 && cur < 99999) WriteI(dollar, oAmount, 99999);
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // Defesa (God + Stamina): reescreve os campos do LOCAL player.
    // Roda dentro do ReadAll (worker, 1x/2s): dano entre ciclos e absorvido
    // no ciclo seguinte; sem hook, sem patch de codigo, sem custo desligado.
    static void ApplyDefense(void* local) {
        if (!local) return;
        // God Mode: trava healthFast + healthSlow em 100.
        if (Config::bGodMode) {
            float hp = ReadF(local, Off::PM_healthFast);
            if (hp > 0.0f && hp < 100.0f) { // morto (<=0) nao ressuscita
                WriteF(local, Off::PM_healthFast, 100.0f);
                WriteF(local, Off::PM_healthSlow, 100.0f);
            }
        }
        // Stamina: trava fast+slow no maxStamina (correr sem cansar).
        if (Config::bInfStamina) {
            float mx = ReadF(local, Off::PM_maxStamina, 100.0f);
            if (!(mx > 0.0f && mx <= 1000.0f)) mx = 100.0f;
            if (ReadF(local, Off::PM_staminaFast) < mx)
                WriteF(local, Off::PM_staminaFast, mx);
            if (ReadF(local, Off::PM_staminaSlow) < mx)
                WriteF(local, Off::PM_staminaSlow, mx);
        }
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
        void* localEnt = nullptr;
        if (StaticInstance(cPlayers, fPCInst, pcs)) {
            void* list = ReadP(pcs, Off::PCS_players);
            s.players = WalkList(list, 16, [&](void* e, int) {
                bool local = InvokeBool(mHasLocal, e);
                float hp = ReadF(e, Off::PM_healthFast);
                if (hp < 0 || hp > 100000) return;
                if (local && !gotLocal) {
                    gotLocal = true;
                    localEnt = e;
                    s.localHp = hp;
                    s.localStam = ReadF(e, Off::PM_staminaFast);
                } else if (!local && !gotAlly) {
                    gotAlly = true;
                    s.allyHp = hp;
                }
            });
            if (!gotLocal) s.localHp = 0;
            else ApplyDefense(localEnt); // escrita so no local, so se ligado
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
        // Present NUNCA invoca — so copia snapshot (Get/GetEsp).
        // ReadAll/AuditBones migraram p/ worker (EspThread, 1x/2s).
        ++s_tick;
        if (!s.ready && !Init()) return;
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





















