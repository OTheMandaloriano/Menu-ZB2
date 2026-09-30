#include "ui_capture.h"
#include "ui_input_queue.h"
#include <atomic>
#include "includes.h"
#include "runtime_gate.h"
#include "silhouette.h"

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
static UiInput::Queue g_uiInput;
static std::atomic<bool> g_uiOpen{false};
static std::atomic<bool> g_wantMouse{false},g_wantKeyboard{false};
static std::atomic<int> g_uiKey{VK_INSERT};
static void DrainUiInput();

static ID3D11Device*           g_pDevice = nullptr;
static ID3D11DeviceContext*    g_pContext = nullptr;
static ID3D11RenderTargetView* g_pRTV = nullptr;
static LRESULT CALLBACK hkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam); // forward
static HWND GetProcessWindow(); // forward (fallback se GetDesc falhar)

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
    RuntimeGate::TryScope guard;
    if (!guard || !g_bInit) { guard.Release(); return oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags); }
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
    RuntimeGate::TryScope guard;
    auto forward = [&]() { guard.Release(); return oPresent(pSwapChain, SyncInterval, Flags); };
    if (!guard) return forward();
    if (Flags & DXGI_PRESENT_TEST) return forward(); // teste oculto: nao desenha
    // FIX crash-no-loading 21/09: durante o loading o Unity apresenta com
    // swapchain incompleta (backbuffer em transicao). Qualquer toque em D3D
    // aqui (GetDevice/CreateRenderTarget) = AV dentro do Present (stack:
    // 2 frames kiero + UnityMain, sem 1 frame de jogo). Gate: so inicializa
    // quando a janela tem area real E o device responde. Antes disso, so
    // repassa o Present (jogo carrega sozinho, sem hook).
    if (!g_bInit) {
        // Padrao mercado (ImGuiRedux/rdbo/kiero-imgui): HWND vem da swapchain
        // (Desc.OutputWindow), nao de EnumWindows. Funciona em loading, menu,
        // partida — independe da janela do cliente. Se a desc falhar, tenta a
        // janela real 1x (fallback barato, sem loop, sem abortar a thread).
        DXGI_SWAP_CHAIN_DESC desc = { 0 };
        HWND hwnd = nullptr;
        if (SUCCEEDED(pSwapChain->GetDesc(&desc)) && desc.OutputWindow)
            hwnd = desc.OutputWindow;
        if (!hwnd) hwnd = GetProcessWindow();
        if (!hwnd) return forward();
        // Janela sem area = loading (splash Unity 6000.3.21f1): nao toca.
        {
            RECT cr = { 0 };
            if (!GetClientRect(hwnd, &cr) || (cr.right - cr.left) < 200 || (cr.bottom - cr.top) < 200)
                return forward();
        }
        // Troca de janela (loading -> partida recria swapchain): re-hook WndProc.
        if (hwnd != g_hWindow) {
            if (oWndProc && g_hWindow)
                SetWindowLongPtr(g_hWindow, GWL_WNDPROC_INDEX, (LONG_PTR)oWndProc);
            oWndProc = nullptr;
            g_hWindow = hwnd;
        }
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pDevice)) && g_pDevice) {
            g_pDevice->GetImmediateContext(&g_pContext);
            CreateRenderTarget(pSwapChain);
            GUI::Initialize(g_hWindow, g_pDevice, g_pContext);
            g_bInit = true;
            Log::Infof("GUI inicializada (HWND=0x%p, swapchain).", g_hWindow);
            oWndProc = (WNDPROC)SetWindowLongPtr(g_hWindow, GWL_WNDPROC_INDEX, (LONG_PTR)hkWndProc);
            Log::Info("WndProc hookado, menu operacional (INSERT/DELETE).");
        } else {
            return forward();
        }
    }

    DrainUiInput();
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();



    GUI::ProcessFeatureHotkeys();
    GUI::Render();
    const auto& input=ImGui::GetIO();
    g_uiOpen.store(Config::bMenuOpen);g_uiKey.store(Config::iMenuKey);
    g_wantMouse.store(input.WantCaptureMouse);g_wantKeyboard.store(input.WantCaptureKeyboard);
    Mono::SetUiState(Config::bMenuOpen?(1|(input.WantCaptureMouse?2:0)|(input.WantCaptureKeyboard?4:0)):0);         // janela do menu (4 abas)
    Mono::SetViewport(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);
    Mono::Tick(); // bootstrap only; never invokes Unity
    Silhouette::Draw(g_pDevice,g_pContext,Config::bSilhouette,Config::colSilhouetteVis,Config::colSilhouetteInv,Config::fSilhouetteWidth);
    GUI::RenderOverlay();  // watermark/debug/FOV fora da janela

    ImGui::EndFrame();
    ImGui::Render();

    if (g_pContext && g_pRTV) {
        ID3D11RenderTargetView* previous[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
        ID3D11DepthStencilView* depth = nullptr;
        g_pContext->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, previous, &depth);
        g_pContext->OMSetRenderTargets(1, &g_pRTV, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pContext->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, previous, depth);
        for (auto* target : previous) if (target) target->Release();
        if (depth) depth->Release();
    }
    return forward();
}

// ImGui owns queued UI input; Unity owns cursor lock/visibility on its Update thread.
static void DrainUiInput() {
    UiInput::Event event;
    while(g_uiInput.Pop(event)) {
        if(event.message==WM_KEYDOWN && (event.wparam==static_cast<WPARAM>(Config::iMenuKey)||(event.wparam==VK_DELETE && !ImGui::GetIO().WantTextInput))) {
            if(!(event.lparam&(1LL<<30))) {
                Config::bMenuOpen=!Config::bMenuOpen;
                if(Config::bMenuOpen)ClipCursor(nullptr);
                else {ReleaseCapture();ImGui::GetIO().ClearInputKeys();}
            }
        } else if(Config::bMenuOpen) ImGui_ImplWin32_WndProcHandler(event.window,event.message,event.wparam,event.lparam);
    }
}
static LRESULT CALLBACK hkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    const bool open=g_uiOpen.load();
    const bool keyboard=uMsg>=WM_KEYFIRST && uMsg<=WM_KEYLAST;
    const bool mouse=uMsg>=WM_MOUSEFIRST && uMsg<=WM_MOUSELAST;
    const bool toggle=uMsg==WM_KEYDOWN && (wParam==static_cast<WPARAM>(g_uiKey.load())||wParam==VK_DELETE);
    if(toggle || (open && (keyboard || mouse || uMsg==WM_SETFOCUS || uMsg==WM_KILLFOCUS))) {
        g_uiInput.Push({hWnd,uMsg,wParam,lParam});
        if(UiCapture::Consume(open,toggle,keyboard,mouse,g_wantKeyboard.load(),g_wantMouse.load()))return TRUE;
    }
    return CallWindowProc(oWndProc,hWnd,uMsg,wParam,lParam);
}

static BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam) {
    DWORD pid = 0;
    GetWindowThreadProcessId(handle, &pid);
    if (GetCurrentProcessId() != pid) return TRUE;
    // Janela do jogo = top-level visivel com area de cliente real.
    // Sem o filtro de area, pega tooltip/overlay invisivel (bug 20/09:
    // MainThread abortava com jogo aberto — janela fantasma estavel).
    if (!IsWindowVisible(handle)) return TRUE;
    RECT r = { 0 };
    if (!GetClientRect(handle, &r)) return TRUE;
    if (r.right - r.left < 200 || r.bottom - r.top < 200) return TRUE;
    // Sem dono e sem estilo TOOLWINDOW (tooltip/sombra).
    LONG ex = GetWindowLong(handle, GWL_EXSTYLE);
    if (ex & WS_EX_TOOLWINDOW) return TRUE;
    if (GetWindow(handle, GW_OWNER) != nullptr) return TRUE;
    g_hWindow = handle;
    return FALSE;
}

static HWND GetProcessWindow() {
    g_hWindow = nullptr;
    EnumWindows(EnumWindowsCallback, 0);
    return g_hWindow;
}

static DWORD WINAPI MainThread(LPVOID lpReserved) {
    Log::SetModule(static_cast<HMODULE>(lpReserved), "ZB2Menu");
    Log::Info("==================================================");
    Log::Infof("Log ativo em: %s", Log::GetPath());
    Log::Info("MainThread iniciada.");

    // Padrao mercado: hooks instalados imediatamente (HWND vem da swapchain no
    // 1o Present). Nenhuma espera de janela aqui — EnumWindowsCallback +
    // GetProcessWindow ficam so como fallback do hkPresent.
    // ETAPA UNICA: instala Present/Resize. WndProc hooka no 1o Present.
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

            Log::Info("Hooks instalados; WndProc hooka no 1o Present (HWND da swapchain).");
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
        HANDLE thread = CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
        if (thread) CloseHandle(thread);
    } else if (dwReason == DLL_PROCESS_DETACH) {
        Mono::Shutdown();
        // Process termination owns reclamation. Do not wait, log or destroy GUI
        // resources under the loader lock while callbacks may still be active.
    }

    return TRUE;
}






