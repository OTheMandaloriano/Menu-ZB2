#pragma once

// Ordem estrita de inclusao: Windows/D3D11 -> kiero -> imgui -> projeto.
// OBJETIVO: ponto unico de include para evitar divergencia de ordem entre TUs.
// ORIGEM: adaptado de kiero-dx9-base (D3D9) para D3D11 (Zumbi Blocks 2, Unity 6 Mono x64).
// TESTES: compilar Release|x64 com 0 erros; validar Present hook in-game.
// HISTORICO: v0.1.0 troca d3d9/d3dx9 por d3d11 + imgui_impl_dx11.

#include <Windows.h>
#include <d3d11.h>

#include "kiero/kiero.h"
#include "kiero/minhook/include/MinHook.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

#include "config.h"
#include "log.h"
#include "offsets.h"
#include "classes.h"
#include "gui.h"
#include "mono.h"

// Present: slot 8 da swapchain D3D11. ResizeBuffers: slot 13.
typedef long(__stdcall* Present_t)(IDXGISwapChain*, UINT, UINT);
typedef long(__stdcall* ResizeBuffers_t)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
typedef LRESULT(CALLBACK* WNDPROC_t)(HWND, UINT, WPARAM, LPARAM);

