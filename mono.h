#pragma once

// ============================================================================
// MONO.H - Reflection Unity Mono via binding dinamico (ZB2 Menu)
// OBJETIVO: ler classes/campos do Assembly-CSharp sem linkar contra o Mono.
// ORIGEM: offsets validados na auditoria runtime #1 (memory/OFFSETS.md).
// TESTES: log resolve OK + overlay com HP local e contagem de zumbis.
// HISTORICO: v0.5.0 bind + daytime. v0.6.0 entidades (List<> + HasLocalControl).
// ============================================================================

namespace Mono {
    struct State {
        bool  ready = false;
        float dayTime = 0.0f;
        float dayLenMin = 0.0f;
        float localHp = 0.0f;    // PlayerMain.healthFast do player local
        float localStam = 0.0f;  // staminaFast
        float allyHp = 0.0f;     // primeiro aliado (ForeignPlayer)
        int   players = 0;
        int   zombies = 0;       // vivos (hp>0)
        float zHp0 = 0.0f;       // HP do primeiro zumbi vivo
        int   resolvedClasses = 0;
        int   resolvedFields = 0;
        int   resolvedMethods = 0;
    };

    bool Init();
    void Tick();
    const State& Get();
    void Shutdown();
}
