#include "aim_logic.h"
#include "monotonic_time.h"
#include "latest_snapshot.h"
#include "mono.h"
#include "modifier_math.h"
#include "runtime_settings.h"
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
typedef void* (__cdecl* FnSigGetParam)(void*, void**); // (sig, iter) â€” GeoArray-like, iter avanza
typedef int (__cdecl* FnTypeGetType)(void*);
typedef const char* (__cdecl* FnTypeGetName)(void*); // mono_type_get_name (auditoria [SIG])
typedef void* (__cdecl* FnClassGetFields)(void*, void**); // mono_class_get_fields (auditoria [FIELDS])
typedef const char* (__cdecl* FnFieldGetName)(void*); // mono_field_get_name
typedef int (__cdecl* FnFieldGetOff)(void*); // mono_field_get_offset (pode nao existir)
typedef MonoObject* (__cdecl* FnRuntimeInvoke)(MonoMethod*, void*, void**, MonoObject**);
typedef void*       (__cdecl* FnObjectUnbox)(MonoObject*);
typedef char*       (__cdecl* FnStringUtf8)(MonoObject*);
typedef void        (__cdecl* FnFree)(void*);
typedef void (__cdecl* AssemblyCallback)(void*, void*);
typedef void (__cdecl* FnAssmForeach)(AssemblyCallback, void*);
typedef int (__cdecl* FnClassValueSize)(MonoClass*, unsigned*); // mono_assembly_foreach
typedef void*       (__cdecl* FnAssmImage)(void*); // mono_assembly_get_image

// Offsets validados (auditoria #1). Nao adivinhar: tudo veio de CE MCP.
namespace Off {
    // PlayersController
    constexpr int PCS_players = 48;
    // PlayerMain (instancia) â€” offsets validados OFFSETS.md (CE MCP 12/09).
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
    static FnAssmForeach    pAssmForeach = nullptr;
    static FnAssmImage      pAssmImage = nullptr;
    static FnClassValueSize pClassValueSize = nullptr;
    static MonoMethod* mGetName = nullptr;
    static MonoMethod* mGetBounds = nullptr; // Renderer.get_bounds (Box 3D real)
    static MonoMethod* mGetViewMat = nullptr; // Camera.get_worldToCameraMatrix (VP proprio)
    static MonoMethod* mGetProjMat = nullptr; // Camera.get_projectionMatrix (VP proprio)
    static float s_vp[16];       // VP = P*V (column-major, padrao Unity)
    static bool  s_vpOk = false;
    static RuntimeSettings::Channel s_settings;
    static RuntimeSettings::Values s_options;
    static float s_vpW = 1920.0f, s_vpH = 1080.0f;
    static int s_ghostDead = 0;   // HP>0 mas isAlive=false (animacao de morte)
    static int s_ghostBad = 0;    // centro/pos nao-finito, absurdo ou origem
    static int s_lastN = 0;

    static MonoDomain* s_dom = nullptr;
    static MonoImage*  s_img = nullptr;

    static MonoClass* cDay = nullptr;
    static MonoClass* cPlayer = nullptr;
    static MonoClass* cZombie = nullptr;
    static MonoClass* cZLoader = nullptr;
    static MonoClass* cPlayers = nullptr;
    static MonoClass* cMp = nullptr;          // MultiplayerController
    static MonoClassField* fMpInst = nullptr; // MultiplayerController.instance
    static MonoMethod* mIsServer = nullptr;   // MultiplayerController.IsServer()
    static MonoMethod* mIsSingle = nullptr;   // MultiplayerController.get_IsSinglePlayer()
    static MonoMethod* mIsMulti = nullptr;    // MultiplayerController.get_IsMultiplayer()
    static MonoMethod* mIsClient = nullptr;   // MultiplayerController.IsClient()
    static MonoClass* cLobby = nullptr;
    static MonoClass* cLobbyPlayer = nullptr;
    static MonoClassField* fLobbyInst = nullptr;
    static MonoClassField* fLobbyPlayerName = nullptr;
    static MonoMethod* mLobbyGetHost = nullptr;
    static MonoMethod* mGetLobbyCode = nullptr;
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
    static MonoMethod* mTryReload = nullptr; // PlayerArms.TryStartReload (coop: recarga legitima)
    static MonoMethod* mCreateItem = nullptr; // InventoryItem.CreateInventoryItem(ID,int)
    static MonoMethod* mAddItem = nullptr;    // PlayerInventory.AddItem(item,filter)
    static MonoMethod* mGotLoot = nullptr;    // PlayerInteraction.GotLootFromServer(ID,int) â€” seed nativo
    static MonoMethod* mDropLoot = nullptr;   // PlayerInventory.DropLoot(item)
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
    // LoadoutSelector: desbloqueio de armas/itens do vendedor.
    static MonoClass* cLoadout = nullptr;        // LoadoutSelector
    static MonoClassField* fLoadInst = nullptr;  // LoadoutSelector.instance
    static MonoMethod* mUnlockAll = nullptr;     // LoadoutSelector.UnlockAll()
    static bool s_loadoutDone = false;           // 1x por sessao
    // Arma/Movimento (sessao dnlib 19/09, dump/sessao-2026-09-19_12-00.txt).
    // Tudo via nome (FieldOff) â€” offsets reais so em runtime.
    static MonoClass* cArms = nullptr;        // PlayerArms (selectedItem, EquippedGun)
    static MonoClass* cWBase = nullptr;       // WeaponBase (singleton: precisionMultiplier, gunSway)
    static MonoClass* cDbgMod = nullptr;      // DebugModifiers (singleton: General)
    static MonoClass* cDbgGen = nullptr;      // DebugGeneralModifiers (DisableSway)
    static MonoClass* cDbgBool = nullptr;     // DebugBoolean (value : Boolean)
    static MonoClass* cPMeleeAtk = nullptr;   // PlayerMeleeAttack (Duration por golpe)
    static MonoClass* cMoveSet = nullptr;     // MeleeMoveSet (nodes[] por arma)
    static MonoClass* cAtkBase = nullptr;     // MeleeAttackBase (singleton: AllAttacks)
    static MonoClassField* fAtkInst = nullptr; // MeleeAttackBase.Instance
    static MonoClassField* fAtkAll = nullptr;  // MeleeAttackBase.AllAttacks (dict ID->attack)
    static MonoClass* cHud = nullptr;          // PlayerHUD (singleton: crosshair visual)
    static MonoClassField* fHudInst = nullptr; // PlayerHUD.instance
    static MonoClassField* fHudInner = nullptr; // PlayerHUD.innerCrossHairTransform
    static MonoClassField* fHudLines = nullptr; // PlayerHUD.crossHairLine (RawImage[])
    static MonoClass* cMove = nullptr;        // PlayerMovement (jumpSpeed)
    static MonoClassField* fWBaseInst = nullptr;  // WeaponBase.instance
    static MonoClassField* fPrecMult = nullptr;   // WeaponBase.precisionMultiplier
    static MonoClassField* fGunSway = nullptr;    // WeaponBase.gunSway
    static MonoClassField* fDbgGen = nullptr;     // DebugModifiers.General
    static MonoClassField* fDisSway = nullptr;    // DebugGeneralModifiers.DisableSway
    static MonoClassField* fBoolVal = nullptr;    // DebugBoolean.value
    static MonoClassField* fGunRecoil = nullptr;  // DatabaseGun.recoil (Vector2 = 2 floats)
    static MonoClassField* fGunRecoilRnd = nullptr; // DatabaseGun.recoilRandomness
    static MonoClassField* fGunSpread = nullptr;  // DatabaseGun.spread
    static MonoClassField* fGunRof = nullptr;     // DatabaseGun.rof
    static MonoClassField* fGunFullAuto = nullptr; // DatabaseGun.fullAuto (1-tiro -> rajada)
    static MonoClassField* fGunBurst = nullptr;   // DatabaseGun.burstCount (rajada -> auto)
    static MonoClassField* fAtkDur = nullptr;     // PlayerMeleeAttack.Duration
    static MonoClassField* fNodeTMin = nullptr;   // MeleeMoveSetNode.transitionTimeMinimum
    static MonoClassField* fNodeTMax = nullptr;   // MeleeMoveSetNode.transitionTimeMaximum
    static MonoClassField* fMoveJump = nullptr;   // PlayerMovement.jumpSpeed
    static void ApplyWeapon(void* local); // forward (recoil/spread/sway/rapid, worker)

    static MonoClassField* fDayInst = nullptr;
    static MonoClassField* fZLInst = nullptr;
    static MonoClassField* fPCInst = nullptr;
    static MonoMethod* mHasLocal = nullptr;
    // Telemetria por site de pInvoke (Fase 1, SEM mudar logica): conta chamadas
    // e falhas por ciclo na worker; o agregado sai 1x/5s no [PI-CALL], e so
    // loga se fail>0 OU media>500us. Leitura atÃ´mica nao precisa (worker unica
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
        return MonotonicTime::Microseconds(t.QuadPart, f.QuadPart);
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
    // OrÃ§amento de invokes por ciclo de BuildEsp (anti-crash em horda).
    // Cada invoke cruza para o Mono e compete com o jogo; com 50+ zumbis,
    // centenas de invokes por ciclo de 33ms viram corrida com o LOD
    // (ver crash 15/09 15:51). Estoura o teto? O resto do ciclo usa o
    // Ãºltimo valor conhecido (fail-open, sem flicker).
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
    static MonoMethod* mGetEuler = nullptr; // Transform.get_eulerAngles -> yaw atual (mira malha fechada)
    static MonoClass* cPhys = nullptr; // UnityEngine.Physics (IsVisible/RaycastAll)
    static bool s_boneLogged = false;
    static bool s_jointLogged = false; // auditoria juntas (1x por sessao)
    static bool s_skelLogged = false; // diagnostico SKEL (1x: mascara + tela dos bracos)
    static bool s_handLogged = false; // diagnostico HAND (1x: ponta da mao em mundo)
    static bool s_handLogged2 = false; // diagnostico HAND2 (1x: maos vivas pos-fix)
    static DWORD WINAPI BootstrapThread(LPVOID); // forward (definida apos BuildEsp)
    static MonoClass* FindUnityClass(const char* name, MonoImage** image = nullptr);
    static MonoImage* UnityImageWithTransform(void); // imagem com UnityEngine.Transform
    static void ApplyMoney(); // forward (dinheiro infinito, worker)
    // REMOVIDO 17/09: DoGiveItem (spawn de itens).
    static void ApplyDefense(void* local); // forward (defesa rapida, worker)
    static void ApplyLoadout(); // forward (LoadoutSelector.UnlockAll, worker)
    static void TopAmmo(void* item, int oAmmo, int oMax); // forward (teto do ammo)
    static void TopStacks(void* local, int oInv, int oStorage, int oItems,
        int oStack, int oDbStack, int oId, bool items); // forward (reserva+pilhas)
    static bool SubTypeOk(void* db); // forward (observa categoria, 1x por valor)
    static void ApplyAmmo(void* local, bool coop = false); // forward (coop=pente livre)
    static int FieldOff(MonoClassField* f); // forward (offset via API, -1 se falhar)
    static void* DbCached(void* item); // forward (cache de GetDataBaseItem, anti-hang)
    static void ItemsReport(void* local, int oInv, int oStorage, int oItems,
        int oStack, int oDbStack, int oId); // forward (relatorio Items, apos TopStacks)
    static void ItemsReportReset(); // forward (reset do relatorio ao desligar)
    static void ResetRestoreCaches();
    static bool HasPendingRestores();
    static void AmmoCachesClear(); // forward (limpa caches na troca de cena)
    static void ReadAll(); // forward (chamada na worker, fora do Present)
    static void AuditBones(); // forward (chamada na worker, fora do Present)
    static void WohaxAim(void* local); // forward (maquina wohax)
    static MonoImage*  s_unity = nullptr;
    static MonoClass*  cCamU = nullptr;
    static MonoClass*  cTrans = nullptr;
    static MonoMethod* mGetPos = nullptr;
    static MonoMethod* mW2S = nullptr;
    static MonoMethod* mGetTrans = nullptr;
    static Vec3 s_camW = { 0, 0, 0 }; // posicao da camera do ciclo (gate skeleton)
    static bool s_camWok = false;
    static float s_skDist2 = -1.0f; // dist2 da entidade atual (gate skeleton, sem invoke)
    // Each thread owns its buffer; publication never overwrites the reader.
    struct EntitySnapshot { EspEntry entries[128] = {}; int count = 0; };
    static LatestSnapshot<EntitySnapshot> s_entitySnapshot;
    static std::atomic<bool> s_bootstrapStarted{false};
    static MonoThread* s_bootstrapMonoThread = nullptr;
    static LatestSnapshot<State> s_stateSnapshot;
    static void __cdecl GameUpdate();
    static void RunGameCycle();
    static void ClearWorldEsp();
    static void ClearDistantEsp();
    static long long s_nextCycle = 0;
    static void ClearEntitySnapshot() { s_entitySnapshot.Publish(EntitySnapshot{}); ClearWorldEsp(); ClearDistantEsp(); }
    static std::atomic<bool> s_espRun{false};
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

    // REMOVIDO 17/09 (pedido do operador): guardiao Anti-Flood.
    // Motivo: a quarentena bania o proprio infinito (escrevia 3x e se
    // auto-bloqueava 30s). Volta o modelo simples: trava direta no teto
    // (maxAmmo/stackMax lidos do item), sem historico, sem quarentena.
    // Repeticao eh esperada (todo ciclo repoe) e nao eh problema.

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
    struct DbCache { void* item; void* db; long long tick; };
    static DbCache s_dbCache[64];
    static long long s_dbTick = 0;
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

    // DbCached: GetDataBaseItem com cache curto (anti-hang 18/09).
    // db muda so em troca/loot (raro): 1 invoke por item, reusa por 300
    // ciclos no meio (memcpy+SEH). Sem isso, 30Hz x N itens x GetDb = hang.
    static void* DbCached(void* item) {
        if (!item || !mGetDb) return nullptr;
        for (int k = 0; k < 64; ++k) {
            if (s_dbCache[k].item == item) {
                if (s_dbTick - s_dbCache[k].tick < 300) return s_dbCache[k].db;
                break;
            }
        }
        void* db = InvokeObj(mGetDb, item, nullptr);
        if (!db) return nullptr;
        int slot = 0;
        long long oldest = s_dbCache[0].tick;
        for (int k = 0; k < 64; ++k) {
            if (!s_dbCache[k].item) { slot = k; break; }
            if (s_dbCache[k].tick < oldest) { oldest = s_dbCache[k].tick; slot = k; }
        }
        s_dbCache[slot].item = item;
        s_dbCache[slot].db = db;
        s_dbCache[slot].tick = s_dbTick;
        return db;
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
    static int WalkList(void* list, int expectMax, Fn fn, int start = 0) {
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
                const int index = ((start % size) + i) % size;
                void* e = nullptr;
                memcpy(&e, (char*)arr + Off::A_data + (size_t)index * 8, 8);
                if (!e) continue;
                // prova de leitura do elemento
                volatile char probe = 0;
                memcpy((void*)&probe, e, 1);
                fn(e, index);
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

    struct UnityClassQuery { const char* name; MonoClass* klass; MonoImage* image; };
    static void __cdecl FindUnityClassInAssembly(void* assembly, void* context) {
        auto& query = *static_cast<UnityClassQuery*>(context);
        if (query.klass || !assembly) return;
        auto image = static_cast<MonoImage*>(pAssmImage(assembly));
        if (!image) return;
        query.klass = pClassFrom(image, "UnityEngine", query.name);
        if (query.klass) query.image = image;
    }
    static MonoClass* FindUnityClass(const char* name, MonoImage** image) {
        UnityClassQuery query = {name, nullptr, nullptr};
        if (!pAssmForeach || !pAssmImage || !pClassFrom) return nullptr;
        __try { pAssmForeach(FindUnityClassInAssembly, &query); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
        if (image) *image = query.image;
        return query.klass;
    }
    static MonoImage* UnityImageWithTransform() {
        MonoImage* image = nullptr;
        FindUnityClass("Transform", &image);
        return image;
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
            // Enumera imagens p/ achar UnityEngine.PhysicsModule (nao-fatal).
            Bind(m, "mono_assembly_foreach", pAssmForeach);
            Bind(m, "mono_assembly_get_image", pAssmImage);
            Bind(m, "mono_class_value_size", pClassValueSize);
            if (!ok) { Log::Error("Mono bind incompleto."); return false; }
            s_dom = pGetRoot();
            if (!s_dom) return false;
            s_bootstrapMonoThread = pAttach(s_dom);
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
        // Coop host vs cliente: MultiplayerController.IsServer() (IL do DropLoot).
        // Host valida o proprio dano: trava tudo. Cliente: recarga legitima.
        ResolveClass("MultiplayerController", cMp);
        ResolveField(cMp, "MultiplayerController", "instance", fMpInst);
        if (cMp) {
            ResolveMethod(cMp, "MultiplayerController", "IsServer", 0, mIsServer);
            ResolveMethod(cMp, "MultiplayerController", "get_IsSinglePlayer", 0, mIsSingle);
            ResolveMethod(cMp, "MultiplayerController", "get_IsMultiplayer", 0, mIsMulti);
            ResolveMethod(cMp, "MultiplayerController", "IsClient", 0, mIsClient);
            ResolveMethod(cMp, "MultiplayerController", "GetLobbyCode", 0, mGetLobbyCode);
        }
        ResolveClass("LobbyController", cLobby);
        ResolveField(cLobby, "LobbyController", "instance", fLobbyInst);
        if (cLobby) mLobbyGetHost = pMethodFrom(cLobby, "GetHost", 0);
        ResolveClass("LobbyPlayer", cLobbyPlayer);
        if (cLobbyPlayer) fLobbyPlayerName = pFieldFrom(cLobbyPlayer, "playerName");
        ResolveField(cDay, "DaytimeController", "instance", fDayInst);
        ResolveField(cZLoader, "ZombieLoader", "Instance", fZLInst);
        ResolveField(cPlayers, "PlayersController", "instance", fPCInst);
        ResolveMethod(cPlayer, "PlayerMain", "get_HasLocalControl", 0, mHasLocal);
        // Municao: classes/campos/metodos (falha = nao-fatal; ApplyAmmo desliga sozinho).
        // selectedItem fica no PlayerArms (nao no PlayerMain): resolve via cPlayer? nao â€”
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
        // Seed nativo (auditoria 21/09): PlayerInteraction.GotLootFromServer
        // = caminho do Descarregar (cria pilha cheia no storage). Resolve 1x.
        {
            MonoClass* cInter = nullptr;
            if (ResolveClass("PlayerInteraction", cInter) && cInter)
                ResolveMethod(cInter, "PlayerInteraction", "GotLootFromServer", 2, mGotLoot);
        }
        {
            MonoClass* cArms = nullptr;
            if (ResolveClass("PlayerArms", cArms)) {
                ResolveField(cArms, "PlayerArms", "selectedItem", fSel);
                // Coop: recarga automatica legitima (ammo==0 -> ReloadGun).
                // Fluxo normal do jogo: host aceita, dano conta.
                MonoMethod* t = pMethodFrom(cArms, "TryStartReload", 0);
                if (t) { mTryReload = t; s.resolvedMethods++; }
                else Log::Warn("Metodo nao resolvido: PlayerArms.TryStartReload/0");
            }
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
        // LoadoutSelector: UnlockAll (desbloqueio de armas/itens do vendedor).
        ResolveClass("LoadoutSelector", cLoadout);
        ResolveField(cLoadout, "LoadoutSelector", "instance", fLoadInst);
        if (cLoadout) ResolveMethod(cLoadout, "LoadoutSelector", "UnlockAll", 0, mUnlockAll);
        // Arma/Movimento (sessao dnlib 19/09): classes/campos por nome.
        // Falha = nao-fatal; cada Apply desliga sozinho (log 1x).
        ResolveClass("PlayerArms", cArms);
        ResolveClass("WeaponBase", cWBase);
        ResolveField(cWBase, "WeaponBase", "instance", fWBaseInst);
        ResolveField(cWBase, "WeaponBase", "precisionMultiplier", fPrecMult);
        ResolveField(cWBase, "WeaponBase", "gunSway", fGunSway);
        ResolveClass("DebugModifiers", cDbgMod);
        ResolveClass("DebugGeneralModifiers", cDbgGen);
        ResolveClass("DebugBoolean", cDbgBool);
        // Defeito 2 (auditoria 21/09): faltava General â€” dbg sempre 0.
        // Nome exato via dnlib: field "general" em DebugModifiers.
        ResolveField(cDbgMod, "DebugModifiers", "General", fDbgGen);
        if (!fDbgGen) ResolveField(cDbgMod, "DebugModifiers", "<General>k__BackingField", fDbgGen);
        ResolveField(cDbgGen, "DebugGeneralModifiers", "DisableSway", fDisSway);
        ResolveField(cDbgBool, "DebugBoolean", "value", fBoolVal);
        ResolveField(cDbGun, "DatabaseGun", "recoil", fGunRecoil);
        ResolveField(cDbGun, "DatabaseGun", "recoilRandomness", fGunRecoilRnd);
        ResolveField(cDbGun, "DatabaseGun", "spread", fGunSpread);
        ResolveField(cDbGun, "DatabaseGun", "rof", fGunRof);
        ResolveField(cDbGun, "DatabaseGun", "fullAuto", fGunFullAuto);
        ResolveField(cDbGun, "DatabaseGun", "burstCount", fGunBurst);
        ResolveClass("PlayerMeleeAttack", cPMeleeAtk);
        ResolveField(cPMeleeAtk, "PlayerMeleeAttack", "<Duration>k__BackingField", fAtkDur);
        ResolveClass("MeleeMoveSet", cMoveSet);
        // Mira fechada: PlayerHUD.instance (crosshair visual = RectTransform).
        ResolveClass("PlayerHUD", cHud);
        ResolveField(cHud, "PlayerHUD", "instance", fHudInst);
        ResolveField(cHud, "PlayerHUD", "innerCrossHairTransform", fHudInner);
        ResolveField(cHud, "PlayerHUD", "crossHairLine", fHudLines);
        // Base global de golpes: MeleeAttackBase.Instance.AllAttacks cobre
        // TODA arma branca (pa/pa/facao/faca/taco) sem depender da mao.
        ResolveClass("MeleeAttackBase", cAtkBase);
        ResolveField(cAtkBase, "MeleeAttackBase", "Instance", fAtkInst);
        ResolveField(cAtkBase, "MeleeAttackBase", "AllAttacks", fAtkAll);
        // Node = nested ValueType (MeleeMoveSet/MeleeMoveSetNode): resolve via
        // classe pai + get_nested_types? nao ha API simples â€” usa o field do
        // array (nodes) e calcula tMin/tMax por posicao (attack=ptr@0,
        // tMin@8, tMax@12). Sem FieldOff: offsets fixos da struct.
        ResolveClass("PlayerMovement", cMove);
        ResolveField(cMove, "PlayerMovement", "jumpSpeed", fMoveJump);
        // UnityEngine em modulos (PhysicsModule tem o Physics). Enumera
        // as imagens e pega a que tem UnityEngine.Transform.
        s_unity = UnityImageWithTransform();
        if (!s_unity) s_unity = pImgLoaded("UnityEngine.CoreModule");
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
            if (cTrans) ResolveMethod(cTrans, "Transform", "get_eulerAngles", 0, mGetEuler);
            if (cComp) ResolveMethod(cComp, "Component", "get_transform", 0, mGetTrans);
            if (cComp) ResolveMethod(cComp, "Component", "get_gameObject", 0, mGetGO);
            MonoClass* cObj = pClassFrom(s_unity, "UnityEngine", "Object");
            if (cObj) { s.resolvedClasses++; ResolveMethod(cObj, "Object", "get_name", 0, mGetName); }
            MonoClass* cRend = pClassFrom(s_unity, "UnityEngine", "Renderer");
            if (cRend) { s.resolvedClasses++; ResolveMethod(cRend, "Renderer", "get_bounds", 0, mGetBounds); }
            // Physics resolves lazily from its own assembly in aim_runtime.inl.
        }                     else Log::Warn("Imagem UnityEngine.CoreModule nao carregada.");

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
    // GetPos fora do orÃ§amento (posicao e dado vital: box/skeleton
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

    // invoke Transform.get_eulerAngles -> (x=pitch, y=yaw, z=roll) graus.
    // Usado pela mira em malha fechada: yaw atual da CameraTransform, sem
    // ancora, sem acumulo. SEH + sanidade finita inline (Fin esta declarado
    // abaixo; nao chamar daqui p/ nao quebrar a ordem de compilacao).
    static bool GetEulerY(void* trans, Vec3& out) {
        if (!mGetEuler || !trans) return false;
        __try {
            MonoObject* exc = nullptr;
            MonoObject* ret = pInvoke(mGetEuler, trans, nullptr, &exc);
            if (exc || !ret) return false;
            memcpy(&out, pUnbox(ret), sizeof(out));
            if (!(out.x == out.x && out.y == out.y && out.z == out.z)) return false;
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }

    void SetViewport(float w, float h) {
        if (w > 100 && h > 100) s_settings.Publish(RuntimeSettings::Capture(w,h));
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
            if (!(cw > Aim::NearPlane)) return false; // melee (~1m) ainda projeta; sanidade barra o lixo
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
            return out.z > Aim::NearPlane;
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
    // FIX Bug 3: array com SK_PHYS (18) entradas â€” iterar so ossos fisicos e
    // calcular HL2L/HL2R explicitamente (fix bugs 1-2: mGetRot + unificado 2D/3D).
    static const int kBoneIdx[SkJoint::SK_PHYS] = {
        12, 11, 10, 9, 8, 1, 2, 3, 4, 5, 6, 7, 13, 14, 15, 16, 17, 18
    };
    // Rotacao do Transform: get_rotation retorna QUATERNION (x,y,z,w â€” 16 bytes),
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

    #include "aim_runtime.inl"

    static void CollectJoints(void* zo, void* cam, EspEntry& out) {
        out.skN = SkJoint::SK_COUNT;
        for (int k = 0; k < SkJoint::SK_COUNT; ++k) { out.skV[k] = false; out.skX[k] = out.skY[k] = 0; }
        // Ciclo 2 (spec VISUAL Â§4.2): worker respeita s_options.bZombieSkeleton
        // (menu VISUAL manda; ReadLayout nao decide mais sozinho). Aliados usam
        // a mesma flag por enquanto (item 15 define a separacao).
        if (!s_options.bZombieSkeleton) { out.skN = 0; return; }
        void* arr = ReadP(zo, Off::ZO_armature);
        if (!arr) { out.skN = 0; return; }
        long long len = 0;
        __try { memcpy(&len, (char*)arr + Off::A_len, sizeof(len)); }
        __except (EXCEPTION_EXECUTE_HANDLER) { out.skN = 0; return; }
        // Boss tem rig diferente (outro tamanho/ordem): mapeia POR NOME
        // (head/neck/sp*/hl/sl/sr/a1*/a2*/l1*/l2*/fl/fr), igual a auditoria
        // [BONE] faz. Rig vazio (len<=0) = sem skeleton. Comum com 19 usa os
        // indices auditados abaixo (caminho rapido, sem strings).
        if (len <= 0) { out.skN = 0; return; }
        if (len != 19) {
            // Rig nao-padrao (boss): resolve cada junta pelo nome do Transform.
            // GetName via invoke 1x/junta (rig curto, poucas entidades boss).
            static int s_rigLenLogged = -1;
            if ((int)len != s_rigLenLogged) {
                s_rigLenLogged = (int)len;
                Log::Infof("[RIG] rig nao-padrao len=%d (mapeando por nome).", (int)len);
            }
            for (long long k = 0; k < len && k < 64; ++k) {
                void* bone = nullptr;
                __try { memcpy(&bone, (char*)arr + Off::A_data + (size_t)k * 8, 8); }
                __except (EXCEPTION_EXECUTE_HANDLER) { bone = nullptr; }
                if (!bone) continue;
                // Nome via Object.get_name no GameObject do Transform
                // (mesma tecnica da AuditBones; 1x/junta, so em rig nao-padrao).
                char nmB[64] = { 0 };
                const char* nm = nullptr;
                __try {
                    if (mGetGO && mGetName) {
                        MonoObject* exc = nullptr;
                        MonoObject* go = pInvoke(mGetGO, bone, nullptr, &exc);
                        if (!exc && go) {
                            MonoObject* exc2 = nullptr;
                            MonoObject* ret = pInvoke(mGetName, go, nullptr, &exc2);
                            if (!exc2 && ret) {
                                char* u = pStrUtf8(ret);
                                if (u) { strncpy_s(nmB, u, _TRUNCATE); pFree(u); nm = nmB; }
                            }
                        }
                    }
                } __except (EXCEPTION_EXECUTE_HANDLER) {}
                if (!nm || !nm[0]) continue;
                int dst = -1;
                if (!strcmp(nm, "head")) dst = SkJoint::SK_HEAD;
                else if (!strcmp(nm, "neck")) dst = SkJoint::SK_NECK;
                else if (!strcmp(nm, "sp3")) dst = SkJoint::SK_SP3;
                else if (!strcmp(nm, "sp2")) dst = SkJoint::SK_SP2;
                else if (!strcmp(nm, "sp1")) dst = SkJoint::SK_SP1;
                else if (!strcmp(nm, "hl")) dst = SkJoint::SK_HL;
                else if (!strcmp(nm, "sl")) dst = SkJoint::SK_SL;
                else if (!strcmp(nm, "sr")) dst = SkJoint::SK_SR;
                else if (!strcmp(nm, "a1l")) dst = SkJoint::SK_A1L;
                else if (!strcmp(nm, "a2l")) dst = SkJoint::SK_A2L;
                else if (!strcmp(nm, "a1r")) dst = SkJoint::SK_A1R;
                else if (!strcmp(nm, "a2r")) dst = SkJoint::SK_A2R;
                else if (!strcmp(nm, "l1l")) dst = SkJoint::SK_L1L;
                else if (!strcmp(nm, "l2l")) dst = SkJoint::SK_L2L;
                else if (!strcmp(nm, "fl")) dst = SkJoint::SK_FL;
                else if (!strcmp(nm, "l1r")) dst = SkJoint::SK_L1R;
                else if (!strcmp(nm, "l2r")) dst = SkJoint::SK_L2R;
                else if (!strcmp(nm, "fr")) dst = SkJoint::SK_FR;
                if (dst < 0) {
                    static char s_unk[8][32] = { 0 };
                    static int s_unkN = 0;
                    bool seen = false;
                    for (int u = 0; u < 8 && s_unk[u][0]; ++u)
                        if (!strcmp(s_unk[u], nm)) { seen = true; break; }
                    if (!seen) {
                        Log::Infof("[RIG] junta desconhecida: %s", nm);
                        if (s_unkN < 8) {
                            strncpy_s(s_unk[s_unkN], nm, _TRUNCATE);
                            s_unkN++;
                        }
                    }
                    continue;
                }
                Vec3 w, s3;
                if (!GetPos(bone, w)) continue;
                if (!Fin(w.x) || !Fin(w.y) || !Fin(w.z)) continue;
                if (!W2S(cam, w, s3)) continue;
                if (!Sane2(s3.x, s3.y)) continue;
                out.skX[dst] = s3.x; out.skY[dst] = s3.y; out.skV[dst] = true;
            }
            return; // rig nao-padrao: sem maos estimadas (indices de comum nao valem)
        }
        void* bones[SkJoint::SK_PHYS] = { nullptr };
        for (int k = 0; k < SkJoint::SK_PHYS; ++k) {
            __try { memcpy(&bones[k], (char*)arr + Off::A_data + (size_t)kBoneIdx[k] * 8, 8); }
            __except (EXCEPTION_EXECUTE_HANDLER) { bones[k] = nullptr; }
        }
        Vec3 wp[SkJoint::SK_PHYS];
        bool wok[SkJoint::SK_PHYS] = { false };
        // Causa A (item 14b): skeleton longe vira box 2D leve. 18 get_position
        // por zumbi x 96 = ~1700 invokes/ciclo â€” e o LOD mexe nesses mesmos
        // Transforms. O alcance segue o slider; o orcamento de invokes permanece.
        // A flag bZombieSkeleton continua mandando (respeita o menu).
        // Probe com distancia JA conhecida no ciclo (eye/foot do 2D ou center
        // da AABB do 3D) â€” nunca invoke extra (o probe com GetPos batia justo
        // no objeto mais fragil: armature se formando no spawn).
        if (s_skDist2 >= 0 && s_skDist2 > s_options.fEspDistance * s_options.fEspDistance) {
            out.skN = 0; return;
        }
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
        // Maos estimadas via rotacao do antebraco â€” unificado p/ 2D e 3D (fix bugs 1-4).
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
        s_aimCount = 0; s_camWok = false; s_aimForwardValid = false;
        if ((!s_options.bZombieEsp && !AimRequested()) || !mGetPos || !mW2S || !mGetTrans) { ClearEntitySnapshot(); return; }
        // MainCamera.instance (static) -> cam@32 (UnityEngine.Camera).
        // Re-resolve aqui (barato, 2Hz) para pegar a Camera viva.
        MonoClass* cMC = pClassFrom(s_img, "", "MainCamera");
        if (!cMC) { ClearEntitySnapshot(); return; }
        MonoClassField* fInst = pFieldFrom(cMC, "instance");
        if (!fInst) { ClearEntitySnapshot(); return; }
        void* mcObj = nullptr;
        if (!StaticInstance(cMC, fInst, mcObj)) { ClearEntitySnapshot(); return; }
        void* cam = ReadP(mcObj, Off::MC_cam);
        if (!cam) { ClearEntitySnapshot(); return; }
        // FIX P0-2 (crash em transicao de cena 15/09): valida o wrapper da camera
        // antes de qualquer invoke. Se a cena trocou (morte/troca de mapa), o
        // MainCamera.instance pode apontar p/ objeto destruido. Probe de 1 byte
        // com SEH: wrapper morto = AV capturado aqui, fora do JIT do Mono.
        { volatile char probe = 0;
          __try { memcpy((void*)&probe, cam, 1); }
          __except (EXCEPTION_EXECUTE_HANDLER) { ClearEntitySnapshot(); return; } }
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
            if (tr && !exc) {
                hasCamW = GetPos(tr, camW);
                MonoMethod* forward = pMethodFrom(cTrans, "get_forward", 0);
                MonoObject* value = forward ? static_cast<MonoObject*>(InvokeObj(forward, tr, nullptr)) : nullptr;
                __try {
                    if (value) { memcpy(&s_aimForward, pUnbox(value), sizeof(s_aimForward)); s_aimForwardValid = true; }
                } __except (EXCEPTION_EXECUTE_HANDLER) { s_aimForwardValid = false; }
            }
            PiAdd(s_piTrC, PiNow() - t0c, hasCamW);
        }
        s_camW = camW; s_camWok = hasCamW;
        if(!hasCamW || !Fin(camW.x) || !Fin(camW.y) || !Fin(camW.z)) {ClearEntitySnapshot();return;}
        float maxD = s_options.fEspDistance;
        float maxD2 = maxD * maxD;
        LARGE_INTEGER t0, t1;
        QueryPerformanceCounter(&t0);
        // OrÃ§amento do ciclo: reseta a cada BuildEsp. Sem orÃ§amento o skeleton
        // vira leitura barata (sem invoke) em vez de travar o jogo.
        s_budgetLeft = s_budgetMax;
        void* zl = nullptr;
        if (!cZLoader) { ClearEntitySnapshot(); return; }
        MonoClassField* fZL = pFieldFrom(cZLoader, "Instance");
        if (!fZL || !StaticInstance(cZLoader, fZL, zl)) { ClearEntitySnapshot(); return; }
        void* list = ReadP(zl, Off::ZL_zombies);
        static int gatherCursor = 0;
        const int gatherStart = gatherCursor;
        const long long gatherDeadline = PiNow() + 6000;
        WalkList(list, 512, [&](void* e, int index) {
            if (PiNow() >= gatherDeadline) return;
            gatherCursor = index + 1;
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
            CollectAimTarget(e, zo, cam, hp);
            if (!s_options.bZombieEsp) return;
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
            if (s_options.iZombieBox == 1 && mGetBounds) {
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
                    if (nv < 2) return; // partially clipped close body still has visible edges
                    // Maos ja calculadas em CollectJoints (fix bugs 1-2, 4) â€” vale p/ 2D e 3D.
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
                        bool inScr = Aim::IntersectsViewport(sh.x, sh.y, sf.x, sf.y, vw, vh);
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
        }, gatherStart);
        // Publish only a completed frame; failures publish an empty frame.
        // Telemetria de orcamento (1x/sessao): prova que o teto segura a horda.
        if (!s_budgetLogged) {
            s_budgetLogged = true;
            char bb[32] = { 0 };
            BudgetFmt(bb, sizeof(bb));
            Log::Infof("[BUDGET] ciclo invocacoes=%s entidades=%d.", bb, n);
        }
        {
            s_lastN = n > 128 ? 128 : n;
            EntitySnapshot snapshot;
            snapshot.count = s_lastN;
            s.espShown = snapshot.count;
            extern long s_ammoWritesExt();
            s.ammoWrites = (int)s_ammoWritesExt();
            for (int i = 0; i < snapshot.count; ++i) snapshot.entries[i] = tmp[i];
            s_entitySnapshot.Publish(snapshot);
        }
        // MÃ©trica de custo do ciclo (fora do lock).
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
    // worker dorme 500ms e tenta de novo â€” nunca invoca no escuro.
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
    static void RefreshSessionDebug() {
        static long long nextRefresh = 0;
        const long long now = PiNow();
        if (now < nextRefresh) return;
        nextRefresh = now + 1000000;

        void* multiplayer = nullptr;
        if (!StaticInstance(cMp, fMpInst, multiplayer) || !multiplayer) {
            s.coopMode = 0;
            strncpy_s(s.roomHost, sizeof(s.roomHost), "---", _TRUNCATE);
            strncpy_s(s.roomId, sizeof(s.roomId), "---", _TRUNCATE);
            return;
        }

        const bool single = mIsSingle && InvokeBool(mIsSingle, multiplayer);
        const bool online = mIsMulti && InvokeBool(mIsMulti, multiplayer);
        const bool client = mIsClient && InvokeBool(mIsClient, multiplayer);
        const bool server = mIsServer && InvokeBool(mIsServer, multiplayer);
        const int mode = single ? 1 : client ? 2 : (online && server) ? 3 : 0;
        if (mode != s.coopMode) {
            s.coopMode = mode;
            static const char* names[] = { "LOBBY", "SINGLE", "CLIENTE", "HOST" };
            Log::Infof("[MODE] modo=%s (single=%d multi=%d server=%d client=%d)", names[mode],
                single ? 1 : 0, online ? 1 : 0, server ? 1 : 0, client ? 1 : 0);
        }

        if (single) {
            strncpy_s(s.roomHost, sizeof(s.roomHost), "Solo", _TRUNCATE);
            strncpy_s(s.roomId, sizeof(s.roomId), "---", _TRUNCATE);
            return;
        }

        strncpy_s(s.roomHost, sizeof(s.roomHost), "---", _TRUNCATE);
        strncpy_s(s.roomId, sizeof(s.roomId), "---", _TRUNCATE);
        __try {
            void* lobbyCode = mGetLobbyCode ? InvokeObj(mGetLobbyCode, multiplayer, nullptr) : nullptr;
            char* code = lobbyCode ? pStrUtf8(static_cast<MonoObject*>(lobbyCode)) : nullptr;
            if (code) {
                strncpy_s(s.roomId, sizeof(s.roomId), code, _TRUNCATE);
                pFree(code);
            }
            void* lobby = nullptr;
            if (mLobbyGetHost && StaticInstance(cLobby, fLobbyInst, lobby)) {
                void* host = InvokeObj(mLobbyGetHost, lobby, nullptr);
                MonoObject* nameObject = host && fLobbyPlayerName ?
                    static_cast<MonoObject*>(ReadP(host, FieldOff(fLobbyPlayerName))) : nullptr;
                char* hostName = nameObject ? pStrUtf8(nameObject) : nullptr;
                if (hostName) {
                    strncpy_s(s.roomHost, sizeof(s.roomHost), hostName, _TRUNCATE);
                    for (char* ch = s.roomHost; *ch; ++ch)
                        if (static_cast<unsigned char>(*ch) < 0x20) *ch = ' ';
                    pFree(hostName);
                }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            strncpy_s(s.roomHost, sizeof(s.roomHost), "---", _TRUNCATE);
            strncpy_s(s.roomId, sizeof(s.roomId), "---", _TRUNCATE);
        }
    }

    static void RunGameCycle() {
        if (!s.ready || !SceneAlive()) {
            ClearEntitySnapshot(); ResetAim(); AmmoCachesClear();
            s.inMap = false; s_camWok = false;
            s.coopMode = 0;
            strncpy_s(s.roomHost, sizeof(s.roomHost), "---", _TRUNCATE);
            strncpy_s(s.roomId, sizeof(s.roomId), "---", _TRUNCATE);
            s_nextCycle = PiNow() + 500000;
            return;
        }
        if (MapSettling()) {
            ClearEntitySnapshot(); ResetAim();
            s_nextCycle = PiNow() + 500000;
            return;
        }
        RefreshSessionDebug();
        s.featureKeysAllowed=false;
        if(s_featureKeys)InvokeAimBool(s_featureKeys,nullptr,nullptr,s.featureKeysAllowed);
        BuildWorldEsp();
        BuildDistantEsp();
        static int s_deadN = 0;
        static int s_defN = 0; // contador p/ defesa rapida (God/Stamina todo ciclo)
        {
            // Defesa rapida: God/Stamina rodam TODO ciclo (~33ms), com ou sem
            // ESP ligado. Leitura barata (1 lista curta + 2 floats); escrita so
            // se a flag ligada E o valor caiu (custo zero no estado estavel).
            // Comeca rapido e so desacelera se o proprio ciclo ficar caro.
            // GATE NO MAPA (18/09): funcoes de escrita so rodam quando o jogador
            // esta dentro do mapa (gotLocal && localHp > 0). No menu/lobby,
            // SceneAlive pode retornar true (camera + ZombieLoader existem),
            // mas o player nao existe ainda. Sem gate = invoke em objeto nulo =
            // hang/crash reportado pelo operador.
            void* aimLocal = nullptr;
            bool wantDef = s.ready && (s_options.bDebugOverlay || HasPendingRestores() || s_options.bGodMode || s_options.bInfStamina || s_options.bInfAmmo || s_options.bInfItems || s_options.bInfMoney || s_options.bUnlockSlots || s_options.bUnlockLoadout || s_options.bNoRecoil || s_options.bNoSpread || s_options.bNoSway || s_options.bTightAim || s_options.bRapidFire || s_options.bFullAuto || s_options.bEnemyMagnet || s_options.bNoClip || s_options.bSpeedHack || s_options.bSuperJump || s_options.bRollSpeed || s_options.bFastKnife || s_options.bInstantReload || AimRequested());
            if (wantDef && SceneAlive()) {
                // Detecta se estamos dentro do mapa: precisa de player local vivo.
                bool inMap = false;
                void* pcs = nullptr;
                void* localEnt = nullptr;
                if (StaticInstance(cPlayers, fPCInst, pcs)) {
                    void* list = ReadP(pcs, Off::PCS_players);
                    int nPl = 0;
                    WalkList(list, 16, [&](void* e, int) {
                        nPl++;
                        if (!localEnt && InvokeBool(mHasLocal, e)) {
                            float hp = ReadF(e, Off::PM_healthFast);
                            if (hp > 0.0f && hp <= 100000.0f) {
                                localEnt = e;
                            }
                        }
                    });
                    inMap = (localEnt != nullptr && nPl >= 1);
                    static int s_lastNPl = -1;
                    if (nPl != s_lastNPl) {
                        if (s_lastNPl >= 0) {
                            Log::Infof("[COOP] transicao %d->%d players: aguardando cena.", s_lastNPl, nPl);
                            s_nextCycle = PiNow() + 500000;
                            AmmoCachesClear();
                            ClearEntitySnapshot(); ResetAim();
                            s_lastNPl = nPl;
                            return;
                        }
                        s_lastNPl = nPl;
                    }
                }
                s.inMap = inMap;
                static void* previousLocal = nullptr;
                if (previousLocal != localEnt) {
                    AmmoCachesClear();
                    previousLocal = localEnt;
                }
                // Funcoes de escrita SO dentro do mapa (gate).
                ApplyWeapon(inMap ? localEnt : nullptr);
                if (inMap && localEnt) {
                    bool coop = (s.coopMode == 2); // CLIENTE
                    if (s_options.bInfMoney) ApplyMoney();
                    ApplyDefense(localEnt);
                    if (s_options.bUnlockLoadout) ApplyLoadout();
                    if (s_options.bInfAmmo || s_options.bInfItems || s_options.bInstantReload) ApplyAmmo(localEnt, coop);
                    aimLocal = localEnt;
                }
                if (++s_defN >= 60) {
                    s_defN = 0;
                    ReadAll();
                    AuditBones();
                }
            }
            if (s.ready && (s_options.bZombieEsp || AimRequested())) {
                if (!SceneAlive()) {
                    // Cena morta/trocando (respawn): zera o snapshot e espera.
                    // TryEnter: se o Present estiver lendo, pula em vez de travar.
                    ClearEntitySnapshot(); s_lastN = 0; AmmoCachesClear();
                    if (++s_deadN == 1) Log::Warn("[SCENE] loader morto â€” worker em espera (respawn?).");
                    ResetAim(); s_camWok = false;
                    s_nextCycle = PiNow() + 500000;
                    return;
                }
                if (s_deadN > 0) { s_deadN = 0; Log::Info("[SCENE] loader vivo â€” worker retomada."); }
                // AUDITORIA 16/09 (hang 02:44, PERF 1438ms): BuildEsp inteiro
                // rodava no MESMO ciclo. Com 86 zumbis, o ciclo estourava 1.4s
                // e o ritmo adaptativo so reagia DEPOIS.
                // Novo ritmo: posicao+skeleton TODO ciclo (barato, ~2ms).
                BuildEsp();
                WohaxAim(aimLocal);
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
            else { s_entitySnapshot.Publish(EntitySnapshot{}); ClearDistantEsp(); ResetAim(); s_camWok = false; }
        }
    }

    // ========================================================================
    // AMMO â€” subsistema de municao infinita (reescrito 22/09, revisao senior).
    //
    // MODELO DO JOGO (IL auditado via dnlib â€” nao mexer sem re-auditar):
    // - InventoryItem eh CLASSE (MonoObject + 16B header). Campos via FieldOff:
    //   id / stackCount / ammo. Regra: stackMax==1 -> usa .ammo (arma);
    //   senao usa .stackCount (pilha). Get/SetGenericNumericValue confirmam.
    // - HUD: EquipmentHUDAmmo.Show le item.ammo (pente) +
    //   PlayerInventory.StoredItemCount(ammoID) (reserva = soma stackCount
    //   das pilhas no STORAGE com id == ammoID).
    // - ReloadGun: need=maxAmmo-ammo; pulled=PullStoredItems(ammoID,need).
    // - Unload (botao): GotLootFromServer(ammoID, ammo) + item.ammo=0.
    //   Cria pilha no storage via caminho de loot do servidor.
    // - AddItem(item, filter): LootPlacingFilter enum INT (0=Inventory,
    //   1=Equipment). filter=0 TRAVA (hang 21/09); filter=1 validado via CE.
    // - CreateInventoryItem(ID,int) eh STATIC (obj=null).
    // - mono_runtime_invoke: args = array de PONTEIROS PARA O VALOR
    //   (void* cargs[] = { &id, &dose }); retorno bool = unbox int8.
    // - selectedItem: EquipmentIndex STRUCT { SetType:int@+0, Value:int@+4 }
    //   em arms+sel. SetType: 0=maos 1=arma 2=misc.
    //
    // ARQUITETURA (regras duras):
    // - Ciclo LEVE (todo tick, ~30Hz): so memcpy/ReadI/WriteI + SEH.
    //   ZERO invoke. Pente (ammo=maxAmmo) + reserva (stackCount=stackMax).
    // - Ciclo PESADO (1x/4 ticks): resolve db novo (DbCached) + ammoID da
    //   equipada + seed de pilha (1 invoke max por tipo, com backoff).
    // - Seed de pilha: GotLootFromServer(ammoID, stackMax) â€” o MESMO caminho
    //   do botao Descarregar. Sem pilha no storage = reserva 0 (correto pelo
    //   modelo do jogo); o seed cria 1x por tipo.
    // - Troca de arma: s_ammoIdCur segue o tipo novo; tipo antigo para de
    //   travar (gasta normal = "descarta" sozinho).
    // - Troca de cena: AmmoCachesClear (ponteiros morrem no GC).
    // ========================================================================
    static bool s_ammoOk = true;
    static long s_ammoWrites = 0; // telemetria: escritas no pente (debug overlay)
    long s_ammoWritesExt() { return s_ammoWrites; }
    static int s_ammoMode = 0; // 0=desconhecido 1=invoke 2=lista (legado; hoje=lista)
    static int s_ammoIdCur = -1; // ammoID da arma equipada (reserva segue ele)
    static int s_ammoIds[8] = { -1,-1,-1,-1,-1,-1,-1,-1 }; // tipos vistos (seed 1x)
    // Estado do seed: 1 pilha por tipo (definicao unica; Clear usa abaixo).
    static bool s_pileMade[8] = { false,false,false,false,false,false,false,false };
    static int s_pileTries[8] = { 0,0,0,0,0,0,0,0 };
    // Limpa caches de item/db/ammoID (troca de cena: ponteiros morrem no GC).
    static void AmmoCachesClear() {
        ResetRestoreCaches();
        for (int k = 0; k < 64; ++k) {
            s_dbCache[k].item = nullptr;
            s_dbCache[k].db = nullptr;
            s_dbCache[k].tick = 0;
        }
        s_ammoIdCur = -1;
        for (int k = 0; k < 8; ++k) { s_ammoIds[k] = -1; s_pileMade[k] = false; s_pileTries[k] = 0; }
    }
    static bool AmmoIdKnown(int id) {
        if (id < 0) return false;
        for (int k = 0; k < 8; ++k) if (s_ammoIds[k] == id) return true;
        return false;
    }
    static void AmmoIdAdd(int id) {
        // Faixa do enum InventoryItem/ID: 2..116 (auditoria 22/09: o miss-log
        // provou que balas reais usam aid 9 (fuzil), 0 e 1 â€” a faixa 10-116
        // as descartava em silencio, por isso so a pistola ganhava reserva).
        if (id < 2 || id > 116 || AmmoIdKnown(id)) return;
        for (int k = 0; k < 8; ++k) {
            if (s_ammoIds[k] < 0) {
                s_ammoIds[k] = id;
                Log::Infof("[AMMO] ammoID +%d (tipo %d)", id, k);
                break;
            }
        }
    }
    // Cria pilha do tipo quando zerada (pente extra no limite do jogo).
    // Via CreateInventoryItem(id,30)+AddItem(Both). LootPlacingFilter real
    // (enum byte): 0=Inventory, 1=Equipment, 2=Both â€” Both tenta os dois.
    // Retry LIMITADO: 3 tentativas por tipo por sessao (cada AddItem sem lugar
    // faz o proprio jogo dropar no chao â€” retry infinito = tapete de loot).
    // Falhou 3x = para e avisa (libere slot ou Desbloquear Slots).
    // Estado do seed: 1 pilha por tipo (slot indexado pelo s_ammoIds).
    // (s_pileMade/s_pileTries declarados acima, antes do AmmoCachesClear.)
    static int s_pileCount[8] = { 0,0,0,0,0,0,0,0 }; // pilhas no storage (TopStacks)
    static int s_pileSmax[8] = { 0,0,0,0,0,0,0,0 }; // stackMax por tipo
    static int s_pileBackN[8] = { 0,0,0,0,0,0,0,0 }; // backoff por slot (~2s)
    static void PileTriesReset(const char* why) {
        for (int k = 0; k < 8; ++k) { s_pileTries[k] = 0; s_pileMade[k] = false; }
        Log::Infof("[AMMO] retry liberado (%s).", why);
    }
    static bool s_slotsWasOn = false; // borda de subida de UnlockSlots
    // Garante 1 pilha cheia do tipo no storage (seed = caminho Descarregar).
    // Chamada SO no ciclo pesado, SO se !hasPilha. 1 invoke por chamada max,
    // com backoff de ~2s por slot e limite de 3 tentativas.
    static void AmmoEnsurePile(void* pinv, int id, int smax) {
        if (!pinv || id <= 0) return;
        if (s_options.bUnlockSlots && !s_slotsWasOn) {
            s_slotsWasOn = true;
            PileTriesReset("slots ligados");
        } else if (!s_options.bUnlockSlots) {
            s_slotsWasOn = false;
        }
        int slot = -1;
        for (int k = 0; k < 8; ++k) if (s_ammoIds[k] == id) slot = k;
        if (slot < 0 || s_pileMade[slot]) return;
        if (s_pileTries[slot] >= 3) return;
        // Resolve 1x (fora do backoff â€” resolve nao invoca, nao trava).
        if (!mGotLoot) {
            static bool s_gotInit = false;
            if (!s_gotInit) {
                s_gotInit = true;
                MonoClass* cInter = nullptr;
                if (ResolveClass("PlayerInteraction", cInter) && cInter)
                    ResolveMethod(cInter, "PlayerInteraction", "GotLootFromServer", 2, mGotLoot);
                if (!mGotLoot) Log::Warn("[AMMO] sem GotLootFromServer (seed nativo off).");
            }
        }
        if (!mCreateItem && cItem) {
            MonoMethod* t = pMethodFrom(cItem, "CreateInventoryItem", 2);
            if (t) { mCreateItem = t; s.resolvedMethods++; }
        }
        if (!mAddItem && cPInv) {
            MonoMethod* t = pMethodFrom(cPInv, "AddItem", 2);
            if (t) { mAddItem = t; s.resolvedMethods++; }
        }
        // Backoff POR SLOT: 1 tentativa a cada ~2s. Fora da vez = zero invoke.
        if (++s_pileBackN[slot] % 60 != 1) return;
        __try {
            s_pileTries[slot]++;
            // Dose = stackMax real (reserva do HUD soma stackCount).
            int dose = (smax > 0 && smax <= 100000) ? smax : 30;
            bool ok = false;
            // Via 1: GotLootFromServer (caminho do botao Descarregar).
            static MonoClassField* fInter = nullptr;
            static bool s_interInit = false;
            if (!s_interInit) {
                s_interInit = true;
                if (cPlayer) fInter = pFieldFrom(cPlayer, "interaction");
            }
            // Via 1: GotLootFromServer no PlayerInteraction do local.
            // interaction mora no PlayerMain: sobe pinv->playerMain->interaction.
            if (mGotLoot) {
                static MonoClassField* fPMain = nullptr;
                static bool s_pmInit = false;
                if (!s_pmInit) {
                    s_pmInit = true;
                    if (cPInv) fPMain = pFieldFrom(cPInv, "playerMain");
                }
                void* pm = fPMain ? ReadP(pinv, FieldOff(fPMain)) : nullptr;
                void* inter = (pm && fInter) ? ReadP(pm, FieldOff(fInter)) : nullptr;
                if (inter) {
                    int idArg = id, doseArg = dose;
                    void* gargs[2] = { &idArg, &doseArg };
                    MonoObject* gexc = nullptr;
                    __try { pInvoke(mGotLoot, inter, gargs, &gexc); }
                    __except (EXCEPTION_EXECUTE_HANDLER) { gexc = (MonoObject*)1; }
                    ok = (gexc == nullptr);
                    if (ok) Log::Infof("[AMMO] seed nativo id=%d qtd=%d (GotLoot).", id, dose);
                }
            }
            // Via 2 (fallback): CreateInventoryItem(static) + AddItem(filter=1).
            // filter: enum INT (0=Inventory trava! 1=Equipment validado via CE).
            if (!ok && mCreateItem && mAddItem) {
                int idArg = id, doseArg = dose;
                void* cargs[2] = { &idArg, &doseArg };
                MonoObject* exc = nullptr;
                MonoObject* ret = pInvoke(mCreateItem, nullptr, cargs, &exc);
                if (exc || !ret) { Log::Warn("[AMMO] CreateInventoryItem falhou."); return; }
                void* item = ret;
                int filter = 1;
                void* aargs[2] = { &item, &filter };
                MonoObject* exc2 = nullptr;
                MonoObject* ret2 = pInvoke(mAddItem, pinv, aargs, &exc2);
                ok = (!exc2 && ret2 && *(unsigned char*)pUnbox(ret2) != 0);
            }
            if (ok) {
                // VERIFICA no storage: conta pilhas do tipo AGORA. So marca
                // com pilha visivel no storage (HUD conta). Fora = tenta de novo.
                int found = 0;
                {
                    void* cont = ReadP(pinv, FieldOff(fStorage));
                    void* ls = cont ? ReadP(cont, FieldOff(fItems)) : nullptr;
                    int oS = FieldOff(fStack), oI = FieldOff(fId);
                    if (ls && oS >= 0 && oI >= 0) WalkList(ls, 64, [&](void* it, int) {
                        if (ReadI(it, oI, -1) == id) found++;
                    });
                }
                if (found > 0) {
                    s_pileMade[slot] = true;
                    Log::Infof("[AMMO] pilha criada: id=%d qtd=%d no storage (HUD conta).", id, dose);
                } else {
                    Log::Infof("[AMMO] pilha id=%d aceita mas fora do storage (tentativa %d/3).", id, s_pileTries[slot]);
                }
            } else {
                Log::Infof("[AMMO] pilha id=%d recusada (tentativa %d/3).", id, s_pileTries[slot]);
                if (s_pileTries[slot] >= 3)
                    Log::Warn("[AMMO] sem espaco p/ pilha (3 tentativas) â€” libere slot ou Desbloquear Slots. Nao tenta mais.");
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }
    static void ApplyAmmo(void* local, bool coop) {
        if (!local || !s_ammoOk) return;
        // Resolve preguiÃ§oso dos offsets (1x; API pode indisponivel no Unity 6).
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
                Log::Warn("[AMMO] cadeia incompleta â€” municao infinita desativada (sem crash).");
                s_ammoOk = false;
                s.ammoOk = false; // debug overlay mostra AMMO:OFF
                return;
            }
            s.ammoOk = true;
        }
        if (oInv < 0) return; // ja desativado acima
        s_dbTick++; // ciclo novo: cache de db expira apos 300 ciclos
        // Throttle de leitura pesada (anti-hang 18/09): o bloco de ammo faz
        // invokes por item (GetDb/TopAmmo); rodar a 30Hz trava o Mono em
        // transicao de mapa. Leitura pesada 1x/4 ciclos; escrita de pente
        // (2 memcpy+SEH, sem invoke) continua todo ciclo.
        // s_ammoSlowN conta; pesado quando %4==0.
        static int s_ammoSlowN = 0;
        bool heavy = ((++s_ammoSlowN & 3) == 0);
        __try {
            void* pinv = ReadP(local, oInv);
            if (!pinv) return;
            void* peq = ReadP(pinv, oEq);
            if (!peq) return;
            // Coop cliente: pente LIVRE (host valida o dano; travar gera
            // pacote inconsistente e o tiro/dinamite nao conta). So identifica
            // o ammoID (leitura) e garante reserva (abaixo) â€” fluxo legitimo.
            // Recarga automatica: ammo==0 -> TryStartReload (fluxo normal do
            // jogo: PullStoredItems da reserva cheia; host aceita, dano conta).
            // Pente extra ao ativar: ao ligar bInfAmmo (borda de subida),
            // garante 1 pilha cheia p/ CADA arma em weapons (nao espera zerar).
            bool lockMag = !coop && s_options.bInfAmmo;
            static bool s_ammoWasOn = false;
            if (s_options.bInfAmmo && !s_ammoWasOn) {
                s_ammoWasOn = true;
                // Reset 1x: permite recriar pilhas gastas na sessao anterior.
                for (int k = 0; k < 8; ++k) s_pileMade[k] = false;
                Log::Info("[AMMO] ativado: garantindo 1 pilha por arma.");
            } else if (!s_options.bInfAmmo) {
                s_ammoWasOn = false;
            }
            // selectedItem REAL (auditoria CE MCP 19/09, corrigida 19/09):
            // EquipmentIndex eh STRUCT { SetType:int@+0, Value:int@+4 }
            // a partir de arms+oSel (oSel=144). SetType: 0=NonEquippable
            // 1=Weapon 2=Misc. Os offsets 16/20 do CE sao absolutos a partir
            // do inicio do OBJETO Mono (header 16 bytes); relativo ao campo
            // (que o FieldOff retorna) eh +0/+4. So usa Value como indice de
            // weapons[] quando SetType==1 (Weapon).
            static int oSelSet = 0, oSelVal = 4; // relativo ao campo (+0/+4)
            {
                static bool s_eiLogged = false;
                if (!s_eiLogged) {
                    s_eiLogged = true;
                    Log::Info("[AMMO] EquipmentIndex: SetType=+0 Value=+4 (rel. arms+sel).");
                }
            }
            // Le a equipada REAL: seleciona arms, valida SetType==Weapon(1),
            // indice Value -> weapons[Value]. Retorna o item ou nullptr.
            // Sem invoke: memcpy+SEH (barato, todo ciclo).
            auto EquippedReal = [&](void* armsPtr, void* peqPtr) -> void* {
                if (!armsPtr || !peqPtr || oSel < 0) return nullptr;
                int st = -1, vv = -1;
                __try {
                    memcpy(&st, (char*)armsPtr + oSel + oSelSet, sizeof(st));
                    memcpy(&vv, (char*)armsPtr + oSel + oSelVal, sizeof(vv));
                } __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
                if (st != 1) return nullptr; // nao eh arma (maos livres/misc)
                if (vv < 0 || vv > 32 || oWeapons < 0) return nullptr;
                void* list = ReadP(peqPtr, oWeapons);
                if (!list) return nullptr;
                int n = 0;
                __try { memcpy(&n, (char*)list + Off::L_size, sizeof(n)); }
                __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
                if (vv >= n) return nullptr;
                void* arr = ReadP(list, Off::L_items);
                void* item = nullptr;
                __try {
                    if (arr) memcpy(&item, (char*)arr + Off::A_data + (size_t)vv * 8, 8);
                } __except (EXCEPTION_EXECUTE_HANDLER) {}
                return item;
            };
            void* armsC = (oArms >= 0) ? ReadP(local, oArms) : nullptr;
            // Telemetria: mostra a equipada real 1x por combinacao
            // (SetType/Value/item/ammo/id). Muda ao trocar de arma.
            {
                static int s_eqSt = -99, s_eqVv = -99;
                if (armsC && peq && oSel >= 0) {
                    int st = -9, vv = -9;
                    __try {
                        memcpy(&st, (char*)armsC + oSel + oSelSet, sizeof(st));
                        memcpy(&vv, (char*)armsC + oSel + oSelVal, sizeof(vv));
                    } __except (EXCEPTION_EXECUTE_HANDLER) {}
                    if (st != s_eqSt || vv != s_eqVv) {
                        s_eqSt = st; s_eqVv = vv;
                        void* it = EquippedReal(armsC, peq);
                        int am = it ? ReadI(it, oAmmo, -9) : -9;
                        Log::Infof("[AMMO-EQ] SetType=%d Value=%d item=0x%p ammo=%d",
                            st, vv, it, am);
                    }
                }
            }
            static int s_coopNoReloadWhy = 0;
            // COOP CLIENTE 23/09 (reload infinito): como cliente, o pente fica
            // LIVRE (lockMag=false) e a reserva trava cheia. Se o pente zera e
            // chamamos TryStartReload em loop, o host pode nunca confirmar
            // (Pull sem sync) e o jogo entra em recarga eterna. Por isso: 1
            // recarga por zerada (borda de subida), max 3x, depois para de
            // insistir (a reserva cheia garante a proxima recarga manual com R).
            static long long s_lastReload = 0;
            static int s_reloadN = 0;
            static bool s_wasZero = false;
            if (coop && s_options.bInfAmmo && armsC && oAmmo >= 0) {
                // Coop cliente: pente vazio na equipada REAL -> TryStartReload.
                void* itemC = EquippedReal(armsC, peq);
                int ammoC = itemC ? ReadI(itemC, oAmmo, -1) : -1;
                bool isZero = (itemC && ammoC == 0);
                if (isZero && !s_wasZero) { s_reloadN = 0; } // zerou de novo: libera
                s_wasZero = isZero;
                if (isZero && s_reloadN < 3) {
                    // Pente vazio: recarrega pelo fluxo do jogo (1x por zerada).
                    long long nowR = PiNow();
                    if (!mTryReload && s_coopNoReloadWhy != 1) {
                        s_coopNoReloadWhy = 1;
                        Log::Warn("[AMMO-COOP] sem TryStartReload (nao resolvido).");
                    }
                    if (mTryReload && nowR - s_lastReload > 3000000LL) {
                        s_lastReload = nowR;
                        s_reloadN++;
                        __try {
                            MonoObject* excR = nullptr;
                            pInvoke(mTryReload, armsC, nullptr, &excR);
                            Log::Infof("[AMMO-COOP] recarga %d/3 (pente vazio).", s_reloadN);
                        } __except (EXCEPTION_EXECUTE_HANDLER) {}
                    }
                }
            }
            // Pente extra REAL (19/09, regra do ReloadGun): a reserva do HUD
            // e StoredItemCount(ammoID da equipada) â€” soma dos stackCount das
            // pilhas no STORAGE com id == ammoID. Entao: garante 1 pilha cheia
            // (stackCount=stackMax) do tipo da EQUIPADA no storage.
            // Sem Create/AddItem (sem invoke = sem hang/crash): escreve direto
            // stackCount=stackMax na 1a pilha do tipo; se nao existe pilha,
            // converte 1 slot vazio? Nao â€” sem slot livre, so loga (o loot
            // natural cria a pilha e a trava assume).
            // Eleicao da arma EM USO (22/09, sem selectedItem):
            // E1 gasto observado (tiro = ammo caiu) > E2 varredura weapons[].
            // selectedItem (EquippedReal) virou fonte opcional, nunca gate.
            // Ciclo leve registra ammoCur por indice; pesado elege + semeia.
            static int oAmmoId = -2;
            if (oAmmoId == -2) {
                oAmmoId = -1;
                if (cDbGun) {
                    MonoClassField* f = pFieldFrom(cDbGun, "ammoID");
                    oAmmoId = FieldOff(f);
                }
            }
            // E2: varredura weapons[] â€” registra TODOS os tipos de bala das
            // armas (db valido, stackMax==1). Funciona sem tiro e sem selectedItem.
            // ANTI-HANG 22/09: roda 1x a cada ~4s (pesadoConta), NAO todo pesado.
            // No loading (mapa assentando), items ainda nao tem db registrado e
            // cada DbCached miss = 1 invoke; 4 armas x invokes seguidos no frame
            // de carga = deadlock com o loader. Espacar resolve.
            // E1 (gasto/tiro) continua todo pesado: so ReadI, zero invoke.
            static int s_e2div = 0;
            bool e2vez = ((++s_e2div & 15) == 0); // 1x/16 pesados (~4s)
            if (heavy && oAmmoId >= 0 && mGetDb && oWeapons >= 0) {
                void* wlist = ReadP(peq, oWeapons);
                // E1 primeiro (barato, sem invoke): detecta tiro por queda de ammo.
                static int s_prevAmmo[64] = { -2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2 };
                if (wlist) WalkList(wlist, 16, [&](void* it, int idx) {
                    if (idx < 0 || idx >= 64) return;
                    int cur = ReadI(it, oAmmo, -1);
                    if (cur < 0 || cur > 100000) return;
                    if (s_prevAmmo[idx] != -2 && cur < s_prevAmmo[idx] && s_ammoIdCur >= 2) {
                        // Tiro: arma idx gastou. Descobre o tipo SEM invoke
                        // (cache db); sem cache, o E2 resolve no proximo ciclo.
                        void* db0 = DbCached(it);
                        if (db0) {
                            int aid0 = ReadI(db0, oAmmoId, -1);
                            if (aid0 >= 2 && aid0 <= 116 && s_ammoIdCur != aid0) {
                                int old = s_ammoIdCur;
                                s_ammoIdCur = aid0;
                                Log::Infof("[AMMO-ELECT] tiro na arma slot=%d (ammoID %d->%d).", idx, old, aid0);
                            }
                        }
                    }
                    s_prevAmmo[idx] = cur;
                });
                // E2 (com invoke, espacado): descobre tipos novos 1x/~4s.
                // ROUND-ROBIN 22/09: 1 arma por vez (e2cursor) â€” 4 invokes no
                // mesmo ciclo travavam; 1 invoke/ciclo nao trava.
                // MISS-LOG 22/09 (auditoria: slots 0/1/3 descartados em
                // silencio): 1 linha por visita (ja espacada, sem spam).
                static int s_e2cursor = 0;
                if (e2vez && wlist) {
                    int nW = 0;
                    __try { memcpy(&nW, (char*)wlist + Off::L_size, sizeof(nW)); }
                    __except (EXCEPTION_EXECUTE_HANDLER) { nW = 0; }
                    if (nW > 0 && nW <= 16) {
                        int idx = s_e2cursor % nW;
                        s_e2cursor++;
                        void* arr = ReadP(wlist, Off::L_items);
                        void* it = nullptr;
                        __try {
                            if (arr) memcpy(&it, (char*)arr + Off::A_data + (size_t)idx * 8, 8);
                        } __except (EXCEPTION_EXECUTE_HANDLER) {}
                        if (!it) {
                            Log::Infof("[AMMO-E2] slot=%d it=null (vazio/bloqueado).", idx);
                        } else {
                            int cur = ReadI(it, oAmmo, -1);
                            void* db = DbCached(it);
                            int sm = db ? ReadI(db, FieldOff(fDbStack), -9) : -99;
                            int aid = (db && oAmmoId >= 0) ? ReadI(db, oAmmoId, -9) : -99;
                            if (cur >= 0 && cur <= 100000 && db && sm == 1 && aid >= 2 && aid <= 116) {
                                if (!AmmoIdKnown(aid))
                                    Log::Infof("[AMMO-CAND] arma slot=%d ammoID=%d (tipo novo).", idx, aid);
                                AmmoIdAdd(aid);
                            } else {
                                Log::Infof("[AMMO-E2] slot=%d cur=%d db=0x%p sm=%d aid=%d (descartado).",
                                    idx, cur, db, sm, aid);
                            }
                        }
                    }
                }
                // Sem tiro ainda: primeiro tipo registrado vira o ativo
                // (fail-open: semeia e trava por ele ate o tiro eleger outro).
                if (s_ammoIdCur < 2 && s_ammoIds[0] >= 2) {
                    s_ammoIdCur = s_ammoIds[0];
                    Log::Infof("[AMMO-ELECT] sem tiro: ativo=%d (primeiro tipo).", s_ammoIdCur);
                }
            }
            // Pente: trava ammo=maxAmmo em TODAS as armas de weapons[]
            // (fail-open: sem selectedItem confiavel, trava todas â€” custo de
            // 2-4 escritas/ciclo, sem invoke). TopAmmo so escreve em ARMA com
            // db valido (stackMax==1, max plausivel); pilha nunca entra aqui.
            // ANTI-HANG 22/09: TopAmmo com db NAO cacheado invoca (miss). No
            // loading isso deadlocka. So trava pente com db JA cacheado:
            // 1o ciclo apos ativar = so registra (E2 espaÃ§ado resolve o db).
            // Excecao: ciclo leve apos e2vez (db acabou de resolver).
            static bool s_dbPrimed = false;
            if (e2vez) s_dbPrimed = true;
            if (lockMag && s_dbPrimed && oWeapons >= 0) {
                void* wlist = ReadP(peq, oWeapons);
                if (wlist) WalkList(wlist, 16, [&](void* it, int) {
                    TopAmmo((void*)it, oAmmo, oMax);
                });
            }
            // Offsets de storage (declarado antes do Instant Reload p/ compilar).
            // + save/load de SetGenericNumericValue (pilha da reserva topa no teto).
            static int oStorage = -2, oItems = -2, oStack = -2, oDbStack = -2, oId = -2;
            if (oStorage == -2) {
                oStorage = FieldOff(fStorage); oItems = FieldOff(fItems);
                oStack = FieldOff(fStack); oDbStack = FieldOff(fDbStack);
                oId = FieldOff(fId);
            }
            static int s_irHits = 0, s_irSkipDb = 0, s_irSkipFull = 0, s_irSkipRes = 0;
            static int s_irSkipSm = 0, s_irSkipMax = 0;
            // INSTANT RELOAD 23/09 (checkbox da aba PLAYER/Weapon): PENTE SEMPRE
            // CHEIO â€” igual aos grandes cheats (nunca zera, nunca recarrega).
            // Modelo: se cur < max, completa na hora puxando da reserva
            // (need=Pull, igual ao ReloadGun mas sem timer/animacao). Como a
            // reserva e infinita (TopStacks trava no teto), o pulled sempre
            // cobre o need = pente nunca esvazia = sem animacao de recarga.
            // CLIENTE 23/09: funciona em todos os modos. Como cliente o host
            // valida o DANO (halt no ShootGun seria pacote inconsistente), mas
            // completar o pente local + consumir a reserva local e exatamente
            // o que o ReloadGun legitimo faria â€” o sync leva o estado final e
            // o host aceita (igual ao TryStartReload, sem divergencia).
            // DIAG 23/09: loga 1x o estado das condicoes p/ achar gate que barra.
            {
                static bool s_irDiag = false;
                if (s_options.bInstantReload && !s_irDiag) {
                    s_irDiag = true;
                    Log::Infof("[INSTANT] on: coop=%d armsC=0x%p primed=%d oWep=%d oMax=%d bInfAmmo=%d",
                        coop ? 1 : 0, armsC, s_dbPrimed ? 1 : 0, oWeapons, oMax, s_options.bInfAmmo ? 1 : 0);
                }
                if (!s_options.bInstantReload) s_irDiag = false;
            }
            if (s_options.bInstantReload && oWeapons >= 0 && oMax >= 0) {
                void* wlist = ReadP(peq, oWeapons);
                if (wlist) WalkList(wlist, 16, [&](void* it, int) {
                    int cur = ReadI(it, oAmmo, -1);
                    // Pente ZERADO tambem completa (0->max direto, sem animacao).
                    // Antes pulava cur<=0 e caia no reload normal (que nao vinha).
                    if (cur < 0 || cur > 100000) return;
                    void* db = DbCached(it);
                    if (!db) { s_irSkipDb++; return; }
                    int sm = ReadI(db, FieldOff(fDbStack), -1);
                    if (sm != 1) { s_irSkipSm++; return; } // so arma
                    int max = ReadI(db, oMax, -1);
                    if (max <= 0 || max > 100000) { s_irSkipMax++; return; }
                    if (cur >= max) { s_irSkipFull++; return; }
                    // need=quanto falta; pulled=min(need, reserva do tipo).
                    int aid = (oAmmoId >= 0) ? ReadI(db, oAmmoId, -1) : -1;
                    int reserve = 0;
                    if (aid >= 2 && aid <= 116 && oStorage >= 0 && oItems >= 0 && oId >= 0 && oStack >= 0) {
                        void* cont = ReadP(pinv, oStorage);
                        void* ls = cont ? ReadP(cont, oItems) : nullptr;
                        if (ls) WalkList(ls, 64, [&](void* p, int) {
                            if (ReadI(p, oId, -1) == aid) {
                                int sc = ReadI(p, oStack, 0);
                                if (sc > 0) reserve += sc;
                            }
                        });
                    }
                    int need = max - cur;
                    int pulled = (reserve < need) ? reserve : need;
                    if (pulled <= 0) { s_irSkipRes++; return; }
                    // Consome da 1a pilha do tipo (igual ao PullStoredItems).
                    int left = pulled;
                    if (oStorage >= 0 && oItems >= 0 && oId >= 0 && oStack >= 0) {
                        void* cont = ReadP(pinv, oStorage);
                        void* ls = cont ? ReadP(cont, oItems) : nullptr;
                        if (ls) WalkList(ls, 64, [&](void* p, int) {
                            if (left <= 0) return;
                            if (ReadI(p, oId, -1) != aid) return;
                            int sc = ReadI(p, oStack, 0);
                            if (sc <= 0) return;
                            int take = (sc < left) ? sc : left;
                            WriteI(p, oStack, sc - take);
                            left -= take;
                        });
                    }
                    WriteI(it, oAmmo, cur + (pulled - left));
                    if (pulled - left > 0) { s_ammoWrites++; s_irHits++; }
                });
                // Telemetria 1x/5s: diz se o instant roda e onde trava.
                static long long s_irLogT = 0;
                {
                    long long now = PiNow();
                    if (now - s_irLogT > 5000000LL) {
                        s_irLogT = now;
                        Log::Infof("[INSTANT] hits=%d skipDb=%d skipFull=%d skipRes=%d skipSm=%d skipMax=%d",
                            s_irHits, s_irSkipDb, s_irSkipFull, s_irSkipRes, s_irSkipSm, s_irSkipMax);
                    }
                }
            }
            // Reserva (bInfAmmo) + pilhas gerais (bInfItems). Offsets resolvidos
            // acima (antes do Instant Reload). HUD = StoredItemCount(local).
            if (s_options.bInfAmmo) {
                TopStacks(local, oInv, oStorage, oItems, oStack, oDbStack, oId, false);
                // Seed: 1 pilha cheia POR TIPO REGISTRADO (todas as armas de
                // weapons[], nao so a ativa). Cada tipo sem pilha no storage
                // ganha 1 seed (round-robin: 1 tipo por ciclo pesado, com o
                // backoff interno do EnsurePile). Trocar de arma = tipo novo
                // ja tem pilha cheia esperando; tipo antigo para de travar
                // (TopStacks so trava o ativo + balas genericas 10-13/108).
                if (heavy) {
                    for (int k = 0; k < 8; ++k) {
                        int tid = s_ammoIds[k];
                        if (tid < 2 || tid > 116) continue;
                        if (s_pileMade[k]) continue; // tipo ja semeado
                        int smax = (s_pileSmax[k] > 0 && s_pileSmax[k] <= 100000)
                            ? s_pileSmax[k] : 200;
                        bool has = false;
                        {
                            void* cont = ReadP(pinv, oStorage);
                            void* ls = cont ? ReadP(cont, oItems) : nullptr;
                            if (ls && oId >= 0) WalkList(ls, 64, [&](void* it, int) {
                                if (has) return;
                                if (ReadI(it, oId, -1) == tid) has = true;
                            });
                        }
                        if (has) { s_pileMade[k] = true; continue; }
                        AmmoEnsurePile(pinv, tid, smax);
                        break; // 1 seed por ciclo pesado (backoff seguro)
                    }
                }
            }
            if (s_options.bInfItems) {
                // RELATORIO 25/09 (pedido do operador): ao ativar, lista 1x no
                // log cada pilha travada (id + stack atual/teto). So na borda.
                TopStacks(local, oInv, oStorage, oItems, oStack, oDbStack, oId, true);
                ItemsReport(local, oInv, oStorage, oItems, oStack, oDbStack, oId);
            } else {
                // Reset: desligou = proxima ativacao relata de novo.
                ItemsReportReset();
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // Trava item.ammo = DatabaseGun.maxAmmo. So ARMA (stackMax==1).
    // Sem max valido = nao escreve (nunca fantasma). So escreve se caiu.
    static void TopAmmo(void* item, int oAmmo, int oMax) {
        if (!item || oAmmo < 0) return;
        static int oDbStackGun = -2;
        if (oDbStackGun == -2) {
            oDbStackGun = FieldOff(fDbStack);
        }
        // maxAmmo: descoberta 1x por ITEM (DatabaseGun nao muda).
        // Cache: evita GetDataBaseItem todo ciclo (era o invoke que travava).
        // GC SAFETY (19/09): ponteiro de item morre no GC/troca de arma; db
        // cacheado de item morto = lixo. Revalida: cur precisa ser plausivel
        // (0..100000); se o item mudou de conteudo, redescobre o max.
        static void* s_maxItem[64] = { nullptr };
        static int s_maxVal[64] = { 0 };
        static int s_maxCur[64] = { -1 }; // ammo visto quando cacheou
        static bool s_maxInit[64] = { false };
        int max = -2; // -2 = ainda nao sei
        int cur = ReadI(item, oAmmo, -1);
        if (cur < 0 || cur > 100000) return; // item invalido/morto: nao toca
        for (int k = 0; k < 64; ++k) {
            if (s_maxItem[k] == item) {
                // GC check: se o ammo atual esta MUITO longe do visto no cache
                // (ex. outra arma reusou o endereco), redescobre.
                int dc = cur - s_maxCur[k];
                if (dc < 0) dc = -dc;
                if (s_maxInit[k] && dc <= 1000) { max = s_maxVal[k]; }
                else { max = -2; s_maxInit[k] = false; }
                break;
            }
        }
        if (max == -2) {
            if (mGetDb && oDbStackGun >= 0) {
                void* db0 = DbCached(item);
                if (db0) {
                    int sm = ReadI(db0, oDbStackGun, -1);
                    if (sm < 0 || sm > 100000) return; // db lixo: nao toca
                    if (sm > 1) return; // pilha: nao eh arma, TopStacks cuida
                }
            }
            max = -1;
            if (mGetDb && oMax >= 0) {
                void* db = DbCached(item);
                if (db) max = ReadI(db, oMax, -1);
            }
            if (max <= 0 || max > 100000) {
                // Sem max valido: NAO escreve (sem fallback fantasma).
                // O fallback antigo segurava "maior valor visto" e travava
                // arma errada/desequipada (bug do desequipar).
                return;
            }
            // Guarda no cache (slot livre ou mais velho).
            int slot = -1;
            for (int k = 0; k < 64; ++k) if (!s_maxItem[k]) { slot = k; break; }
            if (slot < 0) { slot = 0; for (int k = 0; k < 64; ++k) if (s_maxItem[k] < s_maxItem[slot]) slot = k; }
            s_maxItem[slot] = item;
            s_maxVal[slot] = max;
            s_maxCur[slot] = cur;
            s_maxInit[slot] = true;
        }
        // max aqui e sempre valido (>0, <=100000): sem max = return acima.
        if (cur < max) {
            WriteI(item, oAmmo, max);
            s_ammoWrites++;
        }
    }

    // (ItemsReport: forward no topo; definicao apos TopStacks, que detem
    // oMisc/oWepList.)
    // Nomes PT-BR (25/09, enum InventoryItem/ID 2..115 + stackMax audit18).
    static const char* ItemName(int id) {
        switch (id) {
        case 9: return "ShotgunAmmo"; case 10: return "SniperAmmo";
        case 11: return "RifleAmmo"; case 12: return "PistolAmmo";
        case 18: return "Painkiller"; case 19: return "SodaCan";
        case 20: return "Syrup"; case 24: return "Bandage";
        case 25: return "CandyBar"; case 26: return "WaterBottle";
        case 27: return "Fruit"; case 39: return "Shuriken";
        case 51: return "DovesCake"; case 53: return "Stun";
        case 54: return "Grenade"; case 57: return "Wood";
        case 58: return "MetalScrap"; case 59: return "PlasticScrap";
        case 60: return "ExplosiveMaterial"; case 61: return "TriggerGroupParts";
        case 62: return "RecoilBufferParts"; case 63: return "DarkBlocks";
        case 64: return "RiotShell"; case 65: return "GasOperationParts";
        case 66: return "BoltCarrierParts"; case 73: return "GrapeJuice";
        case 74: return "Burger"; case 75: return "ArmoryKey";
        case 76: return "GunClosetKey"; case 88: return "ThrowingAxe";
        case 89: return "Dynamite"; case 90: return "HolyWater";
        case 107: return "He40mmAmmo"; case 109: return "ChocolateMilk";
        case 110: return "Med"; case 111: return "Baguette";
        default: return "?";
        }
    }
    static bool s_itemsRepDone = false;
    static void ItemsReportReset() { s_itemsRepDone = false; }

    // Reserva + pilhas (regra universal do IL Get/SetGenericNumericValue:
    // stackMax==1 -> o numero eh ammo; senao eh stackCount; teto = stackMax).
    // - Com bInfAmmo: trava stackCount=stackMax nas pilhas do storage cujo
    //   id == ammoID da arma (HUD reserva honesto: 30/150, nao 30/0).
    // - Com bInfItems: trava stackCount=stackMax em pilhas de misc+storage
    //   por SUBTIPO (municao/arremessavel/consumivel; material/chave/peca
    //   fora pela raiz â€” anti-flood por categoria, nao por numero).
    // - Toda escrita passa pelo guardiao (direcao+teto+quarentena).
    // - B2: caminha storage E misc (categoria 5 mora em misc).
    static int oMisc = -2; // offset de PlayerEquippedItems.misc (via API)
    static int oWepList = -2; // offset de PlayerEquippedItems.weapons (via API, pilhas no equip)
    static void TopStacks(void* local, int oInv, int oStorage, int oItems,
        int oStack, int oDbStack, int oId, bool items) {
        if (!local || oInv < 0 || oStorage < 0 || oItems < 0) return;
        if (oStack < 0 || oDbStack < 0) return;
        if (oMisc == -2) {
            oMisc = -1;
            if (cPEq) {
                MonoClassField* fm = pFieldFrom(cPEq, "misc");
                oMisc = FieldOff(fm);
            }
        }
        if (oWepList == -2) {
            oWepList = -1;
            if (cPEq) {
                MonoClassField* fw = pFieldFrom(cPEq, "weapons");
                oWepList = FieldOff(fw);
            }
        }
        // Throttle de leitura pesada (anti-hang 18/09): GetDataBaseItem por
        // item a 30Hz trava o Mono. Invokes de db so 1x/4 ciclos (~8Hz);
        // escrita em db conhecido roda todo ciclo (memcpy+WriteI, sem invoke).
        // Reserva do HUD = StoredItemCount = SO storage.items (IL auditado
        // 19/09). Por isso reserva caminha SO storage: pilha fora do storage
        // (misc/weapons) NAO conta no "x/RESERVA" do HUD.
        // COSMETICO 23/09: a aba Equipamento (TAB) mostra stackCount das
        // ARMAS em weapons[] (9/1 em vez de 9/200). Por isso reserva (!items)
        // caminha storage[0] + weapons[1]: no weapons, normaliza stackCount=1
        // nas armas (igual ao SetGenericNumericValue do jogo ao criar).
        static int s_tsSlowN = 0;
        bool tsHeavy = ((++s_tsSlowN & 3) == 0);
        __try {
            void* pinv = ReadP(local, oInv);
            if (!pinv) return;
            // Reserva (!items): storage[0] + weapons[1] (cosmetico TAB).
            // Items (true): storage + misc + weapons (tudo trava no teto).
            void* lists[3] = { nullptr, nullptr, nullptr };
            int nLists = 0;
            void* cont = ReadP(pinv, oStorage);
            if (cont) {
                void* ls = ReadP(cont, oItems);
                if (ls) lists[nLists++] = ls;
            }
            if (!items) {
                // weapons em [1] (li==1): so normalizacao cosmetica.
                void* peq = ReadP(pinv, FieldOff(fEq));
                if (peq && oWepList >= 0 && nLists < 3) {
                    void* lw = ReadP(peq, oWepList);
                    if (lw) lists[nLists++] = lw;
                }
            }
            if (items && oMisc >= 0) {
                void* peq = ReadP(pinv, FieldOff(fEq));
                if (peq) {
                    void* lm = ReadP(peq, oMisc);
                    if (lm) lists[nLists++] = lm;
                    // Pilhas no equipment: weapons tb pode conter consumivel/
                    // arremessavel equipado (stackCount gasta ao usar).
                    if (oWepList >= 0 && nLists < 3) {
                        void* lw = ReadP(peq, oWepList);
                        if (lw) lists[nLists++] = lw;
                    }
                }
            }
            // Reserva: conta pilhas por tipo (p/ criar 1 se zerada).
            // (s_pileCount/s_pileSmax = static global, declarado no topo.)
            if (!items) for (int k = 0; k < 8; ++k) { s_pileCount[k] = 0; s_pileSmax[k] = 0; }
            for (int li = 0; li < nLists; ++li) {
            // Cache de db por item (anti-hang): smax/id conhecidos sem invoke.
            // Heavy (1x/4 ciclos) resolve db novo; leve usa cache ou pula.
            WalkList(lists[li], 64, [&](void* it, int) {
                if (!mGetDb) return;
                void* db = DbCached(it);
                if (!db) {
                    if (!tsHeavy) return; // sem cache e sem vez: pula
                    db = InvokeObj(mGetDb, it, nullptr);
                    if (!db) return;
                    // alimenta o cache p/ os proximos ciclos
                    for (int k = 0; k < 64; ++k) {
                        if (!s_dbCache[k].item) {
                            s_dbCache[k].item = it; s_dbCache[k].db = db;
                            s_dbCache[k].tick = s_dbTick; break;
                        }
                    }
                }
                int smax = ReadI(db, oDbStack, -1);
                if (smax <= 0 || smax > 100000) return; // db lixo/GC: nao toca
                if (oId < 0 || oStack < 0) return;
                int id = ReadI(it, oId, -1);
                int cur = ReadI(it, oStack, -1);
                // GC SAFETY: id fora do enum (2..116) ou stack absurdo =
                // item morto/reciclado: nao registra, nao escreve.
                if (id < 2 || id > 116) return;
                // Preserve the original shared stack limit; only refill owned stacks.
                // COSMETICO 23/09 (aba Equipamento mostra stackCount das armas):
                // ARMA (stackMax==1) na lista weapons (li>0, reserva): normaliza
                // stackCount=1 (igual ao SetGenericNumericValue do jogo ao criar:
                // stackMax==1 -> stackCount=1, ammo=dose). Pente intacto (TopAmmo).
                if (smax == 1) {
                    if (!items && li > 0 && cur != 1) WriteI(it, oStack, 1);
                    return; // arma: nunca trava como pilha
                }
                if (cur < 0 || cur > smax) return;
                if (!items) {
                    // Reserva: trava stackCount=stackMax na pilha do ammoID da
                    // arma (auditoria 20/09: HUD = StoredItemCount(ammoID)).
                    // Compara com s_ammoIdCur (tipo real da equipada), nao com
                    // faixa fixa â€” cobre RiotShell e qualquer calibre especial.
                    // Fallback: balas basicas 10-13/108 (antes do 1o resolve).
                    bool isWanted = (s_ammoIdCur >= 2 && id == s_ammoIdCur) ||
                        ((id >= 10 && id <= 13) || id == 108);
                    if (!isWanted) return;
                    AmmoIdAdd(id); // registra p/ pente extra + telemetria
                    for (int k = 0; k < 8; ++k) {
                        if (s_ammoIds[k] == id) {
                            s_pileCount[k]++;
                            if (smax > s_pileSmax[k]) s_pileSmax[k] = smax;
                            break;
                        }
                    }
                } else {
                    // Items = TUDO (materiais 999x, granada, bandagem, bala).
                    // Sem filtro de smax: madeira/dark block travam no teto.
                }
                if (cur >= smax) return; // ja no teto: nao escreve
                WriteI(it, oStack, smax);
            });
            } // fim for li (storage; items inclui misc/weapons)
            // Cria pilha via AmmoEnsurePile (acima, no ApplyAmmo): aqui so
            // trava o que existe. Sem pilha = proximo ciclo pesado cria.
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // Definicao do relatorio (forward declarado antes de TopStacks).
    static void ItemsReport(void* local, int oInv, int oStorage, int oItems,
        int oStack, int oDbStack, int oId) {
        if (s_itemsRepDone) return;
        s_itemsRepDone = true;
        if (!local || !mGetDb) { Log::Info("[ITEMS] relatorio: sem dados (db indisponivel)."); return; }
        __try {
            void* pinv = ReadP(local, oInv);
            if (!pinv) return;
            Log::Info("[ITEMS] travados (id=stack/teto):");
            void* lists[3] = { nullptr, nullptr, nullptr };
            int nLists = 0;
            void* cont = ReadP(pinv, oStorage);
            if (cont) {
                void* ls = ReadP(cont, oItems);
                if (ls) lists[nLists++] = ls;
            }
            void* peq = ReadP(pinv, FieldOff(fEq));
            if (peq) {
                if (oMisc >= 0) {
                    void* lm = ReadP(peq, oMisc);
                    if (lm && nLists < 3) lists[nLists++] = lm;
                }
                if (oWepList >= 0 && nLists < 3) {
                    void* lw = ReadP(peq, oWepList);
                    if (lw) lists[nLists++] = lw;
                }
            }
            int seen[128] = { 0 };
            int nSeen = 0;
            for (int li = 0; li < nLists; ++li) {
                WalkList(lists[li], 64, [&](void* it, int) {
                    int id = (oId >= 0) ? ReadI(it, oId, -1) : -1;
                    if (id < 2 || id > 116) return;
                    for (int k = 0; k < nSeen; ++k) if (seen[k] == id) return;
                    if (nSeen < 128) seen[nSeen++] = id;
                    void* db = DbCached(it);
                    int sm = db ? ReadI(db, oDbStack, -1) : -1;
                    int cur = (oStack >= 0) ? ReadI(it, oStack, -1) : -1;
                    if (sm == 1) {
                        int am = ReadI(it, FieldOff(fAmmo), -1);
                        Log::Infof("[ITEMS] %s (id=%d) arma pente=%d.", ItemName(id), id, am);
                    } else if (sm > 1) {
                        Log::Infof("[ITEMS] %s (id=%d) %dx.", ItemName(id), id, sm);
                    }
                });
            }
            Log::Infof("[ITEMS] total=%d tipos.", nSeen);
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // C1: categoria permite trava? (municao/arremessavel/consumivel = sim;
    // material/chave/peca/quest = nao). Via DatabaseItem.GetSubType() quando
    // resolvido; fallback: smax<=64.
    static MonoMethod* mSubType = nullptr;
    static bool s_subLogged = false;
    static bool SubTypeOk(void* db) {
        if (!db) return false;
        if (mSubType) {
            int st = -1;
            __try {
                MonoObject* exc = nullptr;
                MonoObject* ret = pInvoke(mSubType, db, nullptr, &exc);
                if (!exc && ret) st = *(int*)pUnbox(ret);
            } __except (EXCEPTION_EXECUTE_HANDLER) {}
            // SubType enum: valores exatos variam; considera permitido tudo
            // exceto material/quest (conhecidos apos 1a observacao no log).
            // Por enquanto: loga 1x cada valor p/ mapear, permite smax<=64.
            if (!s_subLogged) { s_subLogged = true; }
            static int s_seen[64] = { 0 };
            if (st >= 0 && st < 64 && !s_seen[st]) {
                s_seen[st] = 1;
                Log::Infof("[SUBTYPE] db=0x%p subtype=%d", db, st);
            }
        }
        void* itemDb = db; (void)itemDb;
        return true; // decisao final pelo smax no chamador (<=64)
    }

    // Dinheiro infinito (MISC/Sobrevivencia): 1x AddCurrency p/ cada moeda
    // (Dollars=0, Silver=1, Gold=2) por sessao + trava amount=99999 todo ciclo
    // nas 3. B3: antes so Dollar. Offsets via API; falha = desliga sozinho.
    static MonoClassField* fSilver = nullptr;
    static MonoClassField* fGold = nullptr;
    static void ApplyMoney() {
        if (!s_moneyOk) return;
        static int oDollar = -2, oSilver = -2, oGold = -2, oAmount = -2;
        if (oDollar == -2) {
            oDollar = FieldOff(fDollar); oAmount = FieldOff(fAmount);
            if (!fSilver && cCur) fSilver = pFieldFrom(cCur, "<Silver>k__BackingField");
            if (!fGold && cCur) fGold = pFieldFrom(cCur, "<Gold>k__BackingField");
            oSilver = FieldOff(fSilver); oGold = FieldOff(fGold);
            if (!s_moneyLogged) {
                s_moneyLogged = true;
                Log::Infof("[MONEY] offs dollar=%d silver=%d gold=%d amount=%d mAddCur=%d",
                    oDollar, oSilver, oGold, oAmount, mAddCur ? 1 : 0);
            }
            if (oDollar < 0 || oAmount < 0) {
                Log::Warn("[MONEY] cadeia incompleta â€” desativado.");
                s_moneyOk = false;
                return;
            }
        }
        if (oDollar < 0) return;
        __try {
            void* inst = nullptr;
            if (!StaticInstance(cCur, fCurInst, inst) || !inst) return;
            // 1x por sessao: AddCurrency p/ cada moeda resolvida.
            if (mAddCur && !s_moneyGiven) {
                s_moneyGiven = true;
                int offs[3] = { oDollar, oSilver, oGold };
                for (int c = 0; c < 3; ++c) {
                    if (offs[c] < 0) continue;
                    int id = c, qty = 99999;
                    void* args[2] = { &id, &qty };
                    MonoObject* exc = nullptr;
                    __try { pInvoke(mAddCur, inst, args, &exc); }
                    __except (EXCEPTION_EXECUTE_HANDLER) {}
                }
                Log::Info("[MONEY] AddCurrency 1x executado (3 moedas).");
            }
            int offs[3] = { oDollar, oSilver, oGold };
            for (int c = 0; c < 3; ++c) {
                if (offs[c] < 0) continue;
                void* cur_ = ReadP(inst, offs[c]);
                if (!cur_) continue;
                int v = ReadI(cur_, oAmount, -1);
                if (v >= 0 && v < 99999) WriteI(cur_, oAmount, 99999);
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // REMOVIDO 17/09: C3 Spawn de Itens (DoGiveItem + kGiveIds 76/55/90/25/20).
    // IDs reais preservados no historico do commit p/ fase futura.
    // mCreateItem/mAddItem continuam (AmmoEnsurePile cria pilha de reserva).

    // C4: LoadoutSelector.UnlockAll â€” desbloqueia todas as armas/itens do vendedor.
    // Invoke 1x por sessao (flag permanente do jogo). SEH total.
    // NOTA 18/09 (auditoria loja): o VENDEDOR (VendorMenu/BuyOffer) usa
    // Pricing.IsBlocked, NAO LoadoutSelector. UnlockAll abre o LOADOUT (tela
    // de kits); a loja (compra Loadout I/II/III) precisa de dinheiro â€”
    // por isso existe o Infinite Money (99999). LOAD:ON = kits livres.
    static void ApplyLoadout() {
        if (s_loadoutDone) { s.loadoutOn = true; return; }
        if (!cLoadout || !fLoadInst || !mUnlockAll) {
            static bool s_warned = false;
            if (!s_warned) { s_warned = true; Log::Warn("[LOADOUT] cadeia incompleta â€” desativado."); }
            return;
        }
        __try {
            void* inst = nullptr;
            if (!StaticInstance(cLoadout, fLoadInst, inst) || !inst) return;
            MonoObject* exc = nullptr;
            pInvoke(mUnlockAll, inst, nullptr, &exc);
            if (!exc) {
                s_loadoutDone = true;
                s.loadoutOn = true;
                Log::Info("[LOADOUT] UnlockAll executado (todas as armas/itens desbloqueados).");
            } else {
                Log::Warn("[LOADOUT] UnlockAll lancou excecao.");
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            Log::Warn("[LOADOUT] UnlockAll SEH.");
        }
    }

    // Defesa (God + Stamina): reescreve os campos do LOCAL player.
    // Roda no bloco rapido da worker (todo ciclo, ~33ms) SOB GATE de mapa
    // (local vivo dentro da partida). Sem hook, sem patch, custo zero desligado.
    static void ApplyDefense(void* local) {
        if (!local) return;
        // God Mode: trava healthFast + healthSlow em 100.
        if (s_options.bGodMode) {
            float hp = ReadF(local, Off::PM_healthFast);
            if (hp > 0.0f && hp < 100.0f) { // morto (<=0) nao ressuscita
                WriteF(local, Off::PM_healthFast, 100.0f);
                WriteF(local, Off::PM_healthSlow, 100.0f);
            }
        }
        // Stamina: trava fast+slow no maxStamina (correr sem cansar).
        if (s_options.bInfStamina) {
            float mx = ReadF(local, Off::PM_maxStamina, 100.0f);
            if (!(mx > 0.0f && mx <= 1000.0f)) mx = 100.0f;
            if (ReadF(local, Off::PM_staminaFast) < mx)
                WriteF(local, Off::PM_staminaFast, mx);
            if (ReadF(local, Off::PM_staminaSlow) < mx)
                WriteF(local, Off::PM_staminaSlow, mx);
        }
    }

    // Managed ownership preserves baselines across GC and scene transitions.
    static bool s_modifiersPending = false;
    static bool HasPendingRestores() { return s_modifiersPending; }
    static void ResetRestoreCaches() {
        if (s_modifierReset) InvokeAimBool(s_modifierReset, nullptr, nullptr, s_modifiersPending);
    }
    static void ApplyWeapon(void* local) {
        if (!s_modifierApply) return;
        int flags = (s_options.bNoRecoil ? 1 : 0) | (s_options.bNoSpread ? 2 : 0) |
            (s_options.bNoSway ? 4 : 0) | (s_options.bTightAim ? 8 : 0) |
            (s_options.bRapidFire ? 16 : 0) | (s_options.bSpeedHack ? 32 : 0) |
            (s_options.bSuperJump ? 64 : 0) | (s_options.bRollSpeed ? 128 : 0) |
            (s_options.bFastKnife ? 256 : 0) | (s_options.bFullAuto ? 512 : 0) |
            (s_options.bUnlockSlots ? 1024 : 0) | (s_options.bNoClip ? 2048 : 0) | (s_options.bMenuOpen ? 4096 : 0) | (s_options.bEnemyMagnet ? 8192 : 0);
        void* args[] = {local, &flags, &s_options.fRapidMult, &s_options.fSpeedMult,
            &s_options.fJumpMult, &s_options.fRollMult, &s_options.fKnifeMult, &s_options.fNoClipSpeed, &s_options.fMagnetRadius, &s_options.iMagnetTargets};
        InvokeAimBool(s_modifierApply, nullptr, args, s_modifiersPending);
        BridgeError(s_modifierError, "MODIFIERS");
        s.slotsOn=s_options.bUnlockSlots;
        if(s_modifierStatus) {
            MonoObject* exception=nullptr;
            auto message=pInvoke(s_modifierStatus,nullptr,nullptr,&exception);
            if(message && !exception){char* text=pStrUtf8(message);if(text){strncpy_s(s.modifierStatus,text,_TRUNCATE);pFree(text);}}
        }
    }

    // ========================================================================
    // AIMBOT estilo wohax (reescrita 23/09, mesma logica de funcionamento).
    // wohax (Warface, CryBot/Lua): elege por distancia do crosshair em px,
    // mira vetor 3D (bone - camPos normalizado) via direcao do ator, atira
    // so com firing=1 (gatilho segurado), sticky com intervalo entre trocas.
    // Aqui: mesma maquina de estados, adaptada p/ Unity Mono (PlayerCamera
    // Angle+pitch + Rotate yaw; gatilho = Aim Key Hold/Toggle ou AutoAim).
    // ========================================================================
    // (FOV real via menu fCamFov; sem leitura extra.)

    static void AuditAmmoTick(void* pcs); // forward (diagnostico 1x/2s, so leitura)
    static void ReadAll() {
        // LEITURA PURA (18/09): nunca escreve (gate via s.inMap no bloco rapido).
        // Daytime (prova de leitura viva simples).
        void* day = nullptr;
        if (StaticInstance(cDay, fDayInst, day)) {
            s.dayTime = ReadF(day, Off::DT_cur);
            s.dayLenMin = ReadF(day, Off::DT_len);
        }
        // Players: local via invoke, resto = aliados.
        void* pcs = nullptr;
        s.players = 0;
        s.allies = 0;
        s.allyHp = 0.0f;
        s.localHp = 0.0f;
        s.localStam = 0.0f;
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
                } else if (!local) {
                    ++s.allies;
                    if (!gotAlly && hp > 0.0f) {
                        gotAlly = true;
                        s.allyHp = hp;
                    }
                }
            });
            if (!gotLocal) {
                s.localHp = 0;
                s.allies = 0;
                s.allyHp = 0.0f;
            } else {
                s.allies = (std::max)(0, s.players - 1);
            }
            // Gate no mapa (18/09): escrita removida daqui (ReadAll = leitura
            // pura). God/Stamina rodam no bloco rapido sob s.inMap (local vivo).
            // Auditoria ammo 1x/2s (19/09): diagnostico completo da cadeia
            // (arma equipada real, pilhas por tipo, offsets). So observa e
            // loga quando MUDA (sem spam); nunca escreve.
            AuditAmmoTick(pcs);
        }
        // Zumbis: conta vivos + HP do primeiro vivo; cruza com totalRealZombies.
        void* zl = nullptr;
        s.zombies = 0;
        s.zHp0 = 0;
        if (StaticInstance(cZLoader, fZLInst, zl)) {
            int total = ReadI(zl, Off::ZL_totalReal, -1);
            void* list = ReadP(zl, Off::ZL_zombies);
            int alive = 0;
            const long long gatherDeadline = PiNow() + 6000;
        WalkList(list, 512, [&](void* e, int) {
            if (PiNow() >= gatherDeadline) return;
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

    // Auditoria ammo 1x/2s (19/09, CE MCP + dnSpy): diagnostico completo.
    // Para cada PlayerMain local: arms.selectedItem (SetType+Value) ->
    // weapons[Value] (id/ammo) -> DatabaseGun (ammoID/maxAmmo) ->
    // storage.items (pilhas do tipo: id/stackCount/stackMax).
    // So LEITURA (memcpy+SEH, 2 invokes GetDb max por player). Loga 1x por
    // combinacao (SetType/Value/ammoID) + resumo de pilhas quando muda.
    static void AuditAmmoTick(void* pcs) {
        if (!pcs) return;
        __try {
            void* list = ReadP(pcs, Off::PCS_players);
            if (!list) return;
            WalkList(list, 16, [&](void* e, int) {
                if (!InvokeBool(mHasLocal, e)) return;
                // Offsets via FieldOff (resolve 1x, cache estatico).
                static int aInv = -2, aEq = -2, aArms = -2, aSel = -2;
                static int aStor = -2, aItems = -2;
                static int aAmmo = -2, aStack = -2, aId = -2;
                static int aDbAmmoId = -2, aDbMax = -2, aDbStack = -2;
                static int aWep = -2, aMisc = -2;
                if (aInv == -2) {
                    aInv = FieldOff(fInv);
                    aEq = FieldOff(fEq);
                    aStor = FieldOff(fStorage);
                    aItems = FieldOff(fItems);
                    aAmmo = FieldOff(fAmmo);
                    aStack = FieldOff(fStack);
                    aId = FieldOff(fId);
                    if (cPlayer) {
                        MonoClassField* fa = pFieldFrom(cPlayer, "arms");
                        aArms = FieldOff(fa);
                    }
                    if (aArms >= 0) {
                        MonoClass* cArms = nullptr;
                        ResolveClass("PlayerArms", cArms);
                        if (cArms) {
                            MonoClassField* fs = pFieldFrom(cArms, "selectedItem");
                            aSel = FieldOff(fs);
                        }
                    }
                    if (cPEq) {
                        MonoClassField* fw = pFieldFrom(cPEq, "weapons");
                        MonoClassField* fm = pFieldFrom(cPEq, "misc");
                        aWep = FieldOff(fw); aMisc = FieldOff(fm);
                    }
                    if (cDbGun) {
                        MonoClassField* fa2 = pFieldFrom(cDbGun, "ammoID");
                        MonoClassField* fm2 = pFieldFrom(cDbGun, "maxAmmo");
                        aDbAmmoId = FieldOff(fa2); aDbMax = FieldOff(fm2);
                    }
                    MonoClass* cDbItem = nullptr;
                    if (ResolveClass("DatabaseItem", cDbItem)) {
                        MonoClassField* fs2 = pFieldFrom(cDbItem, "stackMax");
                        aDbStack = FieldOff(fs2);
                    }
                    Log::Infof("[AUDIT] offs inv=%d eq=%d arms=%d sel=%d stor=%d items=%d ammo=%d stack=%d id=%d dbAmm=%d dbMax=%d dbStk=%d wep=%d misc=%d",
                        aInv, aEq, aArms, aSel, aStor, aItems, aAmmo, aStack, aId, aDbAmmoId, aDbMax, aDbStack, aWep, aMisc);
                }
                if (aInv < 0 || aArms < 0 || aSel < 0) return;
                void* arms = ReadP(e, aArms);
                if (!arms) { Log::Info("[AUDIT] arms=null"); return; }
                // selectedItem struct: SetType@+0, Value@+4 (rel. arms+aSel).
                int st = -1, vv = -1;
                __try {
                    memcpy(&st, (char*)arms + aSel, sizeof(st));
                    memcpy(&vv, (char*)arms + aSel + 4, sizeof(vv));
                } __except (EXCEPTION_EXECUTE_HANDLER) { return; }
                void* pinv = ReadP(e, aInv);
                void* peq = pinv ? ReadP(pinv, aEq) : nullptr;
                void* wlist = (peq && aWep >= 0) ? ReadP(peq, aWep) : nullptr;
                int nW = 0;
                __try { if (wlist) memcpy(&nW, (char*)wlist + Off::L_size, sizeof(nW)); }
                __except (EXCEPTION_EXECUTE_HANDLER) {}
                // Arma no indice Value (se SetType==Weapon).
                int wId = -1, wAmmo = -1, wDbA = -1, wDbM = -1;
                if (st == 1 && vv >= 0 && vv < nW && wlist) {
                    void* arr = ReadP(wlist, Off::L_items);
                    void* witem = nullptr;
                    __try {
                        if (arr) memcpy(&witem, (char*)arr + Off::A_data + (size_t)vv * 8, 8);
                    } __except (EXCEPTION_EXECUTE_HANDLER) {}
                    if (witem && aAmmo >= 0 && aId >= 0) {
                        __try {
                            memcpy(&wAmmo, (char*)witem + aAmmo, sizeof(wAmmo));
                            memcpy(&wId, (char*)witem + aId, sizeof(wId));
                        } __except (EXCEPTION_EXECUTE_HANDLER) {}
                        if (mGetDb && aDbAmmoId >= 0) {
                            void* db = DbCached(witem);
                            if (db) {
                                __try {
                                    memcpy(&wDbA, (char*)db + aDbAmmoId, sizeof(wDbA));
                                    if (aDbMax >= 0) memcpy(&wDbM, (char*)db + aDbMax, sizeof(wDbM));
                                } __except (EXCEPTION_EXECUTE_HANDLER) {}
                            }
                        }
                    }
                }
                // Pilhas no storage do tipo wDbA (id/stackCount/stackMax).
                int pileN = 0, pileSum = 0, pileSmax = 0;
                if (pinv && aStor >= 0 && aItems >= 0 && wDbA > 0) {
                    void* cont = ReadP(pinv, aStor);
                    void* slist = cont ? ReadP(cont, aItems) : nullptr;
                    if (slist) WalkList(slist, 64, [&](void* it, int) {
                        int id = -1, sc = -1;
                        __try {
                            if (aId >= 0) memcpy(&id, (char*)it + aId, sizeof(id));
                            if (aStack >= 0) memcpy(&sc, (char*)it + aStack, sizeof(sc));
                        } __except (EXCEPTION_EXECUTE_HANDLER) { return; }
                        if (id == wDbA) {
                            pileN++;
                            if (sc > 0) pileSum += sc;
                            if (mGetDb && aDbStack >= 0) {
                                void* db = DbCached(it);
                                if (db) {
                                    int sm = -1;
                                    __try { memcpy(&sm, (char*)db + aDbStack, sizeof(sm)); }
                                    __except (EXCEPTION_EXECUTE_HANDLER) {}
                                    if (sm > pileSmax) pileSmax = sm;
                                }
                            }
                        }
                    });
                }
                // Loga quando MUDA (chave = st/vv/wDbA/wAmmo/pileN/pileSum).
                static long long s_lastKey = 0x7FFFFFFFFFFFFFFFLL;
                long long key = ((long long)(st + 2) << 48) | ((long long)(vv + 2) << 40) |
                    ((long long)(wDbA + 2) << 24) | ((long long)(wAmmo + 2) << 12) |
                    ((long long)(pileN) << 8) | (long long)(pileSum & 0xFF);
                if (key != s_lastKey) {
                    s_lastKey = key;
                    Log::Infof("[AUDIT] sel(SetType=%d Value=%d) weapons_n=%d arma(id=%d ammo=%d) db(ammoID=%d max=%d) pilhas(tipo=%d n=%d soma=%d smax=%d)",
                        st, vv, nW, wId, wAmmo, wDbA, wDbM, wDbA, pileN, pileSum, pileSmax);
                }
            });
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
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
        const long long gatherDeadline = PiNow() + 6000;
        WalkList(list, 512, [&](void* e, int) {
            if (PiNow() >= gatherDeadline) return;
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

    static void __cdecl GameUpdate() {
        if (!s_espRun.load(std::memory_order_acquire)) return;
        const long long now = PiNow();
        if (now < s_nextCycle) return;
        s_options = s_settings.Read();
        s_vpW = s_options.viewportWidth; s_vpH = s_options.viewportHeight;
        s_nextCycle = now + 33000;
        RunGameCycle();
        s_stateSnapshot.Publish(s);
        static long long nextHeartbeat = 0;
        if (now >= nextHeartbeat) {
            nextHeartbeat = now + 5000000;
            Log::Infof("[RUNTIME] Unity Update tid=%lu ciclo=%.1fms entidades=%d", GetCurrentThreadId(), (PiNow()-now)/1000.0, s.espShown);
        }
        const long long elapsed = PiNow() - now;
        if (elapsed > 25000) {
            s_nextCycle = PiNow() + 100000;
            static long long nextLog = 0;
            if (now >= nextLog) { nextLog = now + 5000000; Log::Warnf("[RUNTIME] ciclo lento: %.1fms", elapsed/1000.0); }
        }
    }
    static DWORD WINAPI BootstrapThread(LPVOID) {
        for (int attempt=0; attempt<100 && !Init(); ++attempt) Sleep(100);
        bool installed = false;
        if (s.ready && LoadAimBridge() && s_bridgeStart) {
            HMODULE pinned = nullptr;
            // Managed callbacks outlive FreeLibrary. Keep code mapped until process exit.
            if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                reinterpret_cast<LPCWSTR>(&GameUpdate), &pinned)) {
                void* callback = reinterpret_cast<void*>(&GameUpdate);
                void* args[] = { &callback };
                s_espRun.store(true, std::memory_order_release);
                installed = InvokeAimBool(s_bridgeStart, nullptr, args, installed) && installed;
            }
        }
        if (!installed) s_espRun.store(false, std::memory_order_release);
        Log::Info(installed ? "[RUNTIME] callback Unity Update pronto; nenhuma worker acessa objetos do jogo."
                            : "[RUNTIME] inicializacao falhou; funcionalidades suspensas.");
        using Detach = void (__cdecl*)(MonoThread*);
        auto detach = reinterpret_cast<Detach>(GetProcAddress(GetModuleHandleW(L"mono-2.0-bdwgc.dll"), "mono_thread_detach"));
        if (detach && s_bootstrapMonoThread) detach(s_bootstrapMonoThread);
        return 0;
    }
    void Tick() {
        // Present only starts bootstrap; it never attaches to Mono or invokes Unity.
        bool expected = false;
        if (!s_bootstrapStarted.compare_exchange_strong(expected, true)) return;
        HANDLE thread = CreateThread(nullptr, 0, BootstrapThread, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
        else { s_bootstrapStarted = false; Log::Error("[RUNTIME] nao foi possivel iniciar bootstrap."); }
    }
    const State& Get() { return s_stateSnapshot.Read(); }
    int GetEsp(EspEntry* out, int max) {
        if (!out || max <= 0) return 0;
        const auto& snapshot = s_entitySnapshot.Read();
        const int n = (std::min)(snapshot.count, max);
        for (int i = 0; i < n; ++i) out[i] = snapshot.entries[i];
        return n;
    }
    void Shutdown() {
        // No waits and no synchronization object destruction under loader lock.
        s_espRun.store(false, std::memory_order_release);

    }
}





















