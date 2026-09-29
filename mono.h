#pragma once

// ============================================================================
// MONO.H - Reflection Unity Mono via binding dinamico (ZB2 Menu)
// OBJETIVO: ler classes/campos do Assembly-CSharp sem linkar contra o Mono.
// ORIGEM: offsets validados na auditoria runtime #1 (memory/OFFSETS.md).
// TESTES: overlay com HP local/zumbis; ESP Box 2D via WorldToScreen.
// HISTORICO: v0.5.0 bind + daytime. v0.6.0 entidades. v0.7.0 W2S + Box 2D.
// ============================================================================

namespace Mono {
    struct Vec3 { float x, y, z; };

    // Juntas do rig (auditorias [BONE]+[JOINT] 14/09, armature len=19):
    // hl=pelvis, sp1-3=coluna, a1=ombro->cotovelo(0.28m), a2=antebraco(0.21m).
    // SK_PHYS = 18 ossos fisicos (idx 0-17). HL2 = maos estimadas via rotacao
    // do antebraco (a2) — disponiveis em 2D e 3D (fix bugs 1-4).
    enum SkJoint {
        SK_HEAD = 0, SK_NECK, SK_SP3, SK_SP2, SK_SP1,
        SK_HL, SK_L1L, SK_L2L, SK_FL,
        SK_L1R, SK_L2R, SK_FR,
        SK_SL, SK_A1L, SK_A2L, SK_SR, SK_A1R, SK_A2R,
        SK_PHYS = 18,
        SK_HL2L = 18, SK_HL2R = 19, // maos estimadas (runtime, nao-bones)
        SK_COUNT = 20
    };

    // Entrada de ESP (Fase 3): coordenadas JA em pixels ImGui (y para baixo).
    struct EspEntry {
        float headX, headY;  // cabeca (tela)
        float footX, footY;  // pes (tela)
        float hp, maxHp;
        float dist; // metros ate a camera
        float px[8], py[8]; // cantos da AABB projetados (pixels Unity, y p/ cima)
        bool  pv[8];        // canto na frente da camera
        bool  has3d;        // AABB valida (Box 3D real; senao fallback 2D)
        void* ent;          // ponteiro da entidade (log diagnostico P1)
        float ex, ey, ez;   // extents da AABB (log diagnostico P1)
        int   skN;          // juntas validas (item 12 Skeleton real)
        float skX[20], skY[20]; // pixels Unity (y p/ cima, igual px/py)
        bool  skV[20];      // junta na frente da camera
        char  name[64];
        bool  onScreen;
        bool  isAlly;        // item 15 (sempre false no item 7)
        bool  isBoss;        // ZombieIdentity.type+20 (Riot/Queen/Reaper). Sem
                             // type validado via CE MCP = false (nunca chutar).
    };

    struct CatalogEntry {int id;char name[96];};
    struct IconPixels {int id=-1;bool valid=false;unsigned char rgba[48*48*4]={};};
    int GetCatalog(CatalogEntry* output,int capacity);
    void RequestIcon(int id);
    bool GetIcon(IconPixels& output);
    struct WorldMarker {
        int kind;
        float x, y, distance; // normalized screen coordinates, y down
        char name[96];
        float left,top,right,bottom,health;
        float maxHealth;
        float corners[24],bones[51];
    };
    static_assert(sizeof(WorldMarker)==436, "Managed marker ABI");
    int GetWorldEsp(WorldMarker* output, int capacity);
    struct DistantMarker { float x,y,distance; int type,state; };
    static_assert(sizeof(DistantMarker)==20,"Managed distant marker ABI");
    int GetDistantEsp(DistantMarker* output,int capacity);

    struct State {
        bool featureKeysAllowed = false;
        char modifierStatus[192] = {};
        char visualStatus[128] = {};
        bool  ready = false;
        float dayTime = 0.0f;
        float dayLenMin = 0.0f;
        float localHp = 0.0f;
        float localStam = 0.0f;
        float allyHp = 0.0f;       // HP do primeiro aliado vivo; 0 se nenhum
        int   allies = 0;          // outros jogadores válidos em PlayersController
        char  roomHost[64] = {};   // playerName do host do lobby, não Steam nick
        char  roomId[32] = {};     // MultiplayerController.GetLobbyCode()
        int   players = 0;
        int   zombies = 0;
        float zHp0 = 0.0f;
        float espMs = 0.0f;    // custo do BuildEsp (diagnostico)
        int   espShown = 0;
        char extrasStatus[192]={};
        int pyreCount=0;
        char pyreStatus[192]={};
        int   coopMode = 0;        // 0=lobby/unknown 1=single 2=client 3=host
        int   ammoWrites = 0;   // escritas no pente (0 = trava nao roda)
        bool  ammoOk = true;    // false = cadeia incompleta (AMMO:OFF)
        bool  slotsOk = true;   // false = campos nao resolveram
        bool  slotsOn = false;  // true = ApplySlots rodou (SLOTS:ON)
        bool  inMap = false;   // true = dentro do mapa (localHp>0 E players>=1)
        bool  loadoutOn = false; // true = LoadoutSelector.UnlockAll rodou
        int   resolvedClasses = 0;
        int   resolvedFields = 0;
        int   resolvedMethods = 0;
    };

    bool Init();
    void Tick();
    void SetUiState(int flags);
    const State& Get();
    void SetViewport(float w, float h); // publish viewport + Config snapshot from Present

    // Snapshot do ESP (preenchido no Tick; ler no Render).
    // N <0 = todos; retorna quantidade escrita.
    int GetEsp(EspEntry* out, int max);
    void Shutdown();
}



