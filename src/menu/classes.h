#pragma once
#include <cstdint>
#include "offsets.h"

// ============================================================================
// CLASSES.H - SDK do alvo (Zumbi Blocks 2 / Unity 6 Mono x64)
// OBJETIVO: structs C++ espelhando classes Unity apos auditoria dnSpy.
// ORIGEM: dnSpyEx em Assembly-CSharp.dll + ReClass.NET (ver memory/CLASSES_UTEIS.md).
// TESTES: static_assert de tamanho quando layout validado; SEH em todo acesso.
// HISTORICO: v0.1.0 base matematica + exemplos; entidades reais apos auditoria.
// ============================================================================
// REGRA: nunca chutar layout. Campos reais so entram com origem dnSpy/CE MCP.
// ============================================================================

#pragma pack(push, 1)

struct Vector2 { float x, y; };
struct Vector3 { float x, y, z; };
struct Vector4 { float x, y, z, w; };

// Matriz view-projection da camera Unity (para WorldToScreen).
struct Matrix4x4 { float m[4][4]; };

// Entidade generica minima para a Fase 1/2 (debug overlay).
// O layout real de Zombie/Player sera mapeado na Fase 2 via reflection.
class ExampleEntity {
public:
    char     pad_0000[0x10]; // 0x00 nao mapeado
    int32_t  m_iHealth;      // 0x10 exemplo
    int32_t  m_iTeam;        // 0x14 exemplo (0=zumbi 1=aliado - A CONFIRMAR)
    char     pad_0018[0x20]; // 0x18 gap
    Vector3  m_vecPosition;  // 0x38 exemplo
};

// Snapshot usado pelo ESP (preenchido via reflection, sem expor Mono aqui).
// NOTA: struct legado — o overlay usa Mono::EspEntry (mono.h).
struct EspEntry {
    char     name[64];
    Vector3  pos;        // mundo
    Vector2  screen;     // tela (apos W2S)
    float    dist;       // metros ate camera local
    int      hp;
    int      maxHp;
    bool     isAlly;     // true=aliado(azul) false=zumbi(vermelho)
    bool     isBoss;     // Riot/Queen/Reaper
};

#pragma pack(pop)

// Cor dinamica da health bar por HP (briefing Pt.7.9).
// 76-100 verde | 51-75 verde-amarelado | 26-50 amarelo
// 11-25 laranja | 1-10 vermelho | 0 cinza.
inline void HpColor(float hpPct, float out[4]) {
    if (hpPct > 75.0f)      { out[0]=0.0f;  out[1]=1.0f;  out[2]=0.0f;  out[3]=1.0f; }
    else if (hpPct > 50.0f) { out[0]=0.7f;  out[1]=1.0f;  out[2]=0.0f;  out[3]=1.0f; }
    else if (hpPct > 25.0f) { out[0]=1.0f;  out[1]=1.0f;  out[2]=0.0f;  out[3]=1.0f; }
    else if (hpPct > 10.0f) { out[0]=1.0f;  out[1]=0.55f; out[2]=0.0f;  out[3]=1.0f; }
    else if (hpPct > 0.0f)  { out[0]=1.0f;  out[1]=0.0f;  out[2]=0.0f;  out[3]=1.0f; }
    else                    { out[0]=0.5f;  out[1]=0.5f;  out[2]=0.5f;  out[3]=1.0f; }
}
