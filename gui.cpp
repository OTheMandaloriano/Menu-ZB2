#include "gui.h"
#include "config.h"
#include "log.h"
#include "classes.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include <Windows.h>
#include <cstdio>

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
    float fHpX = -8.0f, fHpY = 0.0f;
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

    static const char* kAimBones[4] = { "Head", "Neck", "Chest", "Pelvis" };
    static const char* kAimPrio[3] = { "Closest to Crosshair", "Lowest HP", "Nearest Distance" };
    static const char* kBoxType[3] = { "2D", "3D", "Corners" };
    static const char* kBossName[3] = { "Riot", "Queen", "Reaper" };
    static const char* kMagType[5] = { "Armas", "Municao", "Loot", "Caixas", "Todos" };
    static const char* kLayout[3] = { "Personalizado", "Ao Lado do Box", "Topo/Base/Centro" };

    static float SnapF(float v, float grid) {
        if (!Config::bSnapGrid || grid <= 0.01f) return v;
        return ((int)((v + grid * 0.5f) / grid)) * grid;
    }

    void Initialize(HWND hWindow, ID3D11Device* pDevice, ID3D11DeviceContext* pContext) {
        if (g_bInit) return;
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

        auto dragText = [&](const char* id, const char* txt, float* px, float* py, ImU32 col) {
            ImVec2 tp = ImVec2(b0.x + *px, b0.y + *py);
            ImVec2 tsz = ImGui::CalcTextSize(txt);
            ImVec2 r0 = ImVec2(tp.x - 3, tp.y - 2), r1 = ImVec2(tp.x + tsz.x + 3, tp.y + tsz.y + 2);
            bool hov = ImGui::IsMouseHoveringRect(r0, r1);
            dl->AddRectFilled(r0, r1, hov ? IM_COL32(50, 90, 140, 160) : IM_COL32(30, 34, 42, 160));
            dl->AddText(tp, col, txt);
            if (hov && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                ImVec2 d = ImGui::GetIO().MouseDelta;
                *px = SnapF(*px + d.x, Config::fSnapSize);
                *py = SnapF(*py + d.y, Config::fSnapSize);
            }
            if (hov) ImGui::SetTooltip("%s (arraste)", id);
        };

        if (Config::bZombieName)
            dragText("Nome", "zumbi_01", &Config::fNameX, &Config::fNameY, IM_COL32_WHITE);
        if (Config::bZombieDist) {
            char b[32]; _snprintf_s(b, _TRUNCATE, "%.0fm", 45.0f);
            dragText("Distancia", b, &Config::fDistX, &Config::fDistY, IM_COL32(200, 220, 255, 255));
        }
        if (Config::bZombieHp) {
            // Orientacao automatica pela posicao da barra (briefing Pt.7.9)
            float cx = Config::fHpX, cy = Config::fHpY;
            bool side = (cx < -4.0f) || (cx > bw - 2.0f);
            bool topbot = !side;
            float pct = Config::fPreviewHp / 100.0f;
            float hc[4]; HpColor(Config::fPreviewHp, hc);
            ImU32 fill = IM_COL32((int)(hc[0]*255), (int)(hc[1]*255), (int)(hc[2]*255), 255);
            if (!side) {
                ImVec2 p0 = ImVec2(b0.x + cx, b0.y + cy), p1 = ImVec2(p0.x + bw, p0.y + 8);
                dl->AddRectFilled(p0, p1, IM_COL32(40, 40, 44, 255));
                dl->AddRectFilled(p0, ImVec2(p0.x + bw * pct, p1.y), fill);
                char pb[16]; _snprintf_s(pb, _TRUNCATE, "%.0f%%", (double)Config::fPreviewHp);
                dl->AddText(ImVec2(p1.x + 4, p0.y - 2), IM_COL32_WHITE, pb);
                ImVec2 r0 = ImVec2(p0.x - 3, p0.y - 3), r1 = ImVec2(p1.x + 34, p1.y + 3);
                if (ImGui::IsMouseHoveringRect(r0, r1) && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    ImVec2 d = ImGui::GetIO().MouseDelta;
                    Config::fHpX = SnapF(Config::fHpX + d.x, Config::fSnapSize);
                    Config::fHpY = SnapF(Config::fHpY + d.y, Config::fSnapSize);
                }
            } else {
                ImVec2 p0 = ImVec2(b0.x + cx, b0.y + cy), p1 = ImVec2(p0.x + 5, p0.y + bh);
                dl->AddRectFilled(p0, p1, IM_COL32(40, 40, 44, 255));
                float fh = bh * pct;
                dl->AddRectFilled(ImVec2(p0.x, p1.y - fh), p1, fill);
                char pb[16]; _snprintf_s(pb, _TRUNCATE, "%.0f%%", (double)Config::fPreviewHp);
                dl->AddText(ImVec2(p0.x - 8, p0.y - 16), IM_COL32_WHITE, pb);
                ImVec2 r0 = ImVec2(p0.x - 12, p0.y - 18), r1 = ImVec2(p1.x + 4, p1.y + 3);
                if (ImGui::IsMouseHoveringRect(r0, r1) && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    ImVec2 d = ImGui::GetIO().MouseDelta;
                    Config::fHpX = SnapF(Config::fHpX + d.x, Config::fSnapSize);
                    Config::fHpY = SnapF(Config::fHpY + d.y, Config::fSnapSize);
                }
            }
            (void)topbot;
        }
        char hpb[64]; _snprintf_s(hpb, _TRUNCATE, "HP preview: %.0f%%", (double)Config::fPreviewHp);
        dl->AddText(ImVec2(origin.x + 8, origin.y + size.y - 18), IM_COL32(150, 150, 160, 255), hpb);
    }

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

    void RenderOverlay() {
        if (!g_bInit) return;
        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        ImGuiIO& io = ImGui::GetIO();
        if (Config::bWatermark)
            dl->AddText(ImVec2(10, 10), IM_COL32(120, 200, 255, 220), "ZB2 Menu | D3D11 | INSERT");
        if (Config::bDebugOverlay && !Config::bMenuOpen) {
            char b[128]; _snprintf_s(b, _TRUNCATE, "LOCAL ? | HP ? | ZUMBIS ? | %.0f fps", (double)io.Framerate);
            dl->AddText(ImVec2(10, 26), IM_COL32(160, 255, 160, 200), b);
        }
        if (Config::bDrawFov && Config::bAimbot && Config::bLimitFov && !Config::b360Mode) {
            ImVec2 sc = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
            float r = Config::fFovAngle * 4.0f;
            dl->AddCircle(sc, r, IM_COL32(120, 220, 255, 200), 64, 1.2f);
        }
    }

    void Render() {
        if (!Config::bMenuOpen || !g_bInit) return;
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
                ImGui::Checkbox("ESP Zumbis", &Config::bZombieEsp);
                ImGui::Combo("Box Zumbi", &Config::iZombieBox, kBoxType, 3);
                ImGui::Checkbox("Nome", &Config::bZombieName); ImGui::SameLine();
                ImGui::Checkbox("Distancia", &Config::bZombieDist); ImGui::SameLine();
                ImGui::Checkbox("Vida", &Config::bZombieHp);
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
                    Config::fNameX = 0; Config::fNameY = -18;
                    Config::fDistX = 0; Config::fDistY = 4;
                    Config::fHpX = -8; Config::fHpY = 0;
                }
                ImGui::Checkbox("Snap to Grid", &Config::bSnapGrid);
                ImGui::SliderFloat("Grid", &Config::fSnapSize, 1, 20, "%.0fpx");
                ImGui::Checkbox("Guias", &Config::bShowGuides);
                ImGui::EndTabItem();
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
                ImGui::Text("Configs JSON em Documents\\<DLL> (Fase 1.4).");
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    }
}

