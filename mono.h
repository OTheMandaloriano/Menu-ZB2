#pragma once

// ============================================================================
// MONO.H - Reflection Unity Mono via binding dinamico (ZB2 Menu)
// OBJETIVO: ler classes/campos do Assembly-CSharp sem linkar contra o Mono
//   (GetModuleHandle + GetProcAddress em mono-2.0-bdwgc.dll).
// ORIGEM: offsets validados na auditoria runtime #1 (memory/OFFSETS.md).
// TESTES: log mostra Resolve OK + curTime do DaytimeController variando.
// HISTORICO: v0.5.0 cria (Fase 2 item 5). Leitura de entidades (List<>) no item 6.
// ============================================================================

namespace Mono {
    struct State {
        bool  ready = false;   // bind + imagem + classes resolvidas
        float dayTime = 0.0f;  // DaytimeController.curTime (horas)
        float dayLenMin = 0.0f;// dayDurationInMinutes
        int   resolvedClasses = 0;
        int   resolvedFields = 0;
        int   resolvedMethods = 0;
    };

    bool Init();          // bind + attach + resolve (idempotente, tenta de novo se falhar)
    void Tick();          // leitura throttled; chamar 1x por Present
    const State& Get();
    void Shutdown();
}
