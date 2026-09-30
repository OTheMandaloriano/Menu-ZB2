#pragma once
#include <cstdint>

// ============================================================================
// OFFSETS.H - Registro centralizado de offsets (ZB2 / Unity 6 Mono x64 D3D11)
// OBJETIVO: nenhum offset espalhado em .cpp; tudo aqui com origem documentada.
// ORIGEM: a preencher via dnSpyEx (Assembly-CSharp.dll) + CE MCP runtime.
//   Formato: nome, offset, tipo, comando MCP, data.
// TESTES: validar leitura/escrita in-game antes de marcar [OK] em memory/OFFSETS.md.
// HISTORICO: v0.1.0 esqueleto; sem offsets chutados (briefing Pt.3.5/Pt.9).
// ============================================================================
// COMO VALIDAR (exemplo CE MCP):
//   scan_all(value, type) -> get_scan_results -> next_scan -> read_memory
//   Toda descoberta vai para memory/OFFSETS.md com o comando usado.
// ============================================================================

namespace Offsets {
    // Guneis de VTable D3D11 (fixos por spec, nao sao do jogo):
    constexpr int VTBL_PRESENT       = 8;
    constexpr int VTBL_RESIZEBUFFERS = 13;

    // ---- Jogo (preencher apos auditoria; NAO adivinhar) ----
    // Exemplo de formato quando validado:
    // constexpr uintptr_t ADDR_GAMEMANAGER = 0x0; // [PENDENTE dnSpy+CE MCP]
    // constexpr uint32_t OFF_HEALTH = 0x0;        // [PENDENTE]

    // Placeholders zerados de proposito: codigo que usar deve checar != 0.
    constexpr uintptr_t ADDR_LOCALPLAYER = 0x0; // [PENDENTE]
    constexpr uintptr_t ADDR_ENTITYLIST  = 0x0; // [PENDENTE]
    constexpr uintptr_t ADDR_CAMERA      = 0x0; // [PENDENTE]

    constexpr uint32_t OFF_HEALTH = 0x0; // [PENDENTE]
    constexpr uint32_t OFF_MAXHP  = 0x0; // [PENDENTE]
    constexpr uint32_t OFF_POS    = 0x0; // [PENDENTE]
    constexpr uint32_t OFF_NAME   = 0x0; // [PENDENTE]
    constexpr uint32_t OFF_TEAM   = 0x0; // [PENDENTE]
}
