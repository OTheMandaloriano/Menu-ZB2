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
    static MonoClass* cMp = nullptr;          // MultiplayerController
    static MonoClassField* fMpInst = nullptr; // MultiplayerController.instance
    static MonoMethod* mIsServer = nullptr;   // MultiplayerController.IsServer()
    static MonoMethod* mIsSingle = nullptr;   // MultiplayerController.get_IsSinglePlayer()
    static MonoMethod* mIsMulti = nullptr;    // MultiplayerController.get_IsMultiplayer()
    static MonoMethod* mIsClient = nullptr;   // MultiplayerController.IsClient()
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
    static MonoMethod* mGotLoot = nullptr;    // PlayerInteraction.GotLootFromServer(ID,int) — seed nativo
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
    // Tudo via nome (FieldOff) — offsets reais so em runtime.
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
    static MonoClass* cPCam = nullptr;        // PlayerCamera (aim: Angle + transform)
    static MonoClassField* fCamAngle = nullptr;  // PlayerCamera.<Angle>k__BackingField (pitch ±80)
    static MonoMethod* mSetAngle = nullptr;   // PlayerCamera.set_Angle(float)
    static MonoMethod* mGetAngle = nullptr;   // PlayerCamera.get_Angle()
    static MonoMethod* mGetCamTr = nullptr;   // PlayerCamera.get_CameraTransform()
    static MonoClassField* fMainCam = nullptr; // PlayerMain.cam (PlayerCamera do local)
    static float AimReadFov(void); // FOV real (definido antes do ApplyAim)
    static void ApplyAim(void* local); // forward (aimbot: selecao+FOV+escrita, worker)
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
    static void ApplyMove(void* local);   // forward (super pulo + fast knife, worker)

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
    static void ApplySlots(void* local); // forward (slots desbloqueados, 1x)
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
    static void AmmoCachesClear(); // forward (limpa caches na troca de cena)
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
        // Coop host vs cliente: MultiplayerController.IsServer() (IL do DropLoot).
        // Host valida o proprio dano: trava tudo. Cliente: recarga legitima.
        ResolveClass("MultiplayerController", cMp);
        ResolveField(cMp, "MultiplayerController", "instance", fMpInst);
        if (cMp) {
            ResolveMethod(cMp, "MultiplayerController", "IsServer", 0, mIsServer);
            ResolveMethod(cMp, "MultiplayerController", "get_IsSinglePlayer", 0, mIsSingle);
            ResolveMethod(cMp, "MultiplayerController", "get_IsMultiplayer", 0, mIsMulti);
            ResolveMethod(cMp, "MultiplayerController", "IsClient", 0, mIsClient);
        }
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
        // Defeito 2 (auditoria 21/09): faltava General — dbg sempre 0.
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
        // classe pai + get_nested_types? nao ha API simples — usa o field do
        // array (nodes) e calcula tMin/tMax por posicao (attack=ptr@0,
        // tMin@8, tMax@12). Sem FieldOff: offsets fixos da struct.
        ResolveClass("PlayerMovement", cMove);
        ResolveField(cMove, "PlayerMovement", "jumpSpeed", fMoveJump);
        // Aimbot ciclo 1 (23/09): PlayerCamera = pitch (Angle ±80) + yaw
        // (Rotate no transform). Falha = nao-fatal; ApplyAim desliga sozinho.
        ResolveClass("PlayerCamera", cPCam);
        ResolveField(cPCam, "PlayerCamera", "<Angle>k__BackingField", fCamAngle);
        if (cPCam) {
            MonoMethod* t = pMethodFrom(cPCam, "set_Angle", 1);
            if (t) { mSetAngle = t; s.resolvedMethods++; }
            t = pMethodFrom(cPCam, "get_Angle", 0);
            if (t) { mGetAngle = t; s.resolvedMethods++; }
            t = pMethodFrom(cPCam, "get_CameraTransform", 0);
            if (t) { mGetCamTr = t; s.resolvedMethods++; }
        }
        if (cPlayer) ResolveField(cPlayer, "PlayerMain", "cam", fMainCam);
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
            extern long s_ammoWritesExt();
            s.ammoWrites = (int)s_ammoWritesExt();
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
            // GATE NO MAPA (18/09): funcoes de escrita so rodam quando o jogador
            // esta dentro do mapa (gotLocal && localHp > 0). No menu/lobby,
            // SceneAlive pode retornar true (camera + ZombieLoader existem),
            // mas o player nao existe ainda. Sem gate = invoke em objeto nulo =
            // hang/crash reportado pelo operador.
            bool wantDef = s.ready && (Config::bGodMode || Config::bInfStamina || Config::bInfAmmo || Config::bInfItems || Config::bInfMoney || Config::bUnlockSlots || Config::bUnlockLoadout || Config::bNoRecoil || Config::bNoSpread || Config::bNoSway || Config::bTightAim || Config::bRapidFire || Config::bSuperJump || Config::bFastKnife || Config::bInstantReload || Config::bAimbot || Config::bAutoAim);
            if (wantDef && SceneAlive()) {
                // Modo real via MultiplayerController (throttle 2s, SEH).
                // coopMode: 0=LOBBY 1=SINGLE 2=CLIENTE 3=HOST.
                static long long s_lastModeCheck = 0;
                {
                    long long nowM = PiNow();
                    if (nowM - s_lastModeCheck > 2000000LL) {
                        s_lastModeCheck = nowM;
                        __try {
                            void* mpInst = nullptr;
                            if (StaticInstance(cMp, fMpInst, mpInst) && mpInst) {
                                bool isSingle = mIsSingle ? InvokeBool(mIsSingle, mpInst) : false;
                                bool isMulti = mIsMulti ? InvokeBool(mIsMulti, mpInst) : false;
                                bool isSrv = mIsServer ? InvokeBool(mIsServer, mpInst) : false;
                                bool isCli = mIsClient ? InvokeBool(mIsClient, mpInst) : false;
                                int mode = 0; // LOBBY
                                if (isSingle) mode = 1;
                                else if (isMulti && isCli) mode = 2;
                                else if (isMulti && isSrv) mode = 3;
                                else if (isMulti) mode = 3; // fallback: multi sem role = host
                                static int s_lastMode = -1;
                                if (mode != s_lastMode) {
                                    s_lastMode = mode;
                                    static const char* modeNames[4] = { "LOBBY", "SINGLE", "CLIENTE", "HOST" };
                                    Log::Infof("[MODE] modo=%s (isSingle=%d isMulti=%d isSrv=%d isCli=%d)",
                                        modeNames[mode], isSingle ? 1 : 0, isMulti ? 1 : 0, isSrv ? 1 : 0, isCli ? 1 : 0);
                                }
                                s.coopMode = mode;
                            }
                        } __except (EXCEPTION_EXECUTE_HANDLER) {}
                    }
                }
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
                            for (int w = 0; w < 20; ++w) {
                                Sleep(100);
                                if (SceneAlive()) break;
                            }
                            // TROCA DE CENA (19/09): ponteiros de item morrem no
                            // GC/remontagem. Limpa caches p/ nao ler/escrever
                            // lixo (era o crash ao trocar de arma/equipar).
                            // Via setter (declarados depois; forward abaixo).
                            AmmoCachesClear();
                            Log::Info("[AMMO] caches limpos (troca de cena).");
                        }
                        s_lastNPl = nPl;
                    }
                }
                s.inMap = inMap;
                // Funcoes de escrita SO dentro do mapa (gate).
                if (inMap && localEnt) {
                    bool coop = (s.coopMode == 2); // CLIENTE
                    if (Config::bInfMoney) ApplyMoney();
                    ApplyDefense(localEnt);
                    ApplyWeapon(localEnt);
                    ApplyMove(localEnt);
                    if (Config::bUnlockSlots) ApplySlots(localEnt);
                    if (Config::bUnlockLoadout) ApplyLoadout();
                    if (Config::bInfAmmo || Config::bInfItems || Config::bInstantReload) ApplyAmmo(localEnt, coop);
                    if (Config::bAimbot || Config::bAutoAim) ApplyAim(localEnt);
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

    // ========================================================================
    // AMMO — subsistema de municao infinita (reescrito 22/09, revisao senior).
    //
    // MODELO DO JOGO (IL auditado via dnlib — nao mexer sem re-auditar):
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
    // - Seed de pilha: GotLootFromServer(ammoID, stackMax) — o MESMO caminho
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
        // provou que balas reais usam aid 9 (fuzil), 0 e 1 — a faixa 10-116
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
    // (enum byte): 0=Inventory, 1=Equipment, 2=Both — Both tenta os dois.
    // Retry LIMITADO: 3 tentativas por tipo por sessao (cada AddItem sem lugar
    // faz o proprio jogo dropar no chao — retry infinito = tapete de loot).
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
        if (Config::bUnlockSlots && !s_slotsWasOn) {
            s_slotsWasOn = true;
            PileTriesReset("slots ligados");
        } else if (!Config::bUnlockSlots) {
            s_slotsWasOn = false;
        }
        int slot = -1;
        for (int k = 0; k < 8; ++k) if (s_ammoIds[k] == id) slot = k;
        if (slot < 0 || s_pileMade[slot]) return;
        if (s_pileTries[slot] >= 3) return;
        // Resolve 1x (fora do backoff — resolve nao invoca, nao trava).
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
                    Log::Warn("[AMMO] sem espaco p/ pilha (3 tentativas) — libere slot ou Desbloquear Slots. Nao tenta mais.");
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }
    static void ApplyAmmo(void* local, bool coop) {
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
            // o ammoID (leitura) e garante reserva (abaixo) — fluxo legitimo.
            // Recarga automatica: ammo==0 -> TryStartReload (fluxo normal do
            // jogo: PullStoredItems da reserva cheia; host aceita, dano conta).
            // Pente extra ao ativar: ao ligar bInfAmmo (borda de subida),
            // garante 1 pilha cheia p/ CADA arma em weapons (nao espera zerar).
            bool lockMag = !coop && Config::bInfAmmo;
            static bool s_ammoWasOn = false;
            if (Config::bInfAmmo && !s_ammoWasOn) {
                s_ammoWasOn = true;
                // Reset 1x: permite recriar pilhas gastas na sessao anterior.
                for (int k = 0; k < 8; ++k) s_pileMade[k] = false;
                Log::Info("[AMMO] ativado: garantindo 1 pilha por arma.");
            } else if (!Config::bInfAmmo) {
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
            if (coop && Config::bInfAmmo && armsC && oAmmo >= 0) {
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
            // e StoredItemCount(ammoID da equipada) — soma dos stackCount das
            // pilhas no STORAGE com id == ammoID. Entao: garante 1 pilha cheia
            // (stackCount=stackMax) do tipo da EQUIPADA no storage.
            // Sem Create/AddItem (sem invoke = sem hang/crash): escreve direto
            // stackCount=stackMax na 1a pilha do tipo; se nao existe pilha,
            // converte 1 slot vazio? Nao — sem slot livre, so loga (o loot
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
            // E2: varredura weapons[] — registra TODOS os tipos de bala das
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
                // ROUND-ROBIN 22/09: 1 arma por vez (e2cursor) — 4 invokes no
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
            // (fail-open: sem selectedItem confiavel, trava todas — custo de
            // 2-4 escritas/ciclo, sem invoke). TopAmmo so escreve em ARMA com
            // db valido (stackMax==1, max plausivel); pilha nunca entra aqui.
            // ANTI-HANG 22/09: TopAmmo com db NAO cacheado invoca (miss). No
            // loading isso deadlocka. So trava pente com db JA cacheado:
            // 1o ciclo apos ativar = so registra (E2 espaçado resolve o db).
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
            // CHEIO — igual aos grandes cheats (nunca zera, nunca recarrega).
            // Modelo: se cur < max, completa na hora puxando da reserva
            // (need=Pull, igual ao ReloadGun mas sem timer/animacao). Como a
            // reserva e infinita (TopStacks trava no teto), o pulled sempre
            // cobre o need = pente nunca esvazia = sem animacao de recarga.
            // So single/host (cliente: host valida o dano via sync; completar
            // local sem passar pelo ReloadGun do servidor gera divergencia —
            // la vale o TryStartReload limitado + R manual).
            // Condicao: bInstantReload ON + bInfAmmo ON (reserva infinita) +
            // arma com db valido. Roda TODO ciclo (leve, sem invoke: memcpy).
            // Sem bInfAmmo junto, a reserva esvazia e o instant "falha" (pulled=0).
            // DIAG 23/09 (instant nao dispara em single): loga 1x o estado das
            // condicoes (coop? armsC? s_dbPrimed?) p/ achar o gate que barra.
            {
                static bool s_irDiag = false;
                if (Config::bInstantReload && !s_irDiag) {
                    s_irDiag = true;
                    Log::Infof("[INSTANT] on: coop=%d armsC=0x%p primed=%d oWep=%d oMax=%d bInfAmmo=%d",
                        coop ? 1 : 0, armsC, s_dbPrimed ? 1 : 0, oWeapons, oMax, Config::bInfAmmo ? 1 : 0);
                }
                if (!Config::bInstantReload) s_irDiag = false;
            }
            if (Config::bInstantReload && !coop && oWeapons >= 0 && oMax >= 0) {
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
            if (Config::bInfAmmo) {
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
            if (Config::bInfItems)
                TopStacks(local, oInv, oStorage, oItems, oStack, oDbStack, oId, true);
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

    // Reserva + pilhas (regra universal do IL Get/SetGenericNumericValue:
    // stackMax==1 -> o numero eh ammo; senao eh stackCount; teto = stackMax).
    // - Com bInfAmmo: trava stackCount=stackMax nas pilhas do storage cujo
    //   id == ammoID da arma (HUD reserva honesto: 30/150, nao 30/0).
    // - Com bInfItems: trava stackCount=stackMax em pilhas de misc+storage
    //   por SUBTIPO (municao/arremessavel/consumivel; material/chave/peca
    //   fora pela raiz — anti-flood por categoria, nao por numero).
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
                    // faixa fixa — cobre RiotShell e qualquer calibre especial.
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
                Log::Warn("[MONEY] cadeia incompleta — desativado.");
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

    // C4: LoadoutSelector.UnlockAll — desbloqueia todas as armas/itens do vendedor.
    // Invoke 1x por sessao (flag permanente do jogo). SEH total.
    // NOTA 18/09 (auditoria loja): o VENDEDOR (VendorMenu/BuyOffer) usa
    // Pricing.IsBlocked, NAO LoadoutSelector. UnlockAll abre o LOADOUT (tela
    // de kits); a loja (compra Loadout I/II/III) precisa de dinheiro —
    // por isso existe o Infinite Money (99999). LOAD:ON = kits livres.
    static void ApplyLoadout() {
        if (s_loadoutDone) { s.loadoutOn = true; return; }
        if (!cLoadout || !fLoadInst || !mUnlockAll) {
            static bool s_warned = false;
            if (!s_warned) { s_warned = true; Log::Warn("[LOADOUT] cadeia incompleta — desativado."); }
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

    // C2: slots desbloqueados (storage+misc+weapons cheios). O print mostra
    // cadeados em 3 lugares: TotalStorage/Misc (bools do PlayerInventory) +
    // armas 2/3 bloqueadas por WeaponSlotTypes (lista de slots validos) +
    // misc travado por UnlockedMiscSlotsCount. Resolve os 3 + GRID:
    // 4) ItemContainer.TotalSize/UsableSize (IntVec2): sem grid util nao ha
    // onde por a pilha (AddItem recusa = x/0 eterno, bug 21/09). Metodos
    // SetTotalSize/SetUsableSize existem — invoke 1x por container.
    static void ApplySlots(void* local) {
        // Roda a cada ativacao (borda de subida) + reforco todo ciclo (o jogo
        // pode re-travar ao trocar de cena). Sem 'done' permanente: barato
        // (2 bytes) e visivel no debug ([SLOTS] mostra estado atual).
        if (!local) return;
        __try {
            void* pinv = ReadP(local, FieldOff(fInv));
            if (!pinv) return;
            static int oTU = -2, oMU = -2, oUMisc = -2, oWST = -2;
            if (oTU == -2) {
                oTU = -1; oMU = -1; oUMisc = -1; oWST = -1;
                if (cPInv) {
                    MonoClassField* a = pFieldFrom(cPInv, "<TotalStorageUnlocked>k__BackingField");
                    MonoClassField* b = pFieldFrom(cPInv, "<TotalMiscSlotsUnlocked>k__BackingField");
                    oTU = FieldOff(a); oMU = FieldOff(b);
                }
                if (cPEq) {
                    MonoClassField* u = pFieldFrom(cPEq, "<UnlockedMiscSlotsCount>k__BackingField");
                    MonoClassField* w = pFieldFrom(cPEq, "WeaponSlotTypes");
                    oUMisc = FieldOff(u); oWST = FieldOff(w);
                }
                Log::Infof("[SLOTS] offs storage=%d misc=%d umisc=%d wst=%d", oTU, oMU, oUMisc, oWST);
                s.slotsOk = (oTU >= 0 && oMU >= 0);
                if (!s.slotsOk) Log::Warn("[SLOTS] campos nao resolveram — slots intactos.");
            }
            if (oTU >= 0) {
                unsigned char v = 0;
                memcpy(&v, (char*)pinv + oTU, 1);
                if (!v) { unsigned char t = 1; memcpy((char*)pinv + oTU, &t, 1); }
            }
            if (oMU >= 0) {
                unsigned char v = 0;
                memcpy(&v, (char*)pinv + oMU, 1);
                if (!v) { unsigned char t = 1; memcpy((char*)pinv + oMU, &t, 1); }
            }
            // 3) UnlockedMiscSlotsCount = MiscCount (abre os 4 cadeados do equip).
            // Le o misc atual (peq+oMisc) e escreve o tamanho como desbloqueado.
            if (oUMisc >= 0) {
                void* peq = ReadP(pinv, FieldOff(fEq));
                if (peq) {
                    int cur = ReadI(peq, oUMisc, -1);
                    if (cur >= 0 && cur < 16) {
                        int want = 16; // abre todos os slots de misc
                        if (cur < want) WriteI(peq, oUMisc, want);
                    }
                }
            }
            // 2) WeaponSlotTypes eh STATIC (CE MCP 19/09: off=0 static=true).
            // Nao se le via instancia (peq+0 = vtable, lixo). Estatico = ler
            // via vtable da classe (pVTable + pStaticGet), igual ZLoader.
            // Por enquanto: diagnostico 1x do tamanho real.
            {
                static bool s_wstLogged = false;
                if (!s_wstLogged && cPEq && s_dom) {
                    s_wstLogged = true;
                    __try {
                        MonoVTable* vt = pVTable(s_dom, cPEq);
                        void* lst = nullptr;
                        if (vt && oWST >= 0) {
                            // static List: o field estatico mora na area estatica;
                            // pStaticGet com field estatico resolve o endereco.
                            MonoClassField* fW = pFieldFrom(cPEq, "WeaponSlotTypes");
                            if (fW) pStaticGet(vt, fW, &lst);
                        }
                        int n = -1;
                        if (lst) memcpy(&n, (char*)lst + Off::L_size, sizeof(n));
                        Log::Infof("[SLOTS] WeaponSlotTypes n=%d (static, slots de arma validos).", n);
                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                        Log::Warn("[SLOTS] WeaponSlotTypes: leitura statica falhou.");
                    }
                }
            }
            // 4) GRID util do storage (bug 21/09: pilha recusada 3x = sem
            // espaco fisico; bools desbloqueados mas grid pequeno = AddItem
            // retorna false). SetUsableSize(16,20) via invoke 1x por sessao.
            // IntVec2 = struct {x@+0, y@+4}; metodo (int,int)->void.
            {
                static bool s_gridDone = false;
                if (!s_gridDone) {
                    void* cont = ReadP(pinv, FieldOff(fStorage));
                    if (cont && cPInv) {
                        // Resolve SetUsableSize na classe ItemContainer 1x.
                        static MonoClass* cCont = nullptr;
                        static MonoMethod* mSetU = nullptr;
                        static bool s_mInit = false;
                        if (!s_mInit) {
                            s_mInit = true;
                            ResolveClass("ItemContainer", cCont);
                            if (cCont) mSetU = pMethodFrom(cCont, "SetUsableSize", 2);
                        }
                        if (mSetU) {
                            int w = 16, h = 20; // teto do jogo (dump 21/09)
                            void* args[2] = { &w, &h };
                            __try {
                                MonoObject* exc = nullptr;
                                pInvoke(mSetU, cont, args, &exc);
                                if (!exc) {
                                    s_gridDone = true;
                                    Log::Info("[SLOTS] grid storage 16x20 (pilha cabe).");
                                    // Reabre tentativas: com espaco, o criador
                                    // merece 3 novas chances.
                                    PileTriesReset("grid 16x20");
                                }
                            } __except (EXCEPTION_EXECUTE_HANDLER) {}
                        }
                    }
                }
            }
            s.slotsOn = true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // Defesa (God + Stamina): reescreve os campos do LOCAL player.
    // Roda no bloco rapido da worker (todo ciclo, ~33ms) SOB GATE de mapa
    // (local vivo dentro da partida). Sem hook, sem patch, custo zero desligado.
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

    // Restaura valor original ao desligar (bug 19/09: cheat grudava).
    // Cada Apply permanente (jump, rof, Duration, transition) guarda
    // (alvo, base); ao desligar escreve a base de volta 1x. Declarado AQUI
    // (antes do ApplyWeapon) p/ valer nos dois Applies.
    struct RestoreSlot { void* key; int off; float base; };
    static RestoreSlot s_rsJump[4], s_rsRof[16], s_rsDur[64], s_rsAim[4];
    static RestoreSlot s_rsSway[4]; // gunSway fallback (defeito 8)
    static RestoreSlot s_rsRecoil[16], s_rsSpread[16]; // recoil/spread (GRUDA fix)
    static bool s_swayWas = false;
    static bool s_swayBaseOk = false;
    static void SwayBaseOkReset() { s_swayBaseOk = false; }
    // Guarda base bool (1 byte) p/ restore nao-destrutivo (defeito 7).
    struct RestoreSlotB { void* key; int off; unsigned char base; };
    static RestoreSlotB s_rsFull[16];
    static void RestorePushB(RestoreSlotB* slots, int cap, void* key, int off, unsigned char base) {
        if (!key || off < 0) return;
        for (int k = 0; k < cap; ++k) {
            if (slots[k].key == key && slots[k].off == off) return;
            if (!slots[k].key) { slots[k].key = key; slots[k].off = off; slots[k].base = base; return; }
        }
    }
    static void RestoreRunB(RestoreSlotB* slots, int cap, const char* tag) {
        int n = 0;
        for (int k = 0; k < cap; ++k) {
            if (!slots[k].key) continue;
            unsigned char cur = 0;
            __try { memcpy(&cur, (char*)slots[k].key + slots[k].off, 1); }
            __except (EXCEPTION_EXECUTE_HANDLER) { slots[k].key = nullptr; continue; }
            if (cur != slots[k].base) {
                __try { memcpy((char*)slots[k].key + slots[k].off, &slots[k].base, 1); n++; }
                __except (EXCEPTION_EXECUTE_HANDLER) {}
            }
            slots[k].key = nullptr;
        }
        if (n > 0) Log::Infof("[RESTORE] %s: %d originais.", tag, n);
    }
    static void RestorePush(RestoreSlot* slots, int cap, void* key, int off, float base) {
        if (!key || off < 0) return;
        for (int k = 0; k < cap; ++k) {
            if (slots[k].key == key && slots[k].off == off) return;
            if (!slots[k].key) { slots[k].key = key; slots[k].off = off; slots[k].base = base; return; }
        }
    }
    static void RestoreRun(RestoreSlot* slots, int cap, const char* tag) {
        int n = 0;
        for (int k = 0; k < cap; ++k) {
            if (!slots[k].key) continue;
            float cur = ReadF(slots[k].key, slots[k].off, -99999.0f);
            if (cur != slots[k].base && cur > -99998.0f) {
                if (WriteF(slots[k].key, slots[k].off, slots[k].base)) n++;
            }
            slots[k].key = nullptr;
        }
        if (n > 0) Log::Infof("[RESTORE] %s: %d valores originais.", tag, n);
    }

    // Arma (No Recoil/Spread/Sway + Rapid Fire): escreve nos DADOS de TODAS
    // as armas do inventario + singletons, nunca no player. Por que todas:
    // cada DatabaseGun eh um ASSET compartilhado (o db da arma na mao pode
    // ser o mesmo de outra no inventario; e trocar de arma troca o db).
    // Travar so a equipada = metade das armas sem efeito.
    // Roda na worker sob gate de mapa (local vivo), so com flag ligada.
    // Falha de resolve = desliga sozinho (log 1x), sem travar.
    static bool s_weapOk = true;   // legado (defeito 6: retry substitui)
    static bool s_weapLogged = false;
    static void ApplyWeapon(void* local) {
        if (!local) return;
        bool want = Config::bNoRecoil || Config::bNoSpread || Config::bNoSway
            || Config::bTightAim || Config::bRapidFire;
        // Restore ANTES do early-return (auditoria 21/09: desligar tudo era
        // inalcançavel). Cada restore roda com sua flag desligada.
        if (!Config::bRapidFire) { RestoreRun(s_rsRof, 16, "rof"); RestoreRunB(s_rsFull, 16, "fullauto"); }
        if (!Config::bTightAim) RestoreRun(s_rsAim, 4, "aim");
        if (!Config::bNoSway) { RestoreRun(s_rsSway, 4, "sway"); SwayBaseOkReset(); }
        if (!Config::bNoRecoil && !Config::bTightAim) RestoreRun(s_rsRecoil, 16, "recoil");
        if (!Config::bNoSpread && !Config::bTightAim) RestoreRun(s_rsSpread, 16, "spread");
        if (!Config::bFastKnife) RestoreRun(s_rsDur, 64, "knife");
        if (!Config::bSuperJump) RestoreRun(s_rsJump, 4, "jump");
        if (!want) return;
        // Resolve preguiçoso 1x (offsets via API; singleton via vtable).
        static int oAmmoId = -2, oMax = -2, oSpread = -2, oRof = -2;
        static int oRecoil = -2, oRecoilRnd = -2;
        static int oFull = -2, oBurst = -2;
        static int oPrec = -2, oSway = -2, oBoolVal = -2;
        static int oGen = -2, oDisSway = -2;
        static int oWep = -2;                       // PlayerEquippedItems.weapons
        static MonoClassField* fWep = nullptr;
        static void* s_wbase = nullptr;    // WeaponBase.instance (cache)
        static void* s_dbgGen = nullptr;   // DebugGeneralModifiers (cache)
        if (oSpread == -2) {
            oAmmoId = FieldOff(fAmmo); oMax = FieldOff(fMaxAmmo);
            oSpread = FieldOff(fGunSpread); oRof = FieldOff(fGunRof);
            oRecoil = FieldOff(fGunRecoil); oRecoilRnd = FieldOff(fGunRecoilRnd);
            oFull = FieldOff(fGunFullAuto); oBurst = FieldOff(fGunBurst);
            oPrec = FieldOff(fPrecMult); oSway = FieldOff(fGunSway);
            oBoolVal = FieldOff(fBoolVal);
            oGen = FieldOff(fDbgGen); oDisSway = FieldOff(fDisSway);
            if (cPEq) { fWep = pFieldFrom(cPEq, "weapons"); oWep = FieldOff(fWep); }
            // Defeito 5 (auditoria 21/09): singletons com retry — se a 1a
            // resolucao pegou cena incompleta, tenta de novo (nao congela).
            if (!s_wbase && cWBase && s_dom) {
                __try {
                    MonoVTable* vt = pVTable(s_dom, cWBase);
                    if (vt && fWBaseInst) pStaticGet(vt, fWBaseInst, &s_wbase);
                } __except (EXCEPTION_EXECUTE_HANDLER) { s_wbase = nullptr; }
            }
            if (!s_dbgGen && cDbgMod && s_dom && oGen >= 0) {
                __try {
                    MonoVTable* vt = pVTable(s_dom, cDbgMod);
                    void* genHolder = nullptr;
                    if (vt && fDbgGen) pStaticGet(vt, fDbgGen, &genHolder);
                    // General pode ser field estatico direto ou instancia com
                    // campo: tenta os dois (SEH cobre).
                    if (genHolder && cDbgGen && oDisSway >= 0) {
                        void* ds = ReadP(genHolder, oDisSway);
                        if (ds) {
                            s_dbgGen = genHolder;
                            Log::Info("[WEAPON] DebugGeneralModifiers resolvido (retry).");
                        }
                    }
                } __except (EXCEPTION_EXECUTE_HANDLER) {}
            }
            if (!s_weapLogged) {
                s_weapLogged = true;
                Log::Infof("[WEAPON] offs spread=%d rof=%d recoil=%d recoilRnd=%d prec=%d sway=%d boolVal=%d wep=%d wbase=%d dbg=%d",
                    oSpread, oRof, oRecoil, oRecoilRnd, oPrec, oSway, oBoolVal, oWep,
                    s_wbase ? 1 : 0, s_dbgGen ? 1 : 0);
            }
            if (oSpread < 0 && oRof < 0 && oRecoil < 0 && oBoolVal < 0) {
                // Retry com throttle (sem morte permanente).
                static int s_weapFailN = 0;
                if (++s_weapFailN > 500) {
                    Log::Warn("[WEAPON] cadeia incompleta apos 500 ciclos — arma em espera (troque de cena).");
                    s_weapFailN = 0;
                }
                return;
            }
        }
        __try {
            // Dbs de TODAS as armas: inventory -> equippedItems -> weapons[]
            // -> item -> GetDataBaseItem (cache DbCached). Rof/recoil/spread
            // sao do ASSET: 1 db pode servir 2 armas iguais.
            // Defeito 1 (auditoria 21/09): SEM dedup permanente — escreve
            // idempotente todo ciclo (reaplica apos troca de cena/loja).
            // Custo: poucas armas x 3 floats, irrelevante.
            void* pinv = ReadP(local, FieldOff(fInv));
            void* peq = pinv ? ReadP(pinv, FieldOff(fEq)) : nullptr;
            void* list = (peq && oWep >= 0) ? ReadP(peq, oWep) : nullptr;
            // Rapid Fire MINIGUN (pedido 19/09): pistola 1-tiro vira metralha.
            // fullAuto+burst ja deixam segurar; rof extremo dita a cadencia:
            // teto 5x -> 8x no mult (rof 6 = 48/s; Cooldown zerado libera).
            // Slider continua 1-5x no menu; interno aceita ate 8x.
            float mult = Config::fRapidMult;
            if (!(mult >= 1.0f && mult <= 8.0f)) mult = 2.0f;
            if (list && mGetDb) {
                static int s_weapNLogged = -1;
                int dbgN = 0;
                __try {
                    int sz = 0;
                    memcpy(&sz, (char*)list + Off::L_size, sizeof(sz));
                    dbgN = sz;
                } __except (EXCEPTION_EXECUTE_HANDLER) {}
                if (dbgN != s_weapNLogged) {
                    s_weapNLogged = dbgN;
                    Log::Infof("[WEAPON] weapons[] n=%d (aplica recoil/spread/rapid em todas).", dbgN);
                }
                WalkList(list, 32, [&](void* it, int) {
                    void* db = DbCached(it);
                    if (!db) return;
                // Sem dedup (defeito 1): reaplica todo ciclo (idempotente).
                // No Recoil: zera recoil (Vector2 = 2 floats) + randomness.
                // Mira Fechada inclui recoil zero (mira nao abre atirando).
                // Restore: guarda base (GRUDA fix 21/09).
                if (Config::bNoRecoil || Config::bTightAim) {
                    if (oRecoil >= 0) {
                        float rx = ReadF(db, oRecoil, -99999.0f);
                        float ry = ReadF(db, oRecoil + 4, -99999.0f);
                        if (rx > -99998.0f) RestorePush(s_rsRecoil, 16, db, oRecoil, rx);
                        if (ry > -99998.0f) RestorePush(s_rsRecoil, 16, db, oRecoil + 4, ry);
                        float z = 0.0f;
                        __try {
                            memcpy((char*)db + oRecoil, &z, sizeof(z));
                            memcpy((char*)db + oRecoil + 4, &z, sizeof(z));
                        } __except (EXCEPTION_EXECUTE_HANDLER) {}
                    }
                    if (oRecoilRnd >= 0) {
                        float rr = ReadF(db, oRecoilRnd, -99999.0f);
                        if (rr != 0.0f && rr > -99998.0f) {
                            RestorePush(s_rsRecoil, 16, db, oRecoilRnd, rr);
                            WriteF(db, oRecoilRnd, 0.0f);
                        }
                    }
                }
                // No Spread: spread=0 (GetGunSpread = 0 x prec = 0).
                // Mira Fechada inclui spread zero (tiro vai junto).
                // Restore: guarda base (GRUDA fix 21/09).
                if ((Config::bNoSpread || Config::bTightAim) && oSpread >= 0) {
                    float sp = ReadF(db, oSpread, -99999.0f);
                    if (sp != 0.0f && sp > -99998.0f) {
                        RestorePush(s_rsSpread, 16, db, oSpread, sp);
                        WriteF(db, oSpread, 0.0f);
                    }
                }
                // Full Auto universal: 1-tiro/rajada vira automatica.
                // fullAuto=true + burstCount=0 em TODA arma (asset): pistola
                // 12/12, Riot 1/1, sniper — todas seguram o gatilho.
                // Defeito 7: restore NAO-destrutivo (guarda base 1 byte).
                if (Config::bRapidFire) {
                    if (oFull >= 0) {
                        __try {
                            unsigned char fa = 0;
                            memcpy(&fa, (char*)db + oFull, 1);
                            RestorePushB(s_rsFull, 16, db, oFull, fa);
                            if (!fa) {
                                unsigned char t = 1;
                                memcpy((char*)db + oFull, &t, 1);
                            }
                        } __except (EXCEPTION_EXECUTE_HANDLER) {}
                    }
                    if (oBurst >= 0) {
                        int bc = ReadI(db, oBurst, -1);
                        if (bc != 0 && bc >= 0 && bc < 100) WriteI(db, oBurst, 0);
                    }
                } else {
                    RestoreRunB(s_rsFull, 16, "fullauto");
                }
                // Rapid Fire: rof x mult em TODA arma (ate 1 tiro:
                // BaseCooldownTime=1/rof cai p/ todas). Base guardada por db
                // + restore ao desligar (volta ao rof original).
                if (Config::bRapidFire && oRof >= 0) {
                    float cur = ReadF(db, oRof, -1.0f);
                    static void* s_rofDb[16] = { nullptr };
                    static float s_rofBase[16] = { 0 };
                    int slot = -1, freeSlot = -1;
                    for (int k = 0; k < 16; ++k) {
                        if (s_rofDb[k] == db) { slot = k; break; }
                        if (freeSlot < 0 && !s_rofDb[k]) freeSlot = k;
                    }
                    if (slot < 0 && cur > 0.0f && cur < 1000.0f && freeSlot >= 0) {
                        slot = freeSlot;
                        s_rofDb[slot] = db;
                        s_rofBase[slot] = cur;
                        RestorePush(s_rsRof, 16, db, oRof, cur);
                    }
                    if (slot >= 0 && s_rofBase[slot] > 0.0f) {
                        float want = s_rofBase[slot] * mult;
                        if (want > 0.0f && want < 5000.0f && cur != want)
                            WriteF(db, oRof, want);
                    }
                }
            });
            // Rapid Fire universal (parte 2): Cooldown zerado na prop atual.
            // Defeito 10: resolve 1x (static) — antes pFieldFrom+pMethodFrom
            // todo ciclo a 30Hz (custo + risco de hang).
            if (Config::bRapidFire && cArms && cPlayer) {
                static int oCd = -2, oArmsEq = -2;
                static MonoClassField* fCd = nullptr;
                static MonoClassField* fArmsEq = nullptr;
                static MonoMethod* s_mEqG = nullptr;
                if (oCd == -2) {
                    oCd = -1; oArmsEq = -1;
                    MonoClass* cPG = nullptr;
                    if (ResolveClass("PhysicalGun", cPG) && cPG) {
                        fCd = pFieldFrom(cPG, "<Cooldown>k__BackingField");
                        if (!fCd) fCd = pFieldFrom(cPG, "Cooldown");
                        oCd = FieldOff(fCd);
                    }
                    fArmsEq = pFieldFrom(cPlayer, "arms");
                    oArmsEq = FieldOff(fArmsEq);
                    s_mEqG = pMethodFrom(cArms, "get_EquippedGun", 0);
                }
                if (oCd >= 0 && oArmsEq >= 0 && s_mEqG) {
                    void* arms = ReadP(local, oArmsEq);
                    if (arms) {
                        void* gun = InvokeObj(s_mEqG, arms, nullptr);
                        if (gun) {
                            float cd = ReadF(gun, oCd, -99.0f);
                            // Cooldown>0 = esperando: libera (ResetCooldown
                            // grava -0.001 no jogo; aqui direto = igual).
                            if (cd > 0.0f && cd < 100.0f)
                                WriteF(gun, oCd, -0.001f);
                        }
                    }
                }
            }
            // Restore central ao desligar: Rapid Fire e Fast Knife voltam ao
            // original assim que a flag cai (o bloco de cada um so roda
            // ligado; aqui roda sempre p/ detectar a borda de descida).
            if (!Config::bRapidFire) RestoreRun(s_rsRof, 16, "rof");
            // Mira Fechada (visual): crosshair junto via PlayerHUD.
            // UpdateCrosshairSizeBasedOnAccuracy abre o crosshair pelo spread;
            // com o tiro ja zerado (spread/recoil acima), trava o visual no
            // minimo: innerCrossHairTransform em escala minima 1x/ciclo.
            // Restore: ao desligar, volta a escala guardada.
            if (Config::bTightAim && cHud && s_dom) {
                static int oHudInner = -2;
                if (oHudInner == -2) {
                    oHudInner = -1;
                    oHudInner = FieldOff(fHudInner);
                    Log::Infof("[AIM] offs innerCross=%d", oHudInner);
                }
                if (oHudInner >= 0) {
                    __try {
                        void* hinst = nullptr;
                        if (StaticInstance(cHud, fHudInst, hinst) && hinst) {
                            void* inner = ReadP(hinst, oHudInner);
                            if (inner) {
                                static int oScale = -2;
                                if (oScale == -2) {
                                    oScale = -1;
                                    if (cTrans) {
                                        MonoClassField* fs = pFieldFrom(cTrans, "m_LocalScale");
                                        if (!fs) fs = pFieldFrom(cTrans, "localScale");
                                        oScale = FieldOff(fs);
                                    }
                                    Log::Infof("[AIM] offs localScale=%d", oScale);
                                }
                                if (oScale >= 0) {
                                    float sx = ReadF(inner, oScale, -1.0f);
                                    static float s_sxBase = -1.0f;
                                    if (s_sxBase < 0.0f && sx > 0.0f && sx < 100.0f) {
                                        s_sxBase = sx;
                                        RestorePush(s_rsAim, 4, inner, oScale, sx);
                                        float sy = ReadF(inner, oScale + 4, -1.0f);
                                        RestorePush(s_rsAim, 4, inner, oScale + 4, sy);
                                    }
                                    // Trava no minimo da base (metade): junto.
                                    if (s_sxBase > 0.0f) {
                                        float want = s_sxBase * 0.5f;
                                        if (sx != want && want > 0.0f)
                                            WriteF(inner, oScale, want);
                                    }
                                }
                            }
                        }
                    } __except (EXCEPTION_EXECUTE_HANDLER) {}
                }
            } else if (!Config::bTightAim) {
                RestoreRun(s_rsAim, 4, "aim");
            }
            }
            // Restore: value volta a false; gunSway volta a base guardada.
            if (!Config::bNoSway) {
                if (s_swayWas) {
                    s_swayWas = false;
                    if (s_dbgGen && oDisSway >= 0 && oBoolVal >= 0) {
                        __try {
                            void* ds = ReadP(s_dbgGen, oDisSway);
                            if (ds && ReadI(ds, oBoolVal, -1) == 1)
                                WriteI(ds, oBoolVal, 0);
                        } __except (EXCEPTION_EXECUTE_HANDLER) {}
                    }
                    Log::Info("[RESTORE] sway: DisableSway=false.");
                }
            }
            if (Config::bNoSway) {
                s_swayWas = true;
                bool done = false;
                if (s_dbgGen && oDisSway >= 0 && oBoolVal >= 0) {
                    __try {
                        void* ds = ReadP(s_dbgGen, oDisSway);
                        if (ds) {
                            // Defeito 9: bool tem 1 byte — memcpy, nao WriteI
                            // (4 bytes corrompia vizinhos).
                            unsigned char cur = 0;
                            memcpy(&cur, (char*)ds + oBoolVal, 1);
                            if (cur == 0) {
                                unsigned char t = 1;
                                memcpy((char*)ds + oBoolVal, &t, 1);
                            }
                            done = true;
                        }
                    } __except (EXCEPTION_EXECUTE_HANDLER) {}
                }
                // Defeito 8: fallback gunSway com restore (antes grudava 0).
                if (!done && s_wbase && oSway >= 0) {
                    float cur = ReadF(s_wbase, oSway, -1.0f);
                    if (cur != 0.0f && cur > -99998.0f) {
                        static bool s_swayBaseOk = false;
                        if (!s_swayBaseOk) {
                            s_swayBaseOk = true;
                            RestorePush(s_rsSway, 4, s_wbase, oSway, cur);
                        }
                        WriteF(s_wbase, oSway, 0.0f);
                    }
                }
            }
            if (!Config::bNoSway) RestoreRun(s_rsSway, 4, "sway");
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static bool s_moveOk = true;
    static bool s_moveLogged = false;
    static void ApplyMove(void* local) {
        if (!local) return;
        bool want = Config::bSuperJump || Config::bFastKnife;
        if (!want || !s_moveOk) return;
        static int oMove = -2, oJump = -2, oDur = -2;
        static MonoClassField* fMove = nullptr;
        if (oJump == -2) {
            oJump = -1; oMove = -1; oDur = -1;
            if (cPlayer) fMove = pFieldFrom(cPlayer, "movement");
            oMove = FieldOff(fMove);
            oJump = FieldOff(fMoveJump);
            oDur = FieldOff(fAtkDur);
            if (!s_moveLogged) {
                s_moveLogged = true;
                Log::Infof("[MOVE] offs move=%d jump=%d dur=%d", oMove, oJump, oDur);
            }
            if ((Config::bSuperJump && (oMove < 0 || oJump < 0)) ||
                (Config::bFastKnife && oDur < 0)) {
                Log::Warn("[MOVE] cadeia incompleta — movimento desativado (sem crash).");
                s_moveOk = false;
                return;
            }
        }
        __try {
            // Super Pulo: jumpSpeed x fJumpMult (guarda base 1x + restore).
            // Teto 10x (slider). Altura fisica = v²/2g: jumpSpeed 6→60 =
            // ~10x a altura (nao linear). Anti-podador: LimitVerticalVelocity
            // corta em -fallDamageThreshold — sobe o threshold junto (guarda
            // base + restore). Sem isso 10x nunca voa (bug 19/09).
            // FIX 19/09 (pulo fraco mesmo no max): o write FALHAVA silencioso
            // porque cur==want na comparacao (float ja arredondado) — agora
            // reescreve se |cur-want|>0.01. E loga 1x o estado real.
            if (Config::bSuperJump && oMove >= 0 && oJump >= 0) {
                void* mv = ReadP(local, oMove);
                if (mv) {
                    float mult = Config::fJumpMult;
                    if (!(mult >= 1.0f && mult <= 10.0f)) mult = 1.5f;
                    static float s_jumpBase = -1.0f;
                    static float s_fallBase = -1.0f;
                    static int oFall = -2;
                    if (oFall == -2) {
                        oFall = -1;
                        if (cMove) {
                            MonoClassField* ff = pFieldFrom(cMove, "fallDamageThreshold");
                            oFall = FieldOff(ff);
                        }
                        Log::Infof("[JUMP] offs fall=%d", oFall);
                    }
                    float cur = ReadF(mv, oJump, -1.0f);
                    if (s_jumpBase < 0.0f && cur > 0.0f && cur < 100.0f) {
                        s_jumpBase = cur;
                        RestorePush(s_rsJump, 4, mv, oJump, cur);
                        Log::Infof("[JUMP] base=%.2f mult=%.1f want=%.2f", (double)cur, (double)mult, (double)(cur * mult));
                    }
                    if (s_jumpBase > 0.0f) {
                        float want = s_jumpBase * mult;
                        float d = cur - want; if (d < 0) d = -d;
                        if (want > 0.0f && want < 1000.0f && d > 0.01f) {
                            if (WriteF(mv, oJump, want)) {
                                static int s_jumpLogged = 0;
                                if (s_jumpLogged < 2) {
                                    s_jumpLogged++;
                                    Log::Infof("[JUMP] aplicado want=%.2f (cur era %.2f).", (double)want, (double)cur);
                                }
                            }
                        }
                    }
                    // Anti-podador: threshold acompanha o mult (queda de 100m
                    // continua sem dano de queda — bonus, nao custo).
                    if (oFall >= 0) {
                        float fc = ReadF(mv, oFall, -1.0f);
                        if (s_fallBase < 0.0f && fc > 0.0f && fc < 1000.0f) {
                            s_fallBase = fc;
                            RestorePush(s_rsJump, 4, mv, oFall, fc);
                        }
                        if (s_fallBase > 0.0f) {
                            float fw = s_fallBase * mult;
                            if (fw > 0.0f && fw < 10000.0f && fc != fw)
                                WriteF(mv, oFall, fw);
                        }
                    }
                }
            } else if (!Config::bSuperJump) {
                RestoreRun(s_rsJump, 4, "jump");
            }
            // Fast Knife: TODA arma branca via base global (pa, pa, facao,
            // faca, taco, cano). Caminho: MeleeAttackBase.Instance.AllAttacks
            // (Dictionary ID->PlayerMeleeAttack) — cobre tudo sem depender da
            // mao. Dict Mono: entries[] vetor de {key, value}; value =
            // PlayerMeleeAttack -> Duration / mult. Alem disso mantem o
            // caminho do MoveSet da mao (transitionTime, chain rapido).
            if (Config::bFastKnife) {
                float multK = Config::fKnifeMult;
                if (!(multK >= 1.0f && multK <= 5.0f)) multK = 2.0f;
                // 1) Base global: AllAttacks (resolve 1x, static).
                static int oAll = -2;
                if (oAll == -2) {
                    oAll = -1;
                    oAll = FieldOff(fAtkAll);
                    Log::Infof("[KNIFE] offs all=%d dur=%d", oAll, oDur);
                    if (oAll < 0)
                        Log::Warn("[KNIFE] AllAttacks sem offset — so MoveSet da mao.");
                }
                if (oAll >= 0 && cAtkBase && s_dom) {
                    __try {
                        void* inst = nullptr;
                        if (StaticInstance(cAtkBase, fAtkInst, inst) && inst) {
                            void* dict = ReadP(inst, oAll);
                            if (dict) {
                                // Dictionary<K,V>: entries = campo 1 (entries[]
                                // apos buckets). Layout Mono: count@20,
                                // entries@24 (confirmar via log [KNIFE-DICT]).
                                static int oEnt = -2;
                                if (oEnt == -2) {
                                    oEnt = -1;
                                    // Descobre entries pelo nome (APIpcional).
                                    MonoClass* cDict = nullptr;
                                    (void)cDict;
                                    // Fallback: varre ponteiros do dict (2
                                    // candidatos: buckets@16, entries@24).
                                    // Testa cada um como vetor Mono valido.
                                    for (int cand = 16; cand <= 32; cand += 8) {
                                        void* arr = ReadP(dict, cand);
                                        if (!arr) continue;
                                        long long ln = 0;
                                        __try { memcpy(&ln, (char*)arr + Off::A_len, sizeof(ln)); }
                                        __except (EXCEPTION_EXECUTE_HANDLER) { continue; }
                                        if (ln > 0 && ln < 256) {
                                            // Vetor plausivel: checa 1a entry
                                            // como {key(int), value(ptr)}.
                                            void* v0 = nullptr;
                                            __try { memcpy(&v0, (char*)arr + Off::A_data + 8, 8); }
                                            __except (EXCEPTION_EXECUTE_HANDLER) { continue; }
                                            if (v0) {
                                                float d0 = ReadF(v0, oDur, -1.0f);
                                                if (d0 > 0.05f && d0 < 10.0f) {
                                                    oEnt = cand;
                                                    Log::Infof("[KNIFE-DICT] entries@%d len=%d atk0 dur=%.2f", cand, (int)ln, (double)d0);
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                    if (oEnt < 0)
                                        Log::Warn("[KNIFE-DICT] entries nao localizado (dict vazio ou layout novo).");
                                }
                                if (oEnt >= 0) {
                                    void* arr = ReadP(dict, oEnt);
                                    long long len = 0;
                                    __try { if (arr) memcpy(&len, (char*)arr + Off::A_len, sizeof(len)); }
                                    __except (EXCEPTION_EXECUTE_HANDLER) { len = 0; }
                                    // Entry .NET: { hashCode(i32), next(i32),
                                    // key, value }. key=PlayerMeleeAttackID
                                    // (byte->int), value=ptr attack (+16?).
                                    // Descobre stride: entry = 24 bytes
                                    // (4+4+4pad+8? ou 4+4+8+8=24).
                                    int wrote = 0;
                                    for (long long k = 0; k < len && k < 64; ++k) {
                                        char* en = nullptr;
                                        __try { en = (char*)arr + Off::A_data + (size_t)k * 24; }
                                        __except (EXCEPTION_EXECUTE_HANDLER) { break; }
                                        void* atk = nullptr;
                                        __try { memcpy(&atk, en + 16, 8); }
                                        __except (EXCEPTION_EXECUTE_HANDLER) { continue; }
                                        if (!atk) continue;
                                        float cur = ReadF(atk, oDur, -1.0f);
                                        if (cur > 0.05f && cur < 10.0f) {
                                            float want = cur / multK;
                                            static void* s_atkG[64] = { nullptr };
                                            static float s_wantG[64] = { 0 };
                                            int si = (int)(k % 64);
                                            if (s_atkG[si] != atk || s_wantG[si] != want) {
                                                s_atkG[si] = atk; s_wantG[si] = want; RestorePush(s_rsDur, 64, atk, oDur, cur);
                                                if (WriteF(atk, oDur, want)) wrote++;
                                            }
                                        }
                                    }
                                    static int s_knifeG = 0;
                                    if (wrote > 0 && s_knifeG < 3) {
                                        s_knifeG++;
                                        Log::Infof("[KNIFE] base global: %d golpes (x%.1f).", wrote, (double)multK);
                                    }
                                }
                            }
                        }
                    } __except (EXCEPTION_EXECUTE_HANDLER) {}
                }
                // 2) MoveSet da mao: transitionTime + Duration (chain rapido
                // na arma atual). Resolve 1x (static). Reusa multK do bloco.
                static int oMS = -2, oNodes = -2;
                if (oMS == -2) {
                    oMS = -1; oNodes = -1;
                    MonoClass* cPM = nullptr;
                    if (ResolveClass("PhysicalMelee", cPM) && cPM) {
                        MonoClassField* f = pFieldFrom(cPM, "<MoveSet>k__BackingField");
                        if (!f) f = pFieldFrom(cPM, "MoveSet");
                        oMS = FieldOff(f);
                    }
                    MonoClassField* fn = pFieldFrom(cMoveSet, "nodes");
                    oNodes = FieldOff(fn);
                    Log::Infof("[KNIFE] offs moveset=%d nodes=%d dur=%d", oMS, oNodes, oDur);
                    if (oMS < 0 || oNodes < 0)
                        Log::Warn("[KNIFE] cadeia incompleta — fast knife parcial (pulo segue).");
                }
                if (oMS >= 0 && oNodes >= 0 && cArms && cPlayer) {
                    // EquippedMelee via arms (resolve metodo 1x, static).
                    static MonoMethod* s_mEqM = nullptr;
                    static bool s_mEqMInit = false;
                    if (!s_mEqMInit) {
                        s_mEqMInit = true;
                        s_mEqM = pMethodFrom(cArms, "get_EquippedMelee", 0);
                    }
                    MonoClassField* fa = pFieldFrom(cPlayer, "arms");
                    void* arms = (fa) ? ReadP(local, FieldOff(fa)) : nullptr;
                    void* pm = (arms && s_mEqM) ? InvokeObj(s_mEqM, arms, nullptr) : nullptr;
                    static int s_pmLogged = 0;
                    if (!pm && s_pmLogged < 2) {
                        s_pmLogged++;
                        Log::Infof("[KNIFE] EquippedMelee=null (arma de fogo/fists?) arms=0x%p", arms);
                    }
                    // Sem arma branca na mao (arma de fogo/fists) = pm null:
                    // nao escreve nada (estado normal, nao erro).
                    if (pm) {
                        void* ms = ReadP(pm, oMS);
                        void* arr = ms ? ReadP(ms, oNodes) : nullptr;
                        static int s_msLogged = 0;
                        if (!arr && s_msLogged < 2) {
                            s_msLogged++;
                            Log::Infof("[KNIFE] moveset=0x%p nodes=null (pm=0x%p)", ms, pm);
                        }
                        if (arr) {
                            long long len = 0;
                            __try { memcpy(&len, (char*)arr + Off::A_len, sizeof(len)); }
                            __except (EXCEPTION_EXECUTE_HANDLER) { len = 0; }
                            int wrote = 0;
                            for (long long k = 0; k < len && k < 16; ++k) {
                                char* node = nullptr;
                                __try { node = (char*)arr + Off::A_data + (size_t)k * 32; }
                                __except (EXCEPTION_EXECUTE_HANDLER) { break; }
                                if (!node) continue;
                                void* atk = nullptr;
                                float tmin = -1, tmax = -1;
                                __try {
                                    memcpy(&atk, node, 8);
                                    memcpy(&tmin, node + 8, 4);
                                    memcpy(&tmax, node + 12, 4);
                                } __except (EXCEPTION_EXECUTE_HANDLER) { continue; }
                                // 1) janela de chain: tMin/tMax pelo mult.
                                if (tmin > 0.01f && tmin < 10.0f) {
                                    float w = tmin / multK;
                                    static float s_lastT[16] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
                                    int si = (int)(k % 16);
                                    if (s_lastT[si] != w) {
                                        s_lastT[si] = w;
                                        float b0 = -1;
                                        __try { memcpy(&b0, node + 8, 4); } __except (EXCEPTION_EXECUTE_HANDLER) {}
                                        RestorePush(s_rsDur, 64, node + 8, 0, b0);
                                        __try { memcpy(node + 8, &w, 4); wrote++; }
                                        __except (EXCEPTION_EXECUTE_HANDLER) {}
                                    }
                                }
                                if (tmax > 0.01f && tmax < 10.0f) {
                                    float w = tmax / multK;
                                    static float s_lastTx[16] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
                                    int si = (int)(k % 16);
                                    if (s_lastTx[si] != w) {
                                        s_lastTx[si] = w;
                                        float b1 = -1;
                                        __try { memcpy(&b1, node + 12, 4); } __except (EXCEPTION_EXECUTE_HANDLER) {}
                                        RestorePush(s_rsDur, 64, node + 12, 0, b1);
                                        __try { memcpy(node + 12, &w, 4); wrote++; }
                                        __except (EXCEPTION_EXECUTE_HANDLER) {}
                                    }
                                }
                                // 2) duracao do golpe: Duration / mult.
                                if (atk && oDur >= 0) {
                                    float cur = ReadF(atk, oDur, -1.0f);
                                    if (cur > 0.05f && cur < 10.0f) {
                                        float want = cur / multK;
                                        static void* s_atk[16] = { nullptr };
                                        static float s_want[16] = { 0 };
                                        int si = (int)(k % 16);
                                        if (s_atk[si] != atk || s_want[si] != want) {
                                            s_atk[si] = atk; s_want[si] = want; RestorePush(s_rsDur, 64, atk, oDur, cur);
                                            if (WriteF(atk, oDur, want)) wrote++;
                                        }
                                    }
                                }
                            }
                            static int s_knifeLogged = 0;
                            if (wrote > 0 && s_knifeLogged < 3) {
                                s_knifeLogged++;
                                Log::Infof("[KNIFE] %d campos acelerados (x%.1f).", wrote, (double)multK);
                            }
                        }
                    }
                }
            }
            // Restore knife ao desligar: Duration/transition voltam ao base.
            if (!Config::bFastKnife) RestoreRun(s_rsDur, 64, "knife");
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // ========================================================================
    // AIMBOT ciclo 1 (23/09): selecao + FOV + escrita de camera.
    // PlayerCamera = pitch (set_Angle, clamp ±80 no jogo) + yaw (Rotate no
    // transform do PlayerCamera). Nunca no CameraTransform (sobrescrito).
    // Alvo = snapshot do ESP (BuildEsp ja validou: vivo, on-screen, dist).
    // Ordem mercado: valido → FOV px → prioridade → smoothing → escreve.
    // Ciclo 1 = visivel, sem silent/trigger/prediction (ciclos 2-4).
    // ========================================================================
    static bool s_aimOk = true;   // false = cadeia incompleta (retry, sem morte)
    static bool s_aimLogged = false;
    static void ApplyAim(void* local) {
        if (!local) return;
        bool want = Config::bAimbot || Config::bAutoAim;
        if (!want) return;
        // Tecla: Hold (segurando) ou Toggle (travado). Sem tecla = AutoAim
        // mira sozinho; com tecla = exige pressionada (padrao mercado).
        bool keyDown = false;
        if (Config::bAutoAim && !Config::bAimbot) {
            keyDown = true; // auto: sem tecla
        } else {
            int vk = Config::iAimKey;
            if (vk == 0) keyDown = true;
            else keyDown = (GetAsyncKeyState(vk) & 0x8000) != 0;
            if (Config::iAimMode == 1) { // Toggle: trava no 1o aperto
                static bool s_toggle = false;
                static bool s_prevDown = false;
                if (keyDown && !s_prevDown) s_toggle = !s_toggle;
                s_prevDown = keyDown;
                keyDown = s_toggle;
            }
        }
        if (!keyDown) return;
        // Resolve 1x (offsets via API; metodos set_Angle/get_Angle).
        static int oCam = -2, oAngle = -2;
        if (oCam == -2) {
            oCam = -1; oAngle = -1;
            if (cPlayer) oCam = FieldOff(fMainCam);
            oAngle = FieldOff(fCamAngle);
            if (!s_aimLogged) {
                s_aimLogged = true;
                Log::Infof("[AIMBOT] offs cam=%d angle=%d setAngle=%d getAngle=%d",
                    oCam, oAngle, mSetAngle ? 1 : 0, mGetAngle ? 1 : 0);
            }
            if (oCam < 0 || !mSetAngle) {
                Log::Warn("[AIMBOT] cadeia incompleta — aimbot em espera.");
                s_aimOk = false;
                return;
            }
        }
        if (oCam < 0 || !mSetAngle) return;
        __try {
            // PlayerCamera do LOCAL (PlayerMain.cam), nao singleton.
            void* pcam = ReadP(local, oCam);
            if (!pcam) return;
            // Snapshot: melhor alvo por prioridade (le o front, sem lock —
            // mesmo padrao do Present; 1 frame velho no pior caso).
            EspEntry* f = s_espFront;
            int n = s_espNFront;
            if (n < 0) n = 0;
            if (n > 128) n = 128;
            if (n <= 0) return;
            float cx = s_vpW * 0.5f, cy = s_vpH * 0.5f;
            float fovPx = Config::bLimitFov && !Config::b360Mode
                ? Config::fFovAngle * 4.0f : 1e9f;
            if (fovPx < 30.0f) fovPx = 30.0f;
            int best = -1;
            float bestScore = 1e30f;
            for (int i = 0; i < n; ++i) {
                const EspEntry& e = f[i];
                if (!e.onScreen || e.hp <= 0) continue;
                // Posicao do bone por iAimBone: head (padrao) / neck / chest /
                // pelvis. Skeleton tem as 4 regioes: HEAD=0, NECK=1, peito =
                // media SP2/SP1, quadril = HL. Sem skeleton = head/foot.
                float tx = 0, ty = 0;
                bool hasBone = false;
                if (e.skN == SK_COUNT) {
                    int b = SK_HEAD;
                    if (Config::iAimBone == 1) b = SK_NECK;
                    else if (Config::iAimBone == 2) b = SK_SP2;   // chest
                    else if (Config::iAimBone == 3) b = SK_HL;    // pelvis
                    if (b >= 0 && b < 20 && e.skV[b]) {
                        tx = e.skX[b]; ty = s_vpH - e.skY[b];
                        hasBone = true;
                    }
                }
                if (!hasBone) {
                    // Fallback sem skeleton: topo da box (head aprox).
                    tx = (e.headX + e.footX) * 0.5f;
                    ty = s_vpH - e.headY;
                    if (!(tx > -10000 && tx < 10000 && ty > -10000 && ty < 10000))
                        continue;
                }
                float dx = tx - cx, dy = ty - cy;
                float dPx = sqrtf(dx * dx + dy * dy);
                if (dPx > fovPx) continue; // fora do FOV
                if (Config::fMaxDistance > 0 && e.dist > Config::fMaxDistance)
                    continue;
                float score = 1e30f;
                if (Config::iAimPriority == 2) score = e.dist;        // Nearest
                else if (Config::iAimPriority == 1) score = e.hp;     // LowestHP
                else score = dPx;                                     // Crosshair
                if (score < bestScore) { bestScore = score; best = i; }
            }
            if (best < 0) return;
            // Converte pixel -> yaw/pitch via matriz VP inversa implicita:
            // usa a posicao 3D do bone (mundo) + posicao da camera.
            // Caminho barato e exato: W2S reverso via razao angular —
            // pitch = Angle atual + atan2(dy_px, H/2 / tan(fov/2)).
            // Sem FOV vertical real: aproxima com sensibilidade angular
            // medida (graus por pixel a 1080p). Erro < 1 grau no centro.
            const EspEntry& t = f[best];
            float dx = 0, dy = 0;
            {
                float tx = 0, ty = 0;
                if (t.skN == SK_COUNT) {
                    int b = SK_HEAD;
                    if (Config::iAimBone == 1) b = SK_NECK;
                    else if (Config::iAimBone == 2) b = SK_SP2;
                    else if (Config::iAimBone == 3) b = SK_HL;
                    if (b >= 0 && b < 20 && t.skV[b]) {
                        tx = t.skX[b]; ty = s_vpH - t.skY[b];
                    } else { tx = (t.headX + t.footX) * 0.5f; ty = s_vpH - t.headY; }
                } else { tx = (t.headX + t.footX) * 0.5f; ty = s_vpH - t.headY; }
                dx = tx - cx; dy = ty - cy;
            }
            // Graus por pixel: deriva do FOV VERTICAL REAL da camera, nunca do
            // raio do circulo (auditoria 23/09: fFovAngle cancela e vira 0.125
            // fixo = overshoot 2.3x). dAng = atan2(dx_px * tan(fovV/2) / (H/2)).
            // fovV: FOVController.CurrentFOV se resolver, senao fCamFov do menu.
            float fovV = Config::fCamFov;
            if (!(fovV > 20.0f && fovV < 120.0f)) fovV = 60.0f;
            {
                static float s_fovCache = -1.0f;
                static long long s_fovT = 0;
                long long now = PiNow();
                if (s_fovCache < 0.0f || now - s_fovT > 5000000LL) {
                    s_fovT = now;
                    // Tenta ler CurrentFOV real 1x/5s (barato, fora do tiro).
                    // Falhou = mantem cache/menu. Nunca trava por isso.
                    float got = AimReadFov();
                    if (got > 20.0f && got < 120.0f) s_fovCache = got;
                    else if (s_fovCache < 0.0f) s_fovCache = fovV;
                }
                fovV = s_fovCache;
            }
            float halfH = s_vpH * 0.5f;
            if (!(halfH > 100.0f)) halfH = 540.0f;
            float tanHalf = tanf(fovV * 0.5f * 0.01745329252f);
            float dYaw = atan2f(dx * tanHalf / halfH, 1.0f) * 57.29577951f;
            float dPitch = -atan2f(dy * tanHalf / halfH, 1.0f) * 57.29577951f;
            // Smoothing em espaco angular (padrao mercado): divide o delta.
            // fSmoothing 1 = snap; 6-8 = legit; >20 = lento.
            float sm = Config::fSmoothing;
            if (!(sm >= 1.0f && sm <= 30.0f)) sm = 8.0f;
            // Snap de perto: <0.15 grau = vai direto (evita jitter parado).
            float angDist = sqrtf(dYaw * dYaw + dPitch * dPitch);
            if (angDist < 0.15f) { dYaw = 0; dPitch = 0; }
            else { dYaw /= sm; dPitch /= sm; }
            // Le pitch atual, soma, clamp ±80 (igual ao jogo), escreve.
            // Yaw: Rotate(up * dYaw) no transform do PlayerCamera.
            __try {
                MonoObject* exc = nullptr;
                // get_Angle (1 invoke, barato; fora do orcamento do ESP).
                float curPitch = 0;
                if (mGetAngle) {
                    MonoObject* ret = pInvoke(mGetAngle, pcam, nullptr, &exc);
                    if (!exc && ret) memcpy(&curPitch, pUnbox(ret), 4);
                } else if (oAngle >= 0) {
                    curPitch = ReadF(pcam, oAngle, 0.0f);
                }
                float want = curPitch + dPitch;
                if (want < -80.0f) want = -80.0f;
                if (want > 80.0f) want = 80.0f;
                if (mSetAngle) {
                    void* args[1] = { &want };
                    MonoObject* exc2 = nullptr;
                    pInvoke(mSetAngle, pcam, args, &exc2);
                } else if (oAngle >= 0) {
                    WriteF(pcam, oAngle, want);
                }
                // Yaw via Transform.Rotate(euler 0,dYaw,0) — 1 arg, igual ao
                // jogo (auditoria 23/09: Rotate/2 resolve overload (Vector3,
                // Space) e ignorava o angulo = deriva fixa p/ direita).
                // Custo: 2 invokes. So quando |dYaw| > 0.05 grau.
                if (dYaw > 0.05f || dYaw < -0.05f) {
                    if (mGetTrans && cTrans) {
                        MonoObject* exc3 = nullptr;
                        MonoObject* tr = pInvoke(mGetTrans, pcam, nullptr, &exc3);
                        if (!exc3 && tr) {
                            // Rotate(Vector3 euler): 1 arg, resolve 1x.
                            static MonoMethod* s_mRot = nullptr;
                            static bool s_rotInit = false;
                            if (!s_rotInit) {
                                s_rotInit = true;
                                s_mRot = pMethodFrom(cTrans, "Rotate", 1);
                            }
                            if (s_mRot) {
                                float eul[3] = { 0.0f, dYaw, 0.0f };
                                void* rargs[1] = { &eul };
                                MonoObject* exc4 = nullptr;
                                pInvoke(s_mRot, tr, rargs, &exc4);
                                if (exc4) {
                                    static bool s_rotWarn = false;
                                    if (!s_rotWarn) {
                                        s_rotWarn = true;
                                        Log::Warn("[AIMBOT] Rotate/1 falhou (exc).");
                                    }
                                }
                            } else {
                                static bool s_rotMiss = false;
                                if (!s_rotMiss) {
                                    s_rotMiss = true;
                                    Log::Warn("[AIMBOT] Rotate/1 nao resolvido.");
                                }
                            }
                        }
                    }
                }
                static int s_aimLogged = 0;
                if (s_aimLogged < 2) {
                    s_aimLogged++;
                    Log::Infof("[AIMBOT] alvo=%d dYaw=%.2f dPitch=%.2f sm=%.0f.",
                        best, (double)dYaw, (double)dPitch, (double)sm);
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {}
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // Le o FOV vertical real (FOVController.CurrentFOV). Chamado pelo aimbot
    // 1x/5s; fora disso nunca invoca. Falhou = -1 (usa menu).
    // Corpo ANTES do ApplyAim (forward acima); sem duplicata abaixo.
    static float AimReadFov(void) {
        __try {
            if (!cPCam || !s_dom) return -1.0f;
            // PlayerCamera.fovController -> FOVController.CurrentFOV.
            // Resolve 1x: classe + 2 campos (static, cacheado).
            static MonoClass* s_cFov = nullptr;
            static MonoClassField* s_fCtl = nullptr;
            static MonoClassField* s_fCur = nullptr;
            static bool s_init = false;
            if (!s_init) {
                s_init = true;
                s_cFov = pClassFrom(s_img, "", "FOVController");
                if (s_cFov) {
                    s_fCtl = pFieldFrom(cPCam, "fovController");
                    s_fCur = pFieldFrom(s_cFov, "CurrentFOV");
                }
            }
            if (!s_fCtl || !s_fCur) return -1.0f;
            // Precisa do PlayerCamera do local — ainda sem chain barato aqui.
            // Ciclo aimbot-2 resolve via local (PlayerMain.cam + offsets).
            // Por enquanto: -1 honesto (menu fCamFov manda, sem chute).
            return -1.0f;
        } __except (EXCEPTION_EXECUTE_HANDLER) { return -1.0f; }
    }

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





















