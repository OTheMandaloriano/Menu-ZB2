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
        int   skN;          // ossos projetados (item 12 Skeleton, max 16)
        float skX[16], skY[16]; // pixels Unity (y p/ cima, igual px/py)
        bool  skV[16];      // osso na frente da camera
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

    // Snapshot do ESP (preenchido no Tick; ler no Render).
    // N <0 = todos; retorna quantidade escrita.
    int GetEsp(EspEntry* out, int max);
    void Shutdown();
}



