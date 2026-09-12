#include "gui.h"
#include "config.h"
#include "log.h"
#include "classes.h"
#include "mono.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include <Windows.h>
#include <cstdio>
#include <string>

// ============================================================================
// GUI.CPP - Dear ImGui D3D11 (ZB2 Menu)
// OBJETIVO: menu 4 abas + overlay + preview interativo drag-and-drop.
// ORIGEM: kiero-dx9-base/gui.cpp; especificacao briefing Pt.4/Pt.6/Pt.7/Pt.8.
// TESTES: build Release|x64 0 erros; abrir com INSERT; arrastar Nome/Dist/Vida.
// HISTORICO: v0.1.0 menu base + preview; funcoes de jogo entram incremental.
// ============================================================================

// ---- Definicao do estado global ----
namespace Config {
    bool bMenuOpen = true;
    int  iMenuKey = VK_INSERT;
    bool bWatermark = true;
    bool bDebugOverlay = true;
    bool bTooltips = true;

    bool  bAimbot = false;
    int   iAimKey = VK_RBUTTON;
    int   iAimMode = 0;
    bool  bAutoAim = false;
    bool  bSilentAim = false;
    bool  bAutoFire = false;
    bool  bTriggerbot = false;
    bool  bVisibleCheck = true;
    int   iAimBone = 0;
    int   iAimPriority = 0;
    float fSmoothing = 8.0f;
    bool  bLimitFov = true;
    float fFovAngle = 90.0f;
    bool  b360Mode = false;
    bool  bDrawFov = true;
    float fMaxDistance = 200.0f;
    bool  bPrediction = false;
    float fLagComp = 50.0f;

    bool  bNoRecoil = false;
    bool  bNoSpread = false;
    bool  bNoSway = false;
    bool  bRapidFire = false;
    float fRapidMult = 2.0f;
    bool  bInfAmmo = false;
    bool  bInstantReload = false;
    bool  bFullAuto = false;
    bool  bSaitama = false;
    float fNadeTime = 2.0f;
    float fExplRadius = 8.0f;
    float fExplDamage = 500.0f;
    bool  bContactExpl = false;
    bool  bPowerDrop = false;

    char szItemSearch[64] = { 0 };
    int  iItemAmount = 1;

    bool  bSpeedHack = false;
    float fSpeedMult = 1.5f;
    bool  bSuperJump = false;
    float fJumpMult = 1.5f;
    bool  bInfStamina = false;
    bool  bRollSpeed = false;
    float fRollMult = 1.5f;

    bool  bZombieEsp = false;
    int   iZombieBox = 0;
    bool  bZombieName = true;
    bool  bZombieDist = true;
    bool  bZombieHp = true;
    bool  bZombiePct = true;
    float fPctX = -38.0f, fPctY = -16.0f;
    bool  bZombieSkeleton = false;
    bool  bZombieSnap = false;
    bool  bZombieHeadDot = false;
    float colZombieVis[4] = { 1, 0, 0, 1 };
    float colZombieInv[4] = { 0.5f, 0, 0, 1 };
    float colZombieNameVis[4] = { 1, 1, 1, 1 };
    float colZombieNameInv[4] = { 0.7f, 0.7f, 0.7f, 1 };
    float colZombieDistVis[4] = { 1, 1, 1, 1 };
    float colZombieDistInv[4] = { 0.7f, 0.7f, 0.7f, 1 };
    float colZombieHpVis[4] = { 0, 1, 0, 1 };
    float colZombieHpInv[4] = { 1, 0, 0, 1 };

    bool  bAllyEsp = false;
    int   iAllyBox = 0;
    bool  bAllyName = true;
    bool  bAllyDist = true;
    bool  bAllyHp = true;
    bool  bAllySkeleton = false;
    bool  bAllySnap = false;
    bool  bAllyHeadDot = false;
    float colAllyVis[4] = { 0.2f, 0.5f, 1.0f, 1 };
    float colAllyInv[4] = { 0.1f, 0.25f, 0.6f, 1 };

    bool  bChams = false;
    float colChamsVis[4] = { 1, 0, 0, 1 };
    float colChamsInv[4] = { 1, 1, 0, 1 };
    bool  bItemEsp = false;
    bool  bItemWeapons = true;
    bool  bItemRare = true;
    bool  bItemAmmo = true;
    bool  bItemSupply = true;
    bool  bPoiEsp = true;
    float colItem[4] = { 1, 0.85f, 0.2f, 1 };
    float fItemRadius = 150.0f;

    float fNameX = 0.0f, fNameY = -18.0f;
    float fDistX = 0.0f, fDistY = 4.0f;
    float fHpX = -8.0f, fHpY = -85.0f;
    int   iNameA = 0, iDistA = 6, iHpA = 3, iPctA = 0; // TL, BL, ML, TL
    int   iLayoutMode = 0;
    int   iLayoutSide = 0;
    float fLayoutOffset = 5.0f;
    float fLayoutSpacing = 2.0f;
    bool  bSnapGrid = true;
    float fSnapSize = 5.0f;
    bool  bShowGuides = true;
    bool  bAlignList = false;
    int   iListDir = 0;
    float fListSpacing = 2.0f;
    float fPreviewHp = 87.0f;

    bool  bEnemyMagnet = false;
    int   iMagnetKey = 0x48; // H
    int   iMagnetMode = 0;
    float fMagnetRadius = 60.0f;
    bool  bMagnetFreeze = false;
    bool  bKillOnSpawn = false;
    bool  bItemMagnet = false;
    int   iItemMagnetKey = 0x4A; // J
    int   iItemMagnetType = 4;
    float fItemMagnetRadius = 50.0f;
    bool  bAutoCollect = false;

    float fSaveX = 0, fSaveY = 0, fSaveZ = 0;

    float fDayHour = 12.0f;
    float fDaySpeed = 1.0f;
    int   iSpawnCount = 5;
    int   iSpawnBoss = 0;

    float fCamFov = 90.0f;
    bool  bThirdPerson = false;
    float fThirdDist = 4.0f;
    bool  bAntiAfk = false;
    bool  bNoClip = false;
    float fNoClipSpeed = 1.3f;
    bool  bNoFall = false;
}

namespace GUI {
    bool g_bHasFocusFix = false;
    static bool g_bInit = false;
    static char s_iniPath[MAX_PATH] = { 0 };
    static HWND s_hWnd = nullptr; // p/ clip do cursor durante o drag

    static const char* kAimBones[4] = { "Head", "Neck", "Chest", "Pelvis" };
    static const char* kAimPrio[3] = { "Closest to Crosshair", "Lowest HP", "Nearest Distance" };
    static const char* kBoxType[3] = { "2D", "3D", "Corners" };
    static const char* kBossName[3] = { "Riot", "Queen", "Reaper" };
    static const char* kMagType[5] = { "Armas", "Municao", "Loot", "Caixas", "Todos" };
    static const char* kLayout[3] = { "Personalizado", "Ao Lado do Box", "Topo/Base/Centro" };

    // P3: offsets do preview sao relativos ao box; clamp evita perder o elemento.
    static void ClampOff(float& v) {
        if (v < -300.0f) v = -300.0f;
        if (v > 300.0f) v = 300.0f;
    }

    static float SnapF(float v, float grid) {
        if (!Config::bSnapGrid || grid <= 0.01f) return v;
        return ((int)((v + grid * 0.5f) / grid)) * grid;
    }

    // Ancoras do ESP (0=TL 1=TC 2=TR 3=ML 4=MC 5=MR 6=BL 7=BC 8=BR).
    static ImVec2 AnchorPt(int a, ImVec2 mn, ImVec2 mx) {
        float cx = (mn.x + mx.x) * 0.5f, cy = (mn.y + mx.y) * 0.5f;
        switch (a) {
        case 0: return mn;
        case 1: return ImVec2(cx, mn.y);
        case 2: return ImVec2(mx.x, mn.y);
        case 3: return ImVec2(mn.x, cy);
        case 4: return ImVec2(cx, cy);
        case 5: return ImVec2(mx.x, cy);
        case 6: return ImVec2(mn.x, mx.y);
        case 7: return ImVec2(cx, mx.y);
        default: return ImVec2(mx.x, mx.y);
        }
    }
    static const char* AnchorName(int a) {
        static const char* n[9] = { "TL","TC","TR","ML","MC","MR","BL","BC","BR" };
        return (a >= 0 && a < 9) ? n[a] : "?";
    }
    // Ao soltar: ancora mais proxima + offset (posicao relativa ao box, qualquer tamanho).
    static void SnapEl(int& anchor, float& ox, float& oy, float bw, float bh) {
        if (bw < 0.01f || bh < 0.01f) return;
        ImVec2 oldP = AnchorPt(anchor, ImVec2(0, 0), ImVec2(bw, bh));
        float posX = oldP.x + ox, posY = oldP.y + oy; // posicao absoluta atual
        float rx = posX / bw, ry = posY / bh;
        int col = rx < 0.33f ? 0 : (rx > 0.66f ? 2 : 1);
        int row = ry < 0.33f ? 0 : (ry > 0.66f ? 2 : 1);
        int na = row * 3 + col;
        ImVec2 np = AnchorPt(na, ImVec2(0, 0), ImVec2(bw, bh));
        anchor = na;
        ox = posX - np.x; // mantem o visual: offset = pos - novaAncora
        oy = posY - np.y;
        ClampOff(ox); ClampOff(oy); // drop fora nao perde o elemento
    }

    // P3: drag pelo cursor do Windows (GetCursorPos) - funciona mesmo se o
    // MouseDelta do ImGui estiver zerado (Raw Input do jogo) ou instavel.
    struct DragCap { bool active = false; bool released = false; POINT start; float ox = 0, oy = 0; };
    static DragCap s_capName, s_capDist, s_capHp, s_capPct;

    // P3: durante o drag, prende o cursor na janela (soltar fora = drop invalido).
    static void DragClip(bool on) {
        if (!s_hWnd) return;
        if (on) {
            RECT r;
            if (!GetClientRect(s_hWnd, &r)) return;
            POINT ul = { r.left, r.top }, lr = { r.right, r.bottom };
            ClientToScreen(s_hWnd, &ul);
            ClientToScreen(s_hWnd, &lr);
            r.left = ul.x; r.top = ul.y; r.right = lr.x; r.bottom = lr.y;
            ClipCursor(&r);
        } else {
            if (!s_capName.active && !s_capDist.active && !s_capHp.active && !s_capPct.active)
                ClipCursor(nullptr);
        }
    }

    static void DragWin(const char* id, ImVec2 r0, ImVec2 r1, float* px, float* py, DragCap& cap, const char* tag = nullptr) {
        bool hov = ImGui::IsMouseHoveringRect(r0, r1);
        if (hov && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !cap.active) {
            cap.active = true;
            GetCursorPos(&cap.start);
            cap.ox = *px; cap.oy = *py;
            DragClip(true);
        }
        if (cap.active) {
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                cap.active = false;
                cap.released = true; // chamador faz SnapEl
                DragClip(false);
            } else {
                POINT c;
                GetCursorPos(&c);
                *px = SnapF(cap.ox + (float)(c.x - cap.start.x), Config::fSnapSize);
                *py = SnapF(cap.oy + (float)(c.y - cap.start.y), Config::fSnapSize);
                ClampOff(*px); ClampOff(*py); // P3: clamp com auto-ajuste
            }
        }
        if (hov) {
            if (tag) ImGui::SetTooltip("%s [%s] (arraste)", id, tag);
            else ImGui::SetTooltip("%s (arraste)", id);
        }
    }

    static bool HpHorizAnchor(int a) { // TOP_*/BOTTOM_*/CENTER = horizontal; ML/MR = vertical
        return a <= 2 || a >= 6 || a == 4;
    }

    void Initialize(HWND hWindow, ID3D11Device* pDevice, ID3D11DeviceContext* pContext) {
        if (g_bInit) return;
        s_hWnd = hWindow;
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        io.LogFilename = nullptr;
        const char* appDir = Log::GetDir();
        if (appDir && appDir[0]) {
            CreateDirectoryA(appDir, nullptr);
            _snprintf_s(s_iniPath, _TRUNCATE, "%s\\imgui.ini", appDir);
            io.IniFilename = s_iniPath;
        } else io.IniFilename = nullptr;
        ImGui::StyleColorsDark();
        ImGui_ImplWin32_Init(hWindow);
        ImGui_ImplDX11_Init(pDevice, pContext);
        g_bInit = true;
    }

    void Shutdown() {
        if (!g_bInit) return;
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        g_bInit = false;
    }

    static void Tip(const char* t) {
        if (!Config::bTooltips || !t) return;
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip("%s", t);
    }

    // Preview interativo: box fixo no centro; Nome/Dist/Vida arrastaveis.
    // Barra orienta sozinha: lados = vertical, topo/base = horizontal.
    void DrawEspPreview(ImVec2 origin, ImVec2 size) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 c = ImVec2(origin.x + size.x * 0.5f, origin.y + size.y * 0.5f);
        float bw = 110.0f, bh = 170.0f;
        ImVec2 b0 = ImVec2(c.x - bw * 0.5f, c.y - bh * 0.5f);
        ImVec2 b1 = ImVec2(c.x + bw * 0.5f, c.y + bh * 0.5f);

        dl->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y),
            IM_COL32(12, 14, 18, 255));
        if (Config::bShowGuides) {
            dl->AddLine(ImVec2(c.x, origin.y), ImVec2(c.x, origin.y + size.y), IM_COL32(60, 60, 70, 255));
            dl->AddLine(ImVec2(origin.x, c.y), ImVec2(origin.x + size.x, c.y), IM_COL32(60, 60, 70, 255));
        }
        // Box fixo + skeleton fake + head dot
        dl->AddRect(b0, b1, IM_COL32(255, 40, 40, 255), 0.0f, 0, 1.5f);
        ImVec2 head = ImVec2(c.x, b0.y - 6);
        dl->AddCircleFilled(head, 4.0f, IM_COL32(255, 40, 40, 255));
        dl->AddLine(ImVec2(c.x, b0.y + 18), ImVec2(c.x, b1.y - 30), IM_COL32(255, 120, 120, 255), 1.5f);
        dl->AddLine(ImVec2(origin.x + size.x * 0.5f, origin.y + size.y), ImVec2(c.x, b1.y), IM_COL32(120, 200, 255, 200), 1.0f);

        // Elemento de texto ancorado: pos = ancora(box) + offset; soltar = re-ancorar.
        auto dragEl = [&](const char* id, const char* txt, int& anchor, float* px, float* py, DragCap& cap, ImU32 col) {
            ImVec2 ap = AnchorPt(anchor, ImVec2(0, 0), ImVec2(bw, bh));
            ImVec2 tp = ImVec2(b0.x + ap.x + *px, b0.y + ap.y + *py);
            ImVec2 tsz = ImGui::CalcTextSize(txt);
            ImVec2 r0 = ImVec2(tp.x - 3, tp.y - 2), r1 = ImVec2(tp.x + tsz.x + 3, tp.y + tsz.y + 2);
            dl->AddRectFilled(r0, r1, ImGui::IsMouseHoveringRect(r0, r1) ? IM_COL32(50, 90, 140, 160) : IM_COL32(30, 34, 42, 160));
            dl->AddText(tp, col, txt);
            DragWin(id, r0, r1, px, py, cap, AnchorName(anchor));
            if (cap.released) { cap.released = false; SnapEl(anchor, *px, *py, bw, bh); }
        };

        if (Config::bZombieName)
            dragEl("Nome", "zumbi_01", Config::iNameA, &Config::fNameX, &Config::fNameY, s_capName, IM_COL32_WHITE);
        if (Config::bZombieDist) {
            char b[32]; _snprintf_s(b, _TRUNCATE, "%.0fm", 45.0f);
            dragEl("Distancia", b, Config::iDistA, &Config::fDistX, &Config::fDistY, s_capDist, IM_COL32(200, 220, 255, 255));
        }
        if (Config::bZombieHp) {
            // Barra auto-orientada pela ANCORA (nao pela posicao): TOP/BOTTOM = horizontal.
            bool horiz = HpHorizAnchor(Config::iHpA);
            ImVec2 ap = AnchorPt(Config::iHpA, ImVec2(0, 0), ImVec2(bw, bh));
            float pct = Config::fPreviewHp / 100.0f;
            float hc[4]; HpColor(Config::fPreviewHp, hc);
            ImU32 fill = IM_COL32((int)(hc[0]*255), (int)(hc[1]*255), (int)(hc[2]*255), 255);
            ImVec2 p0 = ImVec2(b0.x + ap.x + Config::fHpX, b0.y + ap.y + Config::fHpY);
            ImVec2 p1 = horiz ? ImVec2(p0.x + bw, p0.y + 8) : ImVec2(p0.x + 5, p0.y + bh);
            dl->AddRectFilled(p0, p1, IM_COL32(40, 40, 44, 255));
            if (horiz) dl->AddRectFilled(p0, ImVec2(p0.x + bw * pct, p1.y), fill);
            else { float fh = bh * pct; dl->AddRectFilled(ImVec2(p0.x, p1.y - fh), p1, fill); }
            char pb[16]; _snprintf_s(pb, _TRUNCATE, "%.0f%%", (double)Config::fPreviewHp);
            if (Config::bZombiePct) {
                // % tem ancora propria (independente da barra).
                ImVec2 pap = AnchorPt(Config::iPctA, ImVec2(0, 0), ImVec2(bw, bh));
                ImVec2 tp = ImVec2(b0.x + pap.x + Config::fPctX, b0.y + pap.y + Config::fPctY);
                ImVec2 tsz = ImGui::CalcTextSize(pb);
                ImVec2 q0 = ImVec2(tp.x - 3, tp.y - 2), q1 = ImVec2(tp.x + tsz.x + 3, tp.y + tsz.y + 2);
                dl->AddRectFilled(q0, q1, ImGui::IsMouseHoveringRect(q0, q1) ? IM_COL32(50, 90, 140, 160) : IM_COL32(30, 34, 42, 160));
                dl->AddText(tp, IM_COL32_WHITE, pb);
                DragWin("% Vida", q0, q1, &Config::fPctX, &Config::fPctY, s_capPct, AnchorName(Config::iPctA));
                if (s_capPct.released) { s_capPct.released = false; SnapEl(Config::iPctA, Config::fPctX, Config::fPctY, bw, bh); }
            }
            ImVec2 r0 = ImVec2(p0.x - 4, p0.y - 4), r1 = ImVec2(p1.x + 4, p1.y + 4);
            DragWin("Barra Vida", r0, r1, &Config::fHpX, &Config::fHpY, s_capHp, AnchorName(Config::iHpA));
            if (s_capHp.released) { s_capHp.released = false; SnapEl(Config::iHpA, Config::fHpX, Config::fHpY, bw, bh); }
        }
        char hpb[64]; _snprintf_s(hpb, _TRUNCATE, "HP preview: %.0f%%", (double)Config::fPreviewHp);
        dl->AddText(ImVec2(origin.x + 8, origin.y + size.y - 18), IM_COL32(150, 150, 160, 255), hpb);
    }

    static bool g_bEditMode = false; // P3: modo edicao de layout
    static float s_snap[8] = { 0 };  // snapshot p/ Cancelar

    // ---- Hotkeys com modal (Fase 1 item 3) ----
    // OBJETIVO: clique no botao -> modal captura a proxima tecla/mouse. ESC cancela.
    static int* s_capKey = nullptr;

    static void KeyName(int vk, char* out, size_t cap) {
        const char* mouse = nullptr;
        switch (vk) {
        case VK_LBUTTON: mouse = "Mouse Left"; break;
        case VK_RBUTTON: mouse = "Mouse Right"; break;
        case VK_MBUTTON: mouse = "Mouse Mid"; break;
        case VK_XBUTTON1: mouse = "Mouse X1"; break;
        case VK_XBUTTON2: mouse = "Mouse X2"; break;
        }
        if (mouse) { strncpy_s(out, cap, mouse, _TRUNCATE); return; }
        UINT sc = MapVirtualKeyA((UINT)vk, MAPVK_VK_TO_VSC);
        if (sc == 0) { _snprintf_s(out, cap, _TRUNCATE, "VK 0x%02X", vk); return; }
        LONG l = (LONG)(sc << 16);
        if (vk == VK_INSERT || vk == VK_DELETE || vk == VK_HOME || vk == VK_END ||
            vk == VK_PRIOR || vk == VK_NEXT || vk == VK_LEFT || vk == VK_UP ||
            vk == VK_RIGHT || vk == VK_DOWN || vk == VK_NUMLOCK)
            l |= (1 << 24); // extended bit p/ nomes corretos
        if (GetKeyNameTextA(l, out, (int)cap) == 0)
            _snprintf_s(out, cap, _TRUNCATE, "VK 0x%02X", vk);
    }

    // Botao de hotkey: mostra a tecla atual; clicando abre o modal de captura.
    static void HotkeyButton(const char* label, int* vk, const char* tip) {
        ImGui::PushID(label);
        ImGui::Text("%s", label);
        ImGui::SameLine(230);
        char nm[64] = { 0 };
        KeyName(*vk, nm, sizeof(nm));
        if (ImGui::Button(nm, ImVec2(150, 0))) {
            s_capKey = vk;
            ImGui::OpenPopup("hotkey_modal");
        }
        if (tip) Tip(tip);
        if (ImGui::BeginPopupModal("hotkey_modal", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Pressione a tecla para: %s", label);
            ImGui::TextDisabled("ESC cancela. Mouse tambem vale.");
            // Captura a primeira tecla RECEM-pressionada (borda de subida).
            static bool prev[256] = { false };
            bool cur[256] = { false };
            for (int v = 1; v < 256; ++v)
                cur[v] = (GetAsyncKeyState(v) & 0x8000) != 0;
            for (int v = 1; v < 256; ++v) {
                if (cur[v] && !prev[v]) {
                    if (v != VK_ESCAPE && s_capKey)
                        *s_capKey = v;
                    s_capKey = nullptr;
                    ImGui::CloseCurrentPopup();
                    break;
                }
            }
            memcpy(prev, cur, sizeof(prev));
            if (ImGui::Button("Cancelar")) {
                s_capKey = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        } else if (s_capKey == vk) {
            s_capKey = nullptr; // modal fechado por fora: solta a captura
        }
        ImGui::PopID();
    }

    // ---- Configs JSON (Fase 1 item 4) ----
    // OBJETIVO: salvar/carregar todo o estado do menu em Documents\<DLL>\configs\<preset>.json.
    // Sem libs externas: writer fprintf + parser por busca de chaves (tipos: bool/int/float/vec4/str).
    static char s_cfgName[64] = "default";
    static char s_cfgStatus[128] = { 0 };

    static void CfgDir(char* out, size_t cap) {
        out[0] = 0;
        const char* app = Log::GetDir();
        if (app && app[0]) _snprintf_s(out, cap, _TRUNCATE, "%s\\configs", app);
    }

    static void SaveConfig(const char* name) {
        using namespace Config;
        char dir[MAX_PATH] = { 0 }, path[MAX_PATH] = { 0 };
        CfgDir(dir, sizeof(dir));
        if (!dir[0]) { strncpy_s(s_cfgStatus, "Sem pasta de documentos.", _TRUNCATE); return; }
        CreateDirectoryA(dir, nullptr);
        _snprintf_s(path, _TRUNCATE, "%s\\%s.json", dir, name);
        FILE* f = nullptr;
        if (fopen_s(&f, path, "w") != 0 || !f) { strncpy_s(s_cfgStatus, "Falha ao salvar.", _TRUNCATE); return; }
        fprintf(f, "{\n");
#define JB(v) fprintf(f, "\"" #v "\":%s,\n", (v) ? "true" : "false")
#define JI(v) fprintf(f, "\"" #v "\":%d,\n", (int)(v))
#define JF(v) fprintf(f, "\"" #v "\":%f,\n", (double)(float)(v))
#define JV(v) fprintf(f, "\"" #v "\":[%f,%f,%f,%f],\n", (double)(v)[0], (double)(v)[1], (double)(v)[2], (double)(v)[3])
#define JS(v) fprintf(f, "\"" #v "\":\"%s\",\n", (v))
        JB(bMenuOpen); JI(iMenuKey); JB(bWatermark); JB(bDebugOverlay); JB(bTooltips);
        JB(bAimbot); JI(iAimKey); JI(iAimMode); JB(bAutoAim); JB(bSilentAim); JB(bAutoFire);
        JB(bTriggerbot); JB(bVisibleCheck); JI(iAimBone); JI(iAimPriority); JF(fSmoothing);
        JB(bLimitFov); JF(fFovAngle); JB(b360Mode); JB(bDrawFov); JF(fMaxDistance);
        JB(bPrediction); JF(fLagComp);
        JB(bNoRecoil); JB(bNoSpread); JB(bNoSway); JB(bRapidFire); JF(fRapidMult);
        JB(bInfAmmo); JB(bInstantReload); JB(bFullAuto); JB(bSaitama); JF(fNadeTime);
        JF(fExplRadius); JF(fExplDamage); JB(bContactExpl); JB(bPowerDrop);
        JS(szItemSearch); JI(iItemAmount);
        JB(bSpeedHack); JF(fSpeedMult); JB(bSuperJump); JF(fJumpMult); JB(bInfStamina);
        JB(bRollSpeed); JF(fRollMult);
        JB(bZombieEsp); JI(iZombieBox); JB(bZombieName); JB(bZombieDist);         JB(bZombieHp); JB(bZombiePct); JF(fPctX); JF(fPctY); JI(iNameA); JI(iDistA); JI(iHpA); JI(iPctA);
        JB(bZombieSkeleton); JB(bZombieSnap); JB(bZombieHeadDot);
        JV(colZombieVis); JV(colZombieInv); JV(colZombieNameVis); JV(colZombieNameInv);
        JV(colZombieDistVis); JV(colZombieDistInv); JV(colZombieHpVis); JV(colZombieHpInv);
        JB(bAllyEsp); JI(iAllyBox); JB(bAllyName); JB(bAllyDist); JB(bAllyHp);
        JB(bAllySkeleton); JB(bAllySnap); JB(bAllyHeadDot); JV(colAllyVis); JV(colAllyInv);
        JB(bChams); JV(colChamsVis); JV(colChamsInv); JB(bItemEsp); JB(bItemWeapons);
        JB(bItemRare); JB(bItemAmmo); JB(bItemSupply); JB(bPoiEsp); JV(colItem); JF(fItemRadius);
        JF(fNameX); JF(fNameY); JF(fDistX); JF(fDistY); JF(fHpX); JF(fHpY);
        JI(iLayoutMode); JI(iLayoutSide); JF(fLayoutOffset); JF(fLayoutSpacing);
        JB(bSnapGrid); JF(fSnapSize); JB(bShowGuides); JB(bAlignList); JI(iListDir);
        JF(fListSpacing); JF(fPreviewHp);
        JB(bEnemyMagnet); JI(iMagnetKey); JI(iMagnetMode); JF(fMagnetRadius);
        JB(bMagnetFreeze); JB(bKillOnSpawn); JB(bItemMagnet); JI(iItemMagnetKey);
        JI(iItemMagnetType); JF(fItemMagnetRadius); JB(bAutoCollect);
        JF(fSaveX); JF(fSaveY); JF(fSaveZ);
        JF(fDayHour); JF(fDaySpeed); JI(iSpawnCount); JI(iSpawnBoss);
        JF(fCamFov); JB(bThirdPerson); JF(fThirdDist); JB(bAntiAfk); JB(bNoClip);
        JF(fNoClipSpeed); JB(bNoFall);
#undef JB
#undef JI
#undef JF
#undef JV
#undef JS
        fprintf(f, "\"_v\":1\n}\n");
        fclose(f);
        // Reescreve com '{' inicial (mantem writer simples e valido).
        _snprintf_s(s_cfgStatus, _TRUNCATE, "Salvo: %s.json", name);
        Log::Infof("Config salva: %s", path);
    }

    static const char* CfgFind(const std::string& s, const char* k) {
        std::string q = std::string("\"") + k + "\"";
        size_t p = s.find(q);
        if (p == std::string::npos) return nullptr;
        p = s.find(':', p);
        if (p == std::string::npos) return nullptr;
        return s.c_str() + p + 1;
    }

    static void LoadConfig(const char* name) {
        using namespace Config;
        char dir[MAX_PATH] = { 0 }, path[MAX_PATH] = { 0 };
        CfgDir(dir, sizeof(dir));
        if (!dir[0]) { strncpy_s(s_cfgStatus, "Sem pasta de documentos.", _TRUNCATE); return; }
        _snprintf_s(path, _TRUNCATE, "%s\\%s.json", dir, name);
        FILE* f = nullptr;
        if (fopen_s(&f, path, "r") != 0 || !f) { _snprintf_s(s_cfgStatus, _TRUNCATE, "Preset '%s' nao existe.", name); return; }
        std::string s;
        char chunk[1024];
        size_t n;
        while ((n = fread(chunk, 1, sizeof(chunk), f)) > 0) s.append(chunk, n);
        fclose(f);
#define LB(v) do { const char* w = CfgFind(s, #v); if (w) (v) = (strncmp(w, "true", 4) == 0); } while (0)
#define LI(v) do { const char* w = CfgFind(s, #v); if (w) (v) = atoi(w); } while (0)
#define LF(v) do { const char* w = CfgFind(s, #v); if (w) (v) = (float)atof(w); } while (0)
#define LV(v) do { const char* w = CfgFind(s, #v); if (w && *w == '[') sscanf_s(w, "[%f,%f,%f,%f]", &(v)[0], &(v)[1], &(v)[2], &(v)[3]); } while (0)
#define LS(v) do { const char* w = CfgFind(s, #v); if (w) { while (*w == ' ' || *w == '\t') ++w; if (*w == '\"') { ++w; size_t i = 0; while (w[i] && w[i] != '\"' && i + 1 < sizeof(v)) { (v)[i] = w[i]; ++i; } (v)[i] = 0; } } } while (0)
        LB(bMenuOpen); LI(iMenuKey); LB(bWatermark); LB(bDebugOverlay); LB(bTooltips);
        LB(bAimbot); LI(iAimKey); LI(iAimMode); LB(bAutoAim); LB(bSilentAim); LB(bAutoFire);
        LB(bTriggerbot); LB(bVisibleCheck); LI(iAimBone); LI(iAimPriority); LF(fSmoothing);
        LB(bLimitFov); LF(fFovAngle); LB(b360Mode); LB(bDrawFov); LF(fMaxDistance);
        LB(bPrediction); LF(fLagComp);
        LB(bNoRecoil); LB(bNoSpread); LB(bNoSway); LB(bRapidFire); LF(fRapidMult);
        LB(bInfAmmo); LB(bInstantReload); LB(bFullAuto); LB(bSaitama); LF(fNadeTime);
        LF(fExplRadius); LF(fExplDamage); LB(bContactExpl); LB(bPowerDrop);
        LS(szItemSearch); LI(iItemAmount);
        LB(bSpeedHack); LF(fSpeedMult); LB(bSuperJump); LF(fJumpMult); LB(bInfStamina);
        LB(bRollSpeed); LF(fRollMult);
        LB(bZombieEsp); LI(iZombieBox); LB(bZombieName); LB(bZombieDist); LB(bZombieHp); LB(bZombiePct); LF(fPctX); LF(fPctY); LI(iNameA); LI(iDistA); LI(iHpA); LI(iPctA);
        LB(bZombieSkeleton); LB(bZombieSnap); LB(bZombieHeadDot);
        LV(colZombieVis); LV(colZombieInv); LV(colZombieNameVis); LV(colZombieNameInv);
        LV(colZombieDistVis); LV(colZombieDistInv); LV(colZombieHpVis); LV(colZombieHpInv);
        LB(bAllyEsp); LI(iAllyBox); LB(bAllyName); LB(bAllyDist); LB(bAllyHp);
        LB(bAllySkeleton); LB(bAllySnap); LB(bAllyHeadDot); LV(colAllyVis); LV(colAllyInv);
        LB(bChams); LV(colChamsVis); LV(colChamsInv); LB(bItemEsp); LB(bItemWeapons);
        LB(bItemRare); LB(bItemAmmo); LB(bItemSupply); LB(bPoiEsp); LV(colItem); LF(fItemRadius);
        LF(fNameX); LF(fNameY); LF(fDistX); LF(fDistY); LF(fHpX); LF(fHpY);
        LI(iLayoutMode); LI(iLayoutSide); LF(fLayoutOffset); LF(fLayoutSpacing);
        LB(bSnapGrid); LF(fSnapSize); LB(bShowGuides); LB(bAlignList); LI(iListDir);
        LF(fListSpacing); LF(fPreviewHp);
        LB(bEnemyMagnet); LI(iMagnetKey); LI(iMagnetMode); LF(fMagnetRadius);
        LB(bMagnetFreeze); LB(bKillOnSpawn); LB(bItemMagnet); LI(iItemMagnetKey);
        LI(iItemMagnetType); LF(fItemMagnetRadius); LB(bAutoCollect);
        LF(fSaveX); LF(fSaveY); LF(fSaveZ);
        LF(fDayHour); LF(fDaySpeed); LI(iSpawnCount); LI(iSpawnBoss);
        LF(fCamFov); LB(bThirdPerson); LF(fThirdDist); LB(bAntiAfk); LB(bNoClip);
        LF(fNoClipSpeed); LB(bNoFall);
#undef LB
#undef LI
#undef LF
#undef LV
#undef LS
        _snprintf_s(s_cfgStatus, _TRUNCATE, "Carregado: %s.json", name);
        Log::Infof("Config carregada: %s", path);
    }

    void RenderOverlay() {
        if (!g_bInit) return;
        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        ImGuiIO& io = ImGui::GetIO();
        if (Config::bWatermark)
            dl->AddText(ImVec2(10, 10), IM_COL32(120, 200, 255, 220), "ZB2 Menu | D3D11 | INSERT");
        // Debug overlay (Fase 2 item 6): dados VIVOS via reflection C++.
        if (Config::bDebugOverlay) {
            const Mono::State& st = Mono::Get();
            char b[256];
            if (st.ready)
                _snprintf_s(b, _TRUNCATE, "LOCAL HP %.0f | STAM %.0f | ALIADO %.0f | ZUMBIS %d (hp0 %.0f) | DAY %.2fh | ESP %d %.1fms | %.0f fps",
                    (double)st.localHp, (double)st.localStam, (double)st.allyHp,
                    st.zombies, (double)st.zHp0, (double)st.dayTime, st.espShown, (double)st.espMs, (double)io.Framerate);
            else
                _snprintf_s(b, _TRUNCATE, "MONO aguardando cena... | %.0f fps", (double)io.Framerate);
            dl->AddText(ImVec2(10, 26), IM_COL32(160, 255, 160, 200), b);
        }
        // ESP Zumbis Box (Fase 3 itens 7/11): head/pes ja em pixels Unity; inverte Y.
        if (Config::bZombieEsp) {
            Mono::EspEntry es[128];
            int n = Mono::GetEsp(es, 128);
            float H = io.DisplaySize.y;
            ImU32 col = ImGui::GetColorU32(ImVec4(Config::colZombieVis[0], Config::colZombieVis[1], Config::colZombieVis[2], Config::colZombieVis[3]));
            for (int i = 0; i < n; ++i) {
                float h = 0, w = 0, cx = 0, hy = 0, fy = 0;
                ImVec2 r0, r1;
                bool is3d = (Config::iZombieBox == 1 && es[i].has3d);
                if (is3d) {
                    // 3D real: bbox dos 8 cantos da AABB (labels ancoram nela).
                    float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
                    int nv = 0;
                    for (int k = 0; k < 8; ++k) {
                        if (!es[i].pv[k]) continue;
                        float sx = es[i].px[k], sy = H - es[i].py[k];
                        if (sx < x0) x0 = sx; if (sx > x1) x1 = sx;
                        if (sy < y0) y0 = sy; if (sy > y1) y1 = sy;
                        nv++;
                    }
                    if (nv < 2) continue;
                    r0 = ImVec2(x0, y0); r1 = ImVec2(x1, y1);
                    cx = (x0 + x1) * 0.5f; w = x1 - x0; h = y1 - y0; hy = y0; fy = y1;
                    if (h < 4.0f) continue;
                    // Melee: box maior que a tela prende na viewport em vez de sumir.
                    if (w > io.DisplaySize.x || h > io.DisplaySize.y) {
                        r0.x = r0.x < -100 ? -100 : r0.x; r0.y = r0.y < -100 ? -100 : r0.y;
                        r1.x = r1.x > io.DisplaySize.x + 100 ? io.DisplaySize.x + 100 : r1.x;
                        r1.y = r1.y > io.DisplaySize.y + 100 ? io.DisplaySize.y + 100 : r1.y;
                        if (r1.x - r0.x < 4 || r1.y - r0.y < 4) continue;
                    }
                } else {
                    float hx = es[i].headX, hy2 = H - es[i].headY;
                    float fx = es[i].footX, fy2 = H - es[i].footY;
                    h = fy2 - hy2; // altura head->pes
                    if (h < 4.0f) continue;
                    w = h * 0.6f; // zumbi largo + cabeca grande (print 02:30)
                    cx = fx; // pes como centro (estavel quando o zumbi inclina)
                    hy = hy2; fy = fy2;
                    r0 = ImVec2(cx - w * 0.5f, hy); r1 = ImVec2(cx + w * 0.5f, fy);
                    if (w > io.DisplaySize.x || h > io.DisplaySize.y) {
                        // Melee: prende na viewport em vez de sumir.
                        r0.x = r0.x < -100 ? -100 : r0.x; r0.y = r0.y < -100 ? -100 : r0.y;
                        r1.x = r1.x > io.DisplaySize.x + 100 ? io.DisplaySize.x + 100 : r1.x;
                        r1.y = r1.y > io.DisplaySize.y + 100 ? io.DisplaySize.y + 100 : r1.y;
                        if (r1.x - r0.x < 4 || r1.y - r0.y < 4) continue;
                    }
                }
                if (Config::iZombieBox == 0) {
                    if (r0.x < -10000 || r0.x > 10000 || r0.y < -10000 || r0.y > 10000) continue; // P1 render
                    if (r1.x < -10000 || r1.x > 10000 || r1.y < -10000 || r1.y > 10000) continue;
                    dl->AddRect(r0, r1, col, 0.0f, 0, 1.5f);
                } else if (Config::iZombieBox == 2) {
                    // Corners: 8 segmentos nos cantos (item 11).
                    float cl = h * 0.22f; if (cl > w * 0.45f) cl = w * 0.45f;
                    dl->AddLine(r0, ImVec2(r0.x + cl, r0.y), col, 1.5f);
                    dl->AddLine(r0, ImVec2(r0.x, r0.y + cl), col, 1.5f);
                    dl->AddLine(ImVec2(r1.x, r0.y), ImVec2(r1.x - cl, r0.y), col, 1.5f);
                    dl->AddLine(ImVec2(r1.x, r0.y), ImVec2(r1.x, r0.y + cl), col, 1.5f);
                    dl->AddLine(ImVec2(r0.x, r1.y), ImVec2(r0.x + cl, r1.y), col, 1.5f);
                    dl->AddLine(ImVec2(r0.x, r1.y), ImVec2(r0.x, r1.y - cl), col, 1.5f);
                    dl->AddLine(r1, ImVec2(r1.x - cl, r1.y), col, 1.5f);
                    dl->AddLine(r1, ImVec2(r1.x, r1.y - cl), col, 1.5f);
                } else if (Config::iZombieBox == 1 && es[i].has3d) {
                    // 3D real: 12 arestas da AABB (frente 4 + fundo 4 + profundidade 4).
                    static const int E[12][2] = { {0,1},{1,3},{3,2},{2,0},{4,5},{5,7},{7,6},{6,4},{0,4},{1,5},{2,6},{3,7} };
                    for (int e = 0; e < 12; ++e) {
                        int a = E[e][0], b = E[e][1];
                        if (!es[i].pv[a] || !es[i].pv[b]) continue;
                        ImVec2 pa = ImVec2(es[i].px[a], H - es[i].py[a]), pb2 = ImVec2(es[i].px[b], H - es[i].py[b]);
                        if (pa.x < -10000 || pa.x > 10000 || pa.y < -10000 || pa.y > 10000) continue; // P1 render
                        if (pb2.x < -10000 || pb2.x > 10000 || pb2.y < -10000 || pb2.y > 10000) continue;
                        dl->AddLine(pa, pb2, col, 1.2f);
                    }
                } else {
                    // 3D fallback (bounds indisponivel): face traseira deslocada + arestas.
                    ImVec2 d = ImVec2(w * 0.28f, -h * 0.10f);
                    ImVec2 b0 = ImVec2(r0.x + d.x, r0.y + d.y), b1 = ImVec2(r1.x + d.x, r1.y + d.y);
                    dl->AddRect(b0, b1, col, 0.0f, 0, 1.0f);
                    dl->AddRect(r0, r1, col, 0.0f, 0, 1.5f);
                    dl->AddLine(r0, b0, col, 1.0f); dl->AddLine(ImVec2(r1.x, r0.y), ImVec2(b1.x, b0.y), col, 1.0f);
                    dl->AddLine(ImVec2(r0.x, r1.y), ImVec2(b0.x, b1.y), col, 1.0f); dl->AddLine(r1, b1, col, 1.0f);
                }
                if (Config::bZombieDist) {
                    char db[32]; _snprintf_s(db, _TRUNCATE, "%.0fm", (double)es[i].dist);
                    ImU32 dcol = ImGui::GetColorU32(ImVec4(Config::colZombieDistVis[0], Config::colZombieDistVis[1], Config::colZombieDistVis[2], Config::colZombieDistVis[3]));
                    ImVec2 dap = AnchorPt(Config::iDistA, r0, r1);
                    dl->AddText(ImVec2(dap.x + Config::fDistX, dap.y + Config::fDistY), dcol, db);
                }
                if (Config::bZombieHp && es[i].maxHp > 0) {
                    // Barra ancorada + auto-orientada pela ancora (TOP/BOTTOM = horizontal).
                    float pct = es[i].hp / es[i].maxHp;
                    if (pct < 0) pct = 0; if (pct > 1) pct = 1;
                    float hc[4]; HpColor(pct * 100.0f, hc);
                    ImU32 hfill = ImGui::GetColorU32(ImVec4(hc[0], hc[1], hc[2], hc[3]));
                    ImVec2 hap = AnchorPt(Config::iHpA, r0, r1);
                    float bx = hap.x + Config::fHpX, by = hap.y + Config::fHpY;
                    char pb[16]; _snprintf_s(pb, _TRUNCATE, "%.0f%%", (double)(pct * 100.0));
                    if (HpHorizAnchor(Config::iHpA)) {
                        dl->AddRectFilled(ImVec2(bx, by), ImVec2(bx + w, by + 4), IM_COL32(40, 40, 44, 255));
                        dl->AddRectFilled(ImVec2(bx, by), ImVec2(bx + w * pct, by + 4), hfill);
                        if (Config::bZombiePct) {
                            ImVec2 pap = AnchorPt(Config::iPctA, r0, r1);
                            dl->AddText(ImVec2(pap.x + Config::fPctX, pap.y + Config::fPctY), IM_COL32_WHITE, pb);
                        }
                    } else {
                        dl->AddRectFilled(ImVec2(bx, by), ImVec2(bx + 3, by + h), IM_COL32(40, 40, 44, 255));
                        dl->AddRectFilled(ImVec2(bx, by + h * (1 - pct)), ImVec2(bx + 3, by + h), hfill);
                        if (Config::bZombiePct) {
                            ImVec2 pap = AnchorPt(Config::iPctA, r0, r1);
                            dl->AddText(ImVec2(pap.x + Config::fPctX, pap.y + Config::fPctY), IM_COL32_WHITE, pb);
                        }
                    }
                }
                if (Config::bZombieName && es[i].name[0]) {
                    ImU32 ncol = ImGui::GetColorU32(ImVec4(Config::colZombieNameVis[0], Config::colZombieNameVis[1], Config::colZombieNameVis[2], Config::colZombieNameVis[3]));
                    ImVec2 nap = AnchorPt(Config::iNameA, r0, r1);
                    dl->AddText(ImVec2(nap.x + Config::fNameX, nap.y + Config::fNameY), ncol, es[i].name);
                }
            }
        }
        if (Config::bDrawFov && Config::bAimbot && Config::bLimitFov && !Config::b360Mode) {
            ImVec2 sc = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
            float r = Config::fFovAngle * 4.0f;
            dl->AddCircle(sc, r, IM_COL32(120, 220, 255, 200), 64, 1.2f);
        }
    }

    void Render() {
        ImGui::GetIO().MouseDrawCursor = Config::bMenuOpen; // sync por frame (nada desenha cursor com menu fechado)
        if (!Config::bMenuOpen || !g_bInit) {
            s_capName.active = s_capDist.active = s_capHp.active = s_capPct.active = false;
            return;
        }
        { // P3 diagnostico: delta zerado = Raw Input (Hipótese 1); pos parada = foco/input (H2/H3)
            static int s_mlog = 0;
            if (++s_mlog % 600 == 0) {
                ImGuiIO& dio = ImGui::GetIO();
                POINT wp;
                GetCursorPos(&wp);
                Log::Infof("[MOUSE] delta=(%.1f,%.1f) imgui=(%.0f,%.0f) win=(%ld,%ld)",
                    (double)dio.MouseDelta.x, (double)dio.MouseDelta.y,
                    (double)dio.MousePos.x, (double)dio.MousePos.y, wp.x, wp.y);
            }
        }
        ImGui::SetNextWindowSize(ImVec2(860, 560), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("ZB2 Menu - Zumbi Blocks 2 (D3D11)", &Config::bMenuOpen)) { ImGui::End(); return; }

        if (ImGui::BeginTabBar("ZB2Tabs")) {
            // ---- 1 PLAYER ----
            if (ImGui::BeginTabItem("PLAYER")) {
                ImGui::Checkbox("Active Aimbot", &Config::bAimbot); Tip("Liga o aimbot (PvE: zumbis/chefes).");
                ImGui::Combo("Aim Bone", &Config::iAimBone, kAimBones, 4); Tip("Head/Neck/Chest/Pelvis.");
                ImGui::Combo("Aim Priority", &Config::iAimPriority, kAimPrio, 3);
                ImGui::SliderFloat("Smoothing", &Config::fSmoothing, 1, 30, "%.0f");
                ImGui::SliderFloat("Max Distance", &Config::fMaxDistance, 10, 500, "%.0fm");
                ImGui::Checkbox("Auto Aim", &Config::bAutoAim);
                ImGui::Checkbox("Silent Aim", &Config::bSilentAim);
                ImGui::Checkbox("Auto Fire", &Config::bAutoFire);
                ImGui::Checkbox("Triggerbot", &Config::bTriggerbot);
                ImGui::Checkbox("Visible Check", &Config::bVisibleCheck); Tip("Raycast camera->alvo.");
                ImGui::Separator();
                ImGui::Checkbox("Limit FOV", &Config::bLimitFov);
                ImGui::SliderFloat("FOV Angle", &Config::fFovAngle, 1, 360, "%.0f deg");
                ImGui::Checkbox("360 Mode", &Config::b360Mode); Tip("Ignora FOV, mira em 360 graus.");
                ImGui::Checkbox("Draw FOV Circle", &Config::bDrawFov);
                ImGui::Checkbox("Enable Prediction", &Config::bPrediction);
                ImGui::SliderFloat("Lag Comp", &Config::fLagComp, 0, 200, "%.0fms");
                ImGui::Separator();
                ImGui::Text("Weapon Mods");
                ImGui::Checkbox("No Recoil", &Config::bNoRecoil);
                ImGui::Checkbox("No Spread", &Config::bNoSpread);
                ImGui::Checkbox("No Sway", &Config::bNoSway);
                ImGui::Checkbox("Rapid Fire", &Config::bRapidFire);
                ImGui::SliderFloat("Rapid Mult", &Config::fRapidMult, 1, 5, "%.1fx");
                ImGui::Checkbox("Infinite Ammo", &Config::bInfAmmo);
                ImGui::Checkbox("Instant Reload", &Config::bInstantReload);
                ImGui::Checkbox("Full Auto for All", &Config::bFullAuto);
                ImGui::Checkbox("Saitama Mode (4M dano)", &Config::bSaitama);
                ImGui::SliderFloat("Nade Time", &Config::fNadeTime, 0, 10, "%.1fs");
                ImGui::SliderFloat("Explosion Radius (HOST)", &Config::fExplRadius, 1, 50, "%.0f");
                ImGui::SliderFloat("Explosion Damage", &Config::fExplDamage, 10, 10000, "%.0f");
                ImGui::Checkbox("Contact Explosion", &Config::bContactExpl);
                ImGui::Checkbox("Power Drop (400)", &Config::bPowerDrop);
                ImGui::Separator();
                ImGui::Text("Items");
                ImGui::InputText("Search Items", Config::szItemSearch, 64);
                ImGui::SliderInt("Amount", &Config::iItemAmount, 1, 9999);
                if (ImGui::Button("Primary")) {} Tip("Entrega no slot primario (Fase Items).");
                ImGui::SameLine(); if (ImGui::Button("Secondary")) {}
                ImGui::SameLine(); if (ImGui::Button("Melee")) {}
                ImGui::SameLine(); if (ImGui::Button("Add to Inventory")) {}
                ImGui::Separator();
                ImGui::Text("Movement");
                ImGui::Checkbox("Speed Hack", &Config::bSpeedHack);
                ImGui::SliderFloat("Speed Mult", &Config::fSpeedMult, 1, 5, "%.1fx");
                ImGui::Checkbox("Super Jump", &Config::bSuperJump);
                ImGui::SliderFloat("Jump Mult", &Config::fJumpMult, 1, 5, "%.1fx");
                ImGui::Checkbox("Infinite Stamina", &Config::bInfStamina);
                ImGui::Checkbox("Roll Speed", &Config::bRollSpeed);
                ImGui::SliderFloat("Roll Mult", &Config::fRollMult, 1, 5, "%.1fx");
                ImGui::EndTabItem();
            }
            // ---- 2 VISUAL ----
            if (ImGui::BeginTabItem("VISUAL")) {
                if (!g_bEditMode) {
                    if (ImGui::Button("Editar Layout do ESP")) {
                        s_snap[0] = Config::fNameX; s_snap[1] = Config::fNameY;
                        s_snap[2] = Config::fDistX; s_snap[3] = Config::fDistY;
                        s_snap[4] = Config::fHpX; s_snap[5] = Config::fHpY;
                        s_snap[6] = Config::fPctX; s_snap[7] = Config::fPctY;
                        g_bEditMode = true;
                    }
                    Tip("Abre o editor de layout (arrasto pelo cursor do Windows).");
                } else {
                    ImGui::Text("MODO EDICAO - arraste Nome, Dist, Vida, %% (clamp automatico)");
                    ImVec2 epv = ImGui::GetCursorScreenPos();
                    ImVec2 epsz = ImVec2(ImGui::GetContentRegionAvail().x, 420);
                    ImGui::InvisibleButton("pv_edit_zone", epsz);
                    DrawEspPreview(epv, epsz);
                    if (ImGui::Button("Salvar Layout")) { g_bEditMode = false; SaveConfig(s_cfgName); }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancelar")) {
                        Config::fNameX = s_snap[0]; Config::fNameY = s_snap[1];
                        Config::fDistX = s_snap[2]; Config::fDistY = s_snap[3];
                        Config::fHpX = s_snap[4]; Config::fHpY = s_snap[5];
                        Config::fPctX = s_snap[6]; Config::fPctY = s_snap[7];
                        g_bEditMode = false;
                    }
                    ImGui::EndTabItem();
                }
                if (!g_bEditMode) {
                ImGui::Checkbox("ESP Zumbis", &Config::bZombieEsp);
                ImGui::Combo("Box Zumbi", &Config::iZombieBox, kBoxType, 3);
                ImGui::Checkbox("Nome", &Config::bZombieName); ImGui::SameLine();
                ImGui::Checkbox("Distancia", &Config::bZombieDist); ImGui::SameLine();
                ImGui::Checkbox("Vida", &Config::bZombieHp); ImGui::SameLine();
                ImGui::Checkbox("%", &Config::bZombiePct); Tip("Porcentagem arrastavel no preview.");
                ImGui::Checkbox("Skeleton", &Config::bZombieSkeleton); ImGui::SameLine();
                ImGui::Checkbox("Snapline", &Config::bZombieSnap); ImGui::SameLine();
                ImGui::Checkbox("Head Dot", &Config::bZombieHeadDot);
                ImGui::ColorEdit4("Cor Visivel", Config::colZombieVis);
                ImGui::ColorEdit4("Cor Invisivel", Config::colZombieInv);
                ImGui::Separator();
                ImGui::Checkbox("ESP Aliados (azul)", &Config::bAllyEsp);
                ImGui::Combo("Box Aliado", &Config::iAllyBox, kBoxType, 3);
                ImGui::Checkbox("Nome##A", &Config::bAllyName); ImGui::SameLine();
                ImGui::Checkbox("Dist##A", &Config::bAllyDist); ImGui::SameLine();
                ImGui::Checkbox("Vida##A", &Config::bAllyHp);
                ImGui::ColorEdit4("Aliado Visivel", Config::colAllyVis);
                ImGui::ColorEdit4("Aliado Invisivel", Config::colAllyInv);
                ImGui::Separator();
                ImGui::Checkbox("Chams (corpo)", &Config::bChams);
                ImGui::ColorEdit4("Chams Vis", Config::colChamsVis);
                ImGui::ColorEdit4("Chams Inv", Config::colChamsInv);
                ImGui::Checkbox("ESP Itens", &Config::bItemEsp);
                ImGui::Checkbox("Armas", &Config::bItemWeapons); ImGui::SameLine();
                ImGui::Checkbox("Raros", &Config::bItemRare); ImGui::SameLine();
                ImGui::Checkbox("Municao", &Config::bItemAmmo); ImGui::SameLine();
                ImGui::Checkbox("Suprimento", &Config::bItemSupply);
                ImGui::Checkbox("Pontos (heli/chefe/missao)", &Config::bPoiEsp);
                ImGui::ColorEdit4("Cor Item", Config::colItem);
                ImGui::SliderFloat("Raio Item", &Config::fItemRadius, 10, 500, "%.0fm");
                ImGui::Separator();
                ImGui::Text("PREVIEW INTERATIVO (arraste Nome/Dist/Vida)");
                ImVec2 pv = ImGui::GetCursorScreenPos();
                ImVec2 psz = ImVec2(ImGui::GetContentRegionAvail().x, 300);
                ImGui::InvisibleButton("pv_zone", psz);
                DrawEspPreview(pv, psz);
                ImGui::Combo("Layout", &Config::iLayoutMode, kLayout, 3);
                ImGui::SliderFloat("HP simulado", &Config::fPreviewHp, 0, 100, "%.0f%%");
                if (ImGui::Button("Resetar Posicoes")) {
                    Config::fNameX = 0; Config::fNameY = -18; Config::iNameA = 0;
                    Config::fDistX = 0; Config::fDistY = 4; Config::iDistA = 6;
                    Config::fHpX = -8; Config::fHpY = -85; Config::iHpA = 3;
                    Config::fPctX = -38; Config::fPctY = -16; Config::iPctA = 0;
                }
                ImGui::TextDisabled("Ancoras: Nome[%s] Dist[%s] Vida[%s] %%[%s]",
                    AnchorName(Config::iNameA), AnchorName(Config::iDistA),
                    AnchorName(Config::iHpA), AnchorName(Config::iPctA));
                ImGui::Checkbox("Snap to Grid", &Config::bSnapGrid);
                ImGui::SliderFloat("Grid", &Config::fSnapSize, 1, 20, "%.0fpx");
                ImGui::Checkbox("Guias", &Config::bShowGuides);
                } // fim conteudo normal (modo edicao mostra so o preview)
                if (!g_bEditMode) ImGui::EndTabItem();
            }
            // ---- 3 MISC (sempre 3a aba) ----
            if (ImGui::BeginTabItem("MISC")) {
                ImGui::Checkbox("Enemy Magnet (H)", &Config::bEnemyMagnet);
                ImGui::SliderFloat("Raio Magnet", &Config::fMagnetRadius, 10, 300, "%.0fm");
                ImGui::Checkbox("Freeze Attracted", &Config::bMagnetFreeze);
                ImGui::Checkbox("Kill On Spawn", &Config::bKillOnSpawn);
                ImGui::Separator();
                ImGui::Checkbox("Item Magnet (J)", &Config::bItemMagnet);
                ImGui::Combo("Tipo", &Config::iItemMagnetType, kMagType, 5);
                ImGui::SliderFloat("Raio Item Magnet", &Config::fItemMagnetRadius, 10, 200, "%.0fm");
                ImGui::Checkbox("Auto Collect", &Config::bAutoCollect);
                ImGui::Separator();
                ImGui::Text("Teleports");
                ImGui::InputFloat3("XYZ", &Config::fSaveX);
                if (ImGui::Button("Salvar Posicao Atual")) {}
                ImGui::SameLine(); if (ImGui::Button("Teleportar")) {}
                ImGui::Separator();
                if (ImGui::Button("Ir p/ Municao")) {} ImGui::SameLine();
                if (ImGui::Button("Explosivos")) {} ImGui::SameLine();
                if (ImGui::Button("Upgrade")) {}
                if (ImGui::Button("Fogueira")) {} ImGui::SameLine();
                if (ImGui::Button("Mercador")) {}
                ImGui::Separator();
                ImGui::Text("Host Controls (HOST)");
                ImGui::SliderFloat("Hora do dia", &Config::fDayHour, 0, 24, "%.1fh");
                ImGui::SliderFloat("Vel. tempo", &Config::fDaySpeed, 1, 10, "%.1fx");
                ImGui::SliderInt("Qtd Spawn", &Config::iSpawnCount, 1, 100);
                if (ImGui::Button("Spawnar Zumbi")) {}
                ImGui::Combo("Boss", &Config::iSpawnBoss, kBossName, 3);
                ImGui::SameLine(); if (ImGui::Button("Spawnar Boss")) {}
                ImGui::Separator();
                ImGui::Text("Utilities");
                ImGui::SliderFloat("Camera FOV", &Config::fCamFov, 60, 120, "%.0f");
                ImGui::Checkbox("Third Person", &Config::bThirdPerson);
                ImGui::SliderFloat("Dist 3rd", &Config::fThirdDist, 1, 10, "%.1fm");
                ImGui::Checkbox("Anti-AFK", &Config::bAntiAfk);
                ImGui::Checkbox("NoClip (Space/Ctrl)", &Config::bNoClip);
                ImGui::SliderFloat("NoClip Vel", &Config::fNoClipSpeed, 0.5f, 5.0f, "%.1fx");
                ImGui::Checkbox("No Fall Damage", &Config::bNoFall);
                if (ImGui::Button("Revive Yourself")) {}
                ImGui::EndTabItem();
            }
            // ---- 4 SETTINGS ----
            if (ImGui::BeginTabItem("SETTINGS")) {
                ImGui::Text("HOTKEYS (clique e pressione a tecla)");
                HotkeyButton("Menu", &Config::iMenuKey, "Abre/fecha o menu (DELETE sempre funciona).");
                HotkeyButton("Aim Key", &Config::iAimKey, "Tecla do aimbot.");
                HotkeyButton("Enemy Magnet", &Config::iMagnetKey, "Ativa/posiciona o magnet (H).");
                HotkeyButton("Item Magnet", &Config::iItemMagnetKey, "Ativa/posiciona o item magnet (J).");
                ImGui::Separator();
                ImGui::Checkbox("Watermark", &Config::bWatermark);
                ImGui::Checkbox("Debug Overlay", &Config::bDebugOverlay);
                ImGui::Checkbox("Tooltips", &Config::bTooltips);
                ImGui::Separator();
                ImGui::Text("CONFIGS (JSON)");
                ImGui::InputText("Preset", s_cfgName, 64);
                if (ImGui::Button("Salvar")) SaveConfig(s_cfgName); Tip("Salva tudo em configs\\<preset>.json.");
                ImGui::SameLine();
                if (ImGui::Button("Carregar")) LoadConfig(s_cfgName); Tip("Carrega o preset (chaves ausentes mantem valor).");
                if (s_cfgStatus[0]) ImGui::TextDisabled("%s", s_cfgStatus);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    }
}
















