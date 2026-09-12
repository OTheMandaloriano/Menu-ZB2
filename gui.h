#pragma once
#include <d3d11.h>
#include "imgui/imgui.h"

// ============================================================================
// GUI.H - Camada de renderizacao Dear ImGui D3D11 (ZB2 Menu)
// OBJETIVO: menu 4 abas (PLAYER-VISUAL-MISC-SETTINGS) + overlay + preview.
// ORIGEM: kiero-dx9-base/gui.h adaptado D3D9->D3D11; layout do briefing Pt.4/Pt.7.
// TESTES: F1 abre/fecha sem crash; troca de aba sem flicker; preview arrastavel.
// HISTORICO: v0.1.0 troca LPDIRECT3DDEVICE9 por D3D11 device/context.
//   fix: inclui imgui.h para ImVec2 (build Release|x64).
// ============================================================================

namespace GUI {
    void Initialize(HWND hWindow, ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    void Render();
    void RenderOverlay();   // ESP/watermark/debug (fora da janela do menu)
    void Shutdown();

    // Preview interativo do ESP (VISUAL): textos arrastaveis, barra auto-orientada.
    void DrawEspPreview(ImVec2 origin, ImVec2 size);

    extern bool g_bHasFocusFix; // cursor fix: re-mostrar cursor do jogo ao fechar menu
}
