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
        bool  losVis;       // item 14: proporcao exposta >= 40% (multi-bone)
        int   losHits;      // item 14: pontos expostos de 5 (log [LOS])
        float losDepth[5];  // item 14/Passo5: profundidade NDC do osso (depth buffer)
        int   skN;          // juntas validas (item 12 Skeleton real)
        float skX[20], skY[20]; // pixels Unity (y p/ cima, igual px/py)
        bool  skV[20];      // junta na frente da camera
        char  name[64];
        bool  onScreen;
        bool  isAlly;        // item 15 (sempre false no item 7)
    };

    struct State {
        bool  ready = false;
        float dayTime = 0.0f;
        float dayLenMin = 0.0f;
        float localHp = 0.0f;
        float localStam = 0.0f;
        float allyHp = 0.0f;
        int   players = 0;
        int   zombies = 0;
        float zHp0 = 0.0f;
        float espMs = 0.0f;    // custo do BuildEsp (diagnostico)
        int   espShown = 0;
        int   resolvedClasses = 0;
        int   resolvedFields = 0;
        int   resolvedMethods = 0;
    };

    bool Init();
    void Tick();
    const State& Get();
    void SetViewport(float w, float h); // tela atual (p/ W2S proprio)
    // Implementado em main.cpp (captura D3D11). Declarado aqui p/ a worker.
    // Usa :: (global) porque a definicao esta em namespace global, nao em Mono.
    namespace DepthVisShim { bool Sample(float u, float v, float& outNdc); }

    // Snapshot do ESP (preenchido no Tick; ler no Render).
    // N <0 = todos; retorna quantidade escrita.
    int GetEsp(EspEntry* out, int max);
    void Shutdown();
}



