#include "includes.h"

// ============================================================================
// MAIN.CPP - Ponto de entrada + hook D3D11 Present/ResizeBuffers (ZB2 Menu)
// OBJETIVO: overlay ImGui D3D11 no Zumbi Blocks 2 (Unity 6 Mono x64 D3D11).
// ORIGEM: kiero-dx9-base/main.cpp (EndScene slot 42) -> Present slot 8 D3D11.
// TESTES: injetar -> menu INSERT abre; Alt+Tab/resize nao crasha; unload limpo.
// HISTORICO: v0.1.0 troca DX9 por DX11; adiciona cursor fix + render target.
//   fix v0.2.1: corrige corrida bind-Present antes da janela (DisplaySize 0,0):
//   descobre g_hWindow ANTES do kiero::init + guarda !g_hWindow no hkPresent.
//   Diagnostico do operador: menu invisivel + cursor visivel.
// ============================================================================
// FASE 1 ITEM 1: Hook D3D11 + ImGui funcional (menu abre e fecha).
// ============================================================================

#ifdef _WIN64
#define GWL_WNDPROC_INDEX GWLP_WNDPROC
#else
#define GWL_WNDPROC_INDEX GWL_WNDPROC
#endif

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static Present_t       oPresent = nullptr;
static ResizeBuffers_t oResizeBuffers = nullptr;
static WNDPROC         oWndProc = nullptr;
static HWND            g_hWindow = nullptr;
static bool            g_bInit = false;

static ID3D11Device*           g_pDevice = nullptr;
static ID3D11DeviceContext*    g_pContext = nullptr;
static ID3D11RenderTargetView* g_pRTV = nullptr;

static void CreateRenderTarget(IDXGISwapChain* pSwapChain) {
    ID3D11Texture2D* pBack = nullptr;
    if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBack)) && pBack) {
        g_pDevice->CreateRenderTargetView(pBack, nullptr, &g_pRTV);
        pBack->Release();
    }
}

static void CleanupRenderTarget() {
    if (g_pRTV) { g_pRTV->Release(); g_pRTV = nullptr; }
}

// ResizeBuffers: recria RTV (Alt+Tab / resize). Sem isso, tela preta/crash.
static long __stdcall hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount,
    UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
    CleanupRenderTarget();
    ImGui_ImplDX11_InvalidateDeviceObjects();
    long hr = oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
    if (SUCCEEDED(hr)) {
        CreateRenderTarget(pSwapChain);
        ImGui_ImplDX11_CreateDeviceObjects();
    }
    return hr;
}

static long __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    if (!g_bInit) {
        // Guarda: sem janela nao ha como inicializar o backend Win32 do ImGui
        // (HWND nulo => DisplaySize 0,0 => menu invisivel). Pula o frame.
        if (!g_hWindow) {
            return oPresent(pSwapChain, SyncInterval, Flags);
        }
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pDevice)) && g_pDevice) {
            g_pDevice->GetImmediateContext(&g_pContext);
            CreateRenderTarget(pSwapChain);
            GUI::Initialize(g_hWindow, g_pDevice, g_pContext);
            g_bInit = true;
            Log::Info("GUI inicializada (ImGui D3D11/Win32).");
        } else {
            return oPresent(pSwapChain, SyncInterval, Flags);
        }
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    GUI::Render();         // janela do menu (4 abas)
    GUI::RenderOverlay();  // watermark/debug/FOV fora da janela

    ImGui::EndFrame();
    ImGui::Render();

    if (g_pContext && g_pRTV) {
        g_pContext->OMSetRenderTargets(1, &g_pRTV, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
    return oPresent(pSwapChain, SyncInterval, Flags);
}

// WndProc: INSERT/DELETE alterna; cursor fix devolve controle ao jogo fechado.
static LRESULT CALLBACK hkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_KEYDOWN && (wParam == VK_INSERT || wParam == VK_DELETE)) {
        Config::bMenuOpen = !Config::bMenuOpen;
        // Cursor fix: menu fechado -> jogo volta a capturar o mouse.
        ImGuiIO& io = ImGui::GetIO();
        io.MouseDrawCursor = Config::bMenuOpen;
        if (!Config::bMenuOpen) {
            // Solta qualquer captura para o jogo retomar a camera.
            ClipCursor(nullptr);
            while (ShowCursor(TRUE) < 0) {}
        } else {
            while (ShowCursor(FALSE) >= 0) {}
        }
        return TRUE;
    }
    if (Config::bMenuOpen && g_bInit) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
            return TRUE;
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse && (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST))
            return TRUE;
        if (io.WantCaptureKeyboard && (uMsg == WM_KEYDOWN || uMsg == WM_KEYUP || uMsg == WM_CHAR))
            return TRUE;
    }
    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

static BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam) {
    DWORD pid = 0;
    GetWindowThreadProcessId(handle, &pid);
    if (GetCurrentProcessId() != pid) return TRUE;
    if (!IsWindowVisible(handle)) return TRUE;
    g_hWindow = handle;
    return FALSE;
}

static HWND GetProcessWindow() {
    g_hWindow = nullptr;
    EnumWindows(EnumWindowsCallback, 0);
    return g_hWindow;
}

static DWORD WINAPI MainThread(LPVOID lpReserved) {
    (void)lpReserved;
    Log::Info("==================================================");
    Log::Infof("Log ativo em: %s", Log::GetPath());
    Log::Info("MainThread iniciada.");

    // ETAPA 1: descobrir a janela ANTES de qualquer hook do Present.
    // Motivo (fix v0.2.1): o bind do Present ativa o hkPresent a cada frame;
    // se a janela ainda e nullptr, o ImGui inicializa com HWND nulo e o
    // DisplaySize fica (0,0) -> menu invisivel (so o cursor aparece).
    do {
        g_hWindow = GetProcessWindow();
        if (!g_hWindow) Sleep(50);
    } while (g_hWindow == nullptr);
    Log::Infof("Janela encontrada: 0x%p.", g_hWindow);

    // ETAPA 2: com a janela resolvida, instala os hooks.
    bool bAttached = false;
    do {
        if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success) {
            Log::Info("kiero::init OK (D3D11).");
            if (kiero::bind(8, (void**)&oPresent, (void*)hkPresent) == kiero::Status::Success)
                Log::Info("Bind OK: Present (slot 8).");
            else
                Log::Error("Bind FALHOU: Present (slot 8).");
            if (kiero::bind(13, (void**)&oResizeBuffers, (void*)hkResizeBuffers) == kiero::Status::Success)
                Log::Info("Bind OK: ResizeBuffers (slot 13).");
            else
                Log::Error("Bind FALHOU: ResizeBuffers (slot 13).");

            oWndProc = (WNDPROC)SetWindowLongPtr(g_hWindow, GWL_WNDPROC_INDEX, (LONG_PTR)hkWndProc);
            Log::Info("WndProc hookado, menu operacional (INSERT/DELETE).");
            bAttached = true;
        } else {
            Log::Warn("kiero::init falhou, nova tentativa em 100ms...");
            Sleep(100);
        }
    } while (!bAttached);

    Log::Info("MainThread concluida.");
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    (void)lpReserved;
    if (dwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        Log::SetModule(hModule);
        CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
    } else if (dwReason == DLL_PROCESS_DETACH) {
        Log::Info("DLL_PROCESS_DETACH, liberando hooks...");
        if (oWndProc && g_hWindow)
            SetWindowLongPtr(g_hWindow, GWL_WNDPROC_INDEX, (LONG_PTR)oWndProc);
        CleanupRenderTarget();
        if (g_pContext) { g_pContext->Release(); g_pContext = nullptr; }
        if (g_pDevice) { g_pDevice->Release(); g_pDevice = nullptr; }
        GUI::Shutdown();
        kiero::shutdown();
        Log::Info("Hooks liberados.");
        Log::Shutdown();
    }
    return TRUE;
}
