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
// Item 14/Passo 5: depth buffer readback (via principal do visible check).
// Copia 1x por Present ANTES do overlay; worker le via Map (stale = ultimo valido).
static ID3D11Texture2D*        g_depthStaging = nullptr;
static UINT                    g_depthW = 0, g_depthH = 0;
static DXGI_FORMAT             g_depthFmt = DXGI_FORMAT_UNKNOWN;
static CRITICAL_SECTION        g_depthCS;
static bool                    g_depthCSInit = false;
static float                   g_depthNdcW = 0.0f, g_depthNdcH = 0.0f; // viewport NDC real
static void ApplyGameClip(); // forward (definida antes do hkWndProc)
namespace DepthVis {
    // Publica o frame de profundidade p/ a worker (mono.cpp) sem acoplar modulos.
    void Publish(ID3D11DeviceContext* ctx, ID3D11Texture2D* staging, UINT w, UINT h, DXGI_FORMAT fmt, float ndcW, float ndcH);
    // Amostra NDC [0,1] no ultimo frame valido. Retorna false se indisponivel.
    bool Sample(float u, float v, float& outNdc);
    void Shutdown();
    void AuditTick(UINT w, UINT h, DXGI_FORMAT fmt, UINT samples, int hasDsv, int hasTex);
}
namespace Mono { namespace DepthVisShim {
    // Shim: corpo em main.cpp (TU com o DepthVis global). So declara aqui.
    bool Sample(float u, float v, float& outNdc);
} }
bool Mono::DepthVisShim::Sample(float u, float v, float& outNdc) { return ::DepthVis::Sample(u, v, outNdc); }

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
    if (Flags & DXGI_PRESENT_TEST) return oPresent(pSwapChain, SyncInterval, Flags); // teste oculto: nao desenha
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
    Mono::SetViewport(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);

    Mono::Tick();          // reflection (Fase 2 item 5: bind + leitura viva)
    { static int s_clipTick = 0;
      if (!Config::bMenuOpen && g_bInit && (++s_clipTick % 120 == 0)) ApplyGameClip(); }
    GUI::Render();         // janela do menu (4 abas)
    GUI::RenderOverlay();  // watermark/debug/FOV fora da janela

    ImGui::EndFrame();
    ImGui::Render();

    // Item 14/Passo 5: captura o depth ANTES do overlay (cena pura do jogo —
    // o OMSetRenderTargets do overlay troca o DSV; depois dele o OMGetRenderTargets
    // pode devolver DSV errado/nulo). Falha silenciosa = ultimo frame valido.
    // NOTA: este bloco roda DEPOIS de ImGui::Render() mas ANTES de RenderDrawData
    // (o draw do overlay ainda nao executou) — o DSV ainda e o da cena do jogo.
    {
        ID3D11DepthStencilView* dsv = nullptr;
        ID3D11Texture2D* depthTex = nullptr;
        D3D11_VIEWPORT vp[8] = {};
        UINT nvp = 8;
        if (g_pContext) {
            g_pContext->OMGetRenderTargets(1, nullptr, &dsv);
            g_pContext->RSGetViewports(&nvp, vp);
        }
        // Auditoria: registra o que o OM entrega (1-2x). Sem DSV aqui = sem depth.
        {
            ID3D11Resource* ares = nullptr;
            UINT aw = 0, ah = 0, as = 0;
            DXGI_FORMAT af = DXGI_FORMAT_UNKNOWN;
            int hasT = 0;
            if (dsv) {
                D3D11_TEXTURE2D_DESC atd = {};
                ID3D11Resource* ar2 = nullptr;
                dsv->GetResource(&ar2);
                if (ar2) {
                    ID3D11Texture2D* at = nullptr;
                    if (SUCCEEDED(ar2->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&at)) && at) {
                        at->GetDesc(&atd);
                        aw = atd.Width; ah = atd.Height; af = atd.Format; as = atd.SampleDesc.Count;
                        hasT = 1;
                        at->Release();
                    }
                    ar2->Release();
                }
            }
            DepthVis::AuditTick(aw, ah, af, as, dsv ? 1 : 0, hasT);
        }
        if (dsv) {
            D3D11_DEPTH_STENCIL_VIEW_DESC dd = {};
            dsv->GetDesc(&dd);
            ID3D11Resource* res = nullptr;
            dsv->GetResource(&res);
            if (res) {
                res->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&depthTex);
                res->Release();
            }
            if (depthTex) {
                D3D11_TEXTURE2D_DESC td = {};
                depthTex->GetDesc(&td);
                bool fmtOk = (td.Format == DXGI_FORMAT_D24_UNORM_S8_UINT ||
                              td.Format == DXGI_FORMAT_D32_FLOAT ||
                              td.Format == DXGI_FORMAT_D16_UNORM ||
                              td.Format == DXGI_FORMAT_R24G8_TYPELESS ||
                              td.Format == DXGI_FORMAT_R32_TYPELESS ||
                              td.Format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT);
                if (fmtOk && td.Width >= 64 && td.Height >= 64 && td.Width <= 8192 && td.Height <= 8192) {
                    if (!g_depthStaging || td.Width != g_depthW || td.Height != g_depthH || td.Format != g_depthFmt) {
                        if (g_depthStaging) { g_depthStaging->Release(); g_depthStaging = nullptr; }
                        D3D11_TEXTURE2D_DESC sd = {};
                        sd.Width = td.Width; sd.Height = td.Height;
                        sd.MipLevels = 1; sd.ArraySize = 1;
                        sd.Format = td.Format;
                        sd.SampleDesc.Count = 1;
                        sd.Usage = D3D11_USAGE_STAGING;
                        sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                        if (SUCCEEDED(g_pDevice->CreateTexture2D(&sd, nullptr, &g_depthStaging))) {
                            g_depthW = td.Width; g_depthH = td.Height; g_depthFmt = td.Format;
                        }
                    }
                    if (g_depthStaging && td.SampleDesc.Count == 1) {
                        // Somente nao-MSAA aqui (CopyResource exige mesma amostragem).
                        g_pContext->CopyResource(g_depthStaging, depthTex);
                        float vw = (nvp > 0 && vp[0].Width > 0) ? vp[0].Width : (float)td.Width;
                        float vh = (nvp > 0 && vp[0].Height > 0) ? vp[0].Height : (float)td.Height;
                        DepthVis::Publish(g_pContext, g_depthStaging, td.Width, td.Height, td.Format, vw, vh);
                    }
                    // MSAA: sem resolve dedicado nesta versao (log 1x, sem spam).
                    static bool s_msaaWarned = false;
                    if (td.SampleDesc.Count > 1 && !s_msaaWarned) {
                        s_msaaWarned = true;
                        Log::Warn("Depth MSAA>1: visible check usa fallback raycast.");
                    }
                }
                depthTex->Release();
            }
            dsv->Release();
        }
    }
    if (g_pContext && g_pRTV) {
        g_pContext->OMSetRenderTargets(1, &g_pRTV, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
    return oPresent(pSwapChain, SyncInterval, Flags);
}

namespace DepthVis {
    static ID3D11DeviceContext* s_ctx = nullptr;
    static ID3D11Texture2D* s_tex = nullptr;
    static UINT s_w = 0, s_h = 0;
    static DXGI_FORMAT s_fmt = DXGI_FORMAT_UNKNOWN;
    static float s_nw = 0, s_nh = 0;
    static int s_seq = 0, s_logged = 0;
    // Auditoria 14/09: contadores de captura (Publish) vs consumo (Sample).
    // Se published=0 => bloco de captura nunca publicou (DSV nulo/formato/MSAA).
    static long s_pubN = 0, s_mapOk = 0, s_mapFail = 0, s_msaaSkip = 0;
    static int s_statLogged = 0;
    void Publish(ID3D11DeviceContext* ctx, ID3D11Texture2D* staging, UINT w, UINT h, DXGI_FORMAT fmt, float ndcW, float ndcH) {
        if (!g_depthCSInit) { InitializeCriticalSection(&g_depthCS); g_depthCSInit = true; }
        EnterCriticalSection(&g_depthCS);
        s_ctx = ctx; s_tex = staging; s_w = w; s_h = h; s_fmt = fmt; s_nw = ndcW; s_nh = ndcH;
        s_seq++;
        s_pubN++;
        LeaveCriticalSection(&g_depthCS);
    }
    // Auditoria: de onde vem o DSV? Loga formato/dimensao/MSAA 1x (causa raiz).
    void AuditTick(UINT w, UINT h, DXGI_FORMAT fmt, UINT samples, int hasDsv, int hasTex) {
        if (s_statLogged >= 2) return;
        s_statLogged++;
        Log::Infof("[DEPTH-STAT] dsv=%d tex=%d %ux%u fmt=%d msaa=%u.",
            hasDsv, hasTex, w, h, (int)fmt, samples);
    }
    // Amostra o pixel (u,v em [0,1]) e retorna NDC decodificado por formato.
    // D24/D16: inteiro normalizado. D32: float direto. R24G8/R32: typeless views.
    bool Sample(float u, float v, float& outNdc) {
        if (!g_depthCSInit) return false;
        EnterCriticalSection(&g_depthCS);
        ID3D11DeviceContext* ctx = s_ctx;
        ID3D11Texture2D* tex = s_tex;
        UINT w = s_w, h = s_h;
        DXGI_FORMAT fmt = s_fmt;
        float nw = s_nw, nh = s_nh;
        LeaveCriticalSection(&g_depthCS);
        if (!ctx || !tex || w < 64 || h < 64) return false;
        // Converte NDC->texel usando a viewport REAL do jogo (nao DisplaySize).
        int x = (int)(u * nw), y = (int)(v * nh);
        if (x < 0) x = 0; if (y < 0) y = 0;
        if ((UINT)x >= w) x = (int)w - 1;
        if ((UINT)y >= h) y = (int)h - 1;
        D3D11_MAPPED_SUBRESOURCE mp = {};
        // Map em staging com READ e sem flags extras (dado do frame anterior e valido).
        if (FAILED(ctx->Map(tex, 0, D3D11_MAP_READ, 0, &mp))) {
            long f = 0;
            if (g_depthCSInit) { EnterCriticalSection(&g_depthCS); f = ++s_mapFail; LeaveCriticalSection(&g_depthCS); }
            if (f <= 2) Log::Warn("Depth Map falhou (dispositivo/staging).");
            return false;
        }
        if (g_depthCSInit) { EnterCriticalSection(&g_depthCS); s_mapOk++; LeaveCriticalSection(&g_depthCS); }
        bool ok = false;
        __try {
            if (fmt == DXGI_FORMAT_D32_FLOAT || fmt == DXGI_FORMAT_R32_TYPELESS) {
                const float* rows = (const float*)((const unsigned char*)mp.pData + (size_t)y * mp.RowPitch);
                float d = rows[x];
                if (d == d && d > 0.0f && d < 1.0f) { outNdc = d; ok = true; }
            } else if (fmt == DXGI_FORMAT_D24_UNORM_S8_UINT || fmt == DXGI_FORMAT_R24G8_TYPELESS) {
                const unsigned char* row = (const unsigned char*)mp.pData + (size_t)y * mp.RowPitch;
                unsigned d24 = row[x * 4 + 0] | ((unsigned)row[x * 4 + 1] << 8) | ((unsigned)row[x * 4 + 2] << 16);
                float d = (float)d24 / 16777215.0f;
                if (d > 0.0f && d < 1.0f) { outNdc = d; ok = true; }
            } else if (fmt == DXGI_FORMAT_D16_UNORM) {
                const unsigned short* rows = (const unsigned short*)((const unsigned char*)mp.pData + (size_t)y * mp.RowPitch);
                float d = (float)rows[x] / 65535.0f;
                if (d > 0.0f && d < 1.0f) { outNdc = d; ok = true; }
            } else if (fmt == DXGI_FORMAT_D32_FLOAT_S8X24_UINT) {
                const float* rows = (const float*)((const unsigned char*)mp.pData + (size_t)y * mp.RowPitch);
                // pitch em float: 8 bytes por pixel (D32 + stencil).
                const unsigned char* b = (const unsigned char*)mp.pData + (size_t)y * mp.RowPitch;
                float d = *(const float*)(b + (size_t)x * 8);
                if (d == d && d > 0.0f && d < 1.0f) { outNdc = d; ok = true; }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) { ok = false; }
        ctx->Unmap(tex, 0);
        return ok;
    }
    void Shutdown() {
        if (g_depthStaging) { g_depthStaging->Release(); g_depthStaging = nullptr; }
        if (g_depthCSInit) { DeleteCriticalSection(&g_depthCS); g_depthCSInit = false; }
        s_ctx = nullptr; s_tex = nullptr; s_w = s_h = 0;
    }
}

// WndProc: INSERT/DELETE alterna; cursor fix devolve controle ao jogo fechado.
static bool s_cursorHiddenByUs = false; // guarda: par abre/fecha a prova de key-repeat

// Prende o cursor na area cliente da janela (modo janela: sem isso a seta
// escapa e cliques caem fora do jogo). So com menu fechado e jogo em foco.
static void ApplyGameClip() {
    if (!g_hWindow) return;
    if (GetForegroundWindow() != g_hWindow) return;
    RECT r;
    if (!GetClientRect(g_hWindow, &r)) return;
    POINT ul = { r.left, r.top }, lr = { r.right, r.bottom };
    ClientToScreen(g_hWindow, &ul);
    ClientToScreen(g_hWindow, &lr);
    r.left = ul.x; r.top = ul.y; r.right = lr.x; r.bottom = lr.y;
    ClipCursor(&r);
}

static LRESULT CALLBACK hkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // Volta de ALT+TAB/foco: re-prende o cursor se o menu estiver fechado.
    if (uMsg == WM_ACTIVATE && LOWORD(wParam) != WA_INACTIVE && !Config::bMenuOpen && g_bInit) {
        ApplyGameClip();
        return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
    }
    if (uMsg == WM_KEYDOWN && ((int)wParam == Config::iMenuKey || wParam == VK_DELETE)) {
        Config::bMenuOpen = !Config::bMenuOpen;
        // Cursor (v0.7.1): idempotente por estado + diagnostico no log.
        // ShowCursor tem contador GLOBAL da sessao; builds antigas (loop)
        // podem ter deixado ele corrompido - o log abaixo denuncia (se ao
        // fechar o cnt ja estiver >=0, a sessao esta corrompida: reboot limpa).
        ImGuiIO& io = ImGui::GetIO();
        io.MouseDrawCursor = Config::bMenuOpen;
        if (!Config::bMenuOpen) {
            ApplyGameClip(); // re-prende na janela (jogo retoma camera/cursor travado)
            if (s_cursorHiddenByUs) {
                int c = ShowCursor(TRUE);
                s_cursorHiddenByUs = false;
                Log::Infof("Cursor restaurado p/ jogo (cnt=%d).", c);
            }
        } else {
            if (!s_cursorHiddenByUs) {
                int c = ShowCursor(FALSE);
                s_cursorHiddenByUs = true;
                Log::Infof("Cursor oculto p/ menu (cnt=%d).", c);
            }
        }
        return TRUE;
    }
    // Menu fechado (ou GUI ainda nao init): jogo processa tudo, sem tocar.
    if (!Config::bMenuOpen || !g_bInit)
        return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return TRUE;
    {
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
        ClipCursor(nullptr);
        if (oWndProc && g_hWindow)
            SetWindowLongPtr(g_hWindow, GWL_WNDPROC_INDEX, (LONG_PTR)oWndProc);
        CleanupRenderTarget();
        DepthVis::Shutdown();
        if (g_pContext) { g_pContext->Release(); g_pContext = nullptr; }
        if (g_pDevice) { g_pDevice->Release(); g_pDevice = nullptr; }
        GUI::Shutdown();
        kiero::shutdown();
        Log::Info("Hooks liberados.");
        Log::Shutdown();
    }
    return TRUE;
}






