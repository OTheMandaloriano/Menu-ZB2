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

    float fNameX = 0.0f, fNameY = -16.0f;
    float fDistX = 0.0f, fDistY = 4.0f;
    float fHpX   = -6.0f, fHpY = 0.0f;   // ZERO ABSOLUTO: topo da barra alinhado com o topo do box!
    int   iNameA = 1, iDistA = 7, iHpA = 0, iPctA = 5; // legado v2 (migracao)
    int   iSideN = 0, iSideD = 1, iSideH = 2, iSideP = 3; // topo, base, esq, dir
    int   iAlinN = 1, iAlinD = 1, iAlinH = 1, iAlinP = 1; // centro
    int   iCfgVer = 2;
    float fPropN = 0.05f, fPropD = 0.05f, fPropH = 0.06f, fPropP = 0.06f; // Pilar 2
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
    static const char* kLayout[3] = { "Personalizado", "Classico Esquerda", "Classico Direita" };

    // A07: presets preenchem o MESMO modelo do arraste (lado/alin/nudge/prop).
    static void ApplyPreset(int p) {
        if (p == 1) { // Classico Esquerda: barra esq, nome topo, dist base, % dir
            Config::iSideN = 0; Config::iAlinN = 1; Config::fNameX = 0; Config::fNameY = -16;
            Config::iSideD = 1; Config::iAlinD = 1; Config::fDistX = 0; Config::fDistY = 4;
            Config::iSideH = 2; Config::iAlinH = 1; Config::fHpX = -6; Config::fHpY = 0;
            Config::iSideP = 3; Config::iAlinP = 1; Config::fPctX = 4; Config::fPctY = 0;
        } else if (p == 2) { // Classico Direita: espelho
            Config::iSideN = 0; Config::iAlinN = 1; Config::fNameX = 0; Config::fNameY = -16;
            Config::iSideD = 1; Config::iAlinD = 1; Config::fDistX = 0; Config::fDistY = 4;
            Config::iSideH = 3; Config::iAlinH = 1; Config::fHpX = 6; Config::fHpY = 0;
            Config::iSideP = 2; Config::iAlinP = 1; Config::fPctX = -4; Config::fPctY = 0;
        }
        Config::iLayoutMode = p;
    }

    // P3: offsets do preview sao relativos ao box; clamp evita perder o elemento.
    static void ClampOff(float& v) {
        // Trava de respiro: nenhum elemento se afasta mais que 25px da entidade.
        if (v < -25.0f) v = -25.0f;
        if (v >  25.0f) v =  25.0f;
    }

    static float SnapF(float v, float grid) {
        // A08: arredondamento simetrico e idempotente (snap(snap(x))==snap(x)).
        if (!Config::bSnapGrid || grid <= 0.01f) return v;
        float q = v / grid;
        float r = (q >= 0.0f) ? floorf(q + 0.5f) : -floorf(-q + 0.5f);
        return r * grid;
    }

    // Drop: lado pela borda mais proxima do centro (lado atual vence empates em 6px),
    // alinhamento por t, nudge = sobra clampada em ±60. Sem salto: posicao preservada.
    static ImVec2 PlaceEl(int side, int align, float ew, float eh, float gap, ImVec2 mn, ImVec2 mx);
    static float ElGap(int side, float prop, float bw, float bh);
    static void DropEl(int& side, int& al, float& nx, float& ny, float prop, float ew, float eh, float bw, float bh, ImVec2 dropTL) {
        float cx = dropTL.x + ew * 0.5f, cy = dropTL.y + eh * 0.5f;
        float d[4] = { fabsf(cy - 0.0f), fabsf(cy - bh), fabsf(cx - 0.0f), fabsf(cx - bw) };
        if (side >= 0 && side < 4) d[side] -= 6.0f; // histerese
        int ns = 0;
        for (int i = 1; i < 4; ++i) if (d[i] < d[ns]) ns = i;
        float t = 0.5f;
        if (ns == 0 || ns == 1) t = bw > 0.01f ? cx / bw : 0.5f;
        else t = bh > 0.01f ? cy / bh : 0.5f;
        if (t < 0) t = 0; if (t > 1) t = 1;
        int na = t < 0.33f ? 0 : (t > 0.66f ? 2 : 1);
        ImVec2 mn = ImVec2(0, 0), mx = ImVec2(bw, bh);
        float gap = ElGap(ns, prop, bw, bh);
        ImVec2 base = PlaceEl(ns, na, ew, eh, gap, mn, mx);
        side = ns; al = na;
        nx = dropTL.x - base.x; ny = dropTL.y - base.y;
        if (nx < -60) nx = -60; if (nx > 60) nx = 60;
        if (ny < -60) ny = -60; if (ny > 60) ny = 60;
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

    static DragCap* s_dragOwner = nullptr; // A09: captura unica (sobreposicao resolve por ordem)

    static void DragWin(const char* id, ImVec2 r0, ImVec2 r1, float* px, float* py, DragCap& cap, const char* tag = nullptr) {
        bool hov = ImGui::IsMouseHoveringRect(r0, r1);
        if (hov && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !cap.active && !s_dragOwner) {
            cap.active = true;
            s_dragOwner = &cap;
            GetCursorPos(&cap.start);
            cap.ox = *px; cap.oy = *py;
            DragClip(true);
        }
        if (cap.active) {
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                cap.active = false;
                cap.released = true; // chamador faz SnapEl
                if (s_dragOwner == &cap) s_dragOwner = nullptr;
                DragClip(false);
            } else if (s_dragOwner != &cap) {
                cap.active = false; // outro dono assumiu: solta sem confirmar
                if (s_dragOwner == &cap) s_dragOwner = nullptr;
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

    static bool HpHorizAnchor(int a) {
        // Legado: orientacao agora vem do LADO (PlaceEl). Mantido p/ compat.
        return (a == 1 || a == 7);
    }

    // Layout lado+alinhamento (SSOT preview+jogo). side:0=topo 1=base 2=esq 3=dir.
    // align: 0=inicio 1=centro 2=fim. Retorna o CANTO SUPERIOR ESQUERDO do
    // retangulo do elemento (ew,eh medidos), totalmente fora do box + gap.
    static ImVec2 PlaceEl(int side, int align, float ew, float eh, float gap, ImVec2 mn, ImVec2 mx) {
        if (side < 0 || side > 3) side = 0;
        if (align < 0 || align > 2) align = 1;
        float x, y;
        if (side == 0) {
            y = mn.y - gap - eh;
            x = align == 0 ? mn.x : (align == 1 ? (mn.x + mx.x) * 0.5f - ew * 0.5f : mx.x - ew);
        } else if (side == 1) {
            y = mx.y + gap;
            x = align == 0 ? mn.x : (align == 1 ? (mn.x + mx.x) * 0.5f - ew * 0.5f : mx.x - ew);
        } else if (side == 2) {
            x = mn.x - gap - ew;
            y = align == 0 ? mn.y : (align == 1 ? (mn.y + mx.y) * 0.5f - eh * 0.5f : mx.y - eh);
        } else {
            x = mx.x + gap;
            y = align == 0 ? mn.y : (align == 1 ? (mn.y + mx.y) * 0.5f - eh * 0.5f : mx.y - eh);
        }
        return ImVec2(x, y);
    }
    // Gap = 8px minimos + extra proporcional (lado esq/dir usa largura, resto altura).
    static float ElGap(int side, float prop, float bw, float bh) {
        float ref = (side == 2 || side == 3) ? bw : bh;
        if (ref < 1.0f) ref = 1.0f;
        float g = 8.0f + (prop > 0.0f ? prop * ref : 0.0f);
        if (g > 68.0f) g = 68.0f;
        return g;
    }
    static bool BarHorizSide(int side) { return side == 0 || side == 1; }

    // Pilar 2: offset hibrido = base (usuario) + extra proporcional clampado.
    // ref = largura (ancoras laterais) ou altura (demais). extra=0 se prop=0.
    static ImVec2 HybridPos(int anchor, float baseX, float baseY, float prop, float minE, float maxE, ImVec2 mn, ImVec2 mx) {
        ImVec2 ap = AnchorPt(anchor, mn, mx);
        // Laterais E cantos usam largura; topo/base/centro usam altura.
        bool useW = (anchor == 3 || anchor == 5 || anchor == 0 || anchor == 2 || anchor == 6 || anchor == 8);
        float ref = useW ? (mx.x - mn.x) : (mx.y - mn.y);
        if (ref < 1.0f) ref = 1.0f;
        // Respiro minimo escala com a distancia: 4px longe, ate 15px perto.
        float minResp = ref * 0.15f;
        if (minResp < 4.0f) minResp = 4.0f;
        if (minResp > 15.0f) minResp = 15.0f;
        (void)minE; // min de tabela absorvido pelo minResp dinamico
        float extra = prop * ref;
        if (prop <= 0.0f) extra = 0.0f; // kill switch: prop 0 = sem respiro
        else { if (extra < minResp) extra = minResp; if (extra > maxE) extra = maxE; }
        int col = anchor % 3, row = anchor / 3;
        float dx = col == 0 ? -1.0f : (col == 2 ? 1.0f : 0.0f);
        float dy = row == 0 ? -1.0f : (row == 2 ? 1.0f : 0.0f);
        // Base do usuario contida no envelope seguro.
        float bx = baseX < -25.0f ? -25.0f : (baseX > 25.0f ? 25.0f : baseX);
        float by = baseY < -25.0f ? -25.0f : (baseY > 25.0f ? 25.0f : baseY);
        return ImVec2(ap.x + bx + dx * extra, ap.y + by + dy * extra);
    }

    // Pilar 6: base calibrada de fabrica (v2). Zera Nome/% (sem colisao),
    // barra no TOPO da lateral (acompanha qualquer altura), % independente.
    static void ApplyFactoryDefaults() {
        Config::iNameA = 1; Config::fNameX = 0.0f; Config::fNameY = -16.0f; Config::fPropN = 0.05f; // TC
        Config::iDistA = 7; Config::fDistX = 0.0f; Config::fDistY = 4.0f;   Config::fPropD = 0.05f; // BC
        Config::iHpA = 0;   Config::fHpX = -6.0f;  Config::fHpY = 0.0f;    Config::fPropH = 0.06f; // TL: topo com topo
        Config::iPctA = 5;  Config::fPctX = 4.0f;  Config::fPctY = 0.0f;   Config::fPropP = 0.06f; // MR
        Config::iCfgVer = 2;
    }

    static void LoadConfig(const char* name); // forward (definida abaixo)
    static bool SaveConfig(const char* name); // forward (definida abaixo)

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
        // Correcao 2: config valida ja na injecao (fabrica v2 se legado/ausente).
        LoadConfig("default");
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
        ImGui::PushClipRect(origin, ImVec2(origin.x + size.x, origin.y + size.y), true); // A11: clip do canvas
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

        // Modelo lado+alinhamento (SSOT com o jogo). idx: 0=nome 1=dist 2=vida 3=pct.
        int* pSide[4] = { &Config::iSideN, &Config::iSideD, &Config::iSideH, &Config::iSideP };
        int* pAl[4] = { &Config::iAlinN, &Config::iAlinD, &Config::iAlinH, &Config::iAlinP };
        float* pNx[4] = { &Config::fNameX, &Config::fDistX, &Config::fHpX, &Config::fPctX };
        float* pNy[4] = { &Config::fNameY, &Config::fDistY, &Config::fHpY, &Config::fPctY };
        float* pPr[4] = { &Config::fPropN, &Config::fPropD, &Config::fPropH, &Config::fPropP };
        bool* pEn[4] = { &Config::bZombieName, &Config::bZombieDist, &Config::bZombieHp, &Config::bZombiePct };
        DragCap* pCap[4] = { &s_capName, &s_capDist, &s_capHp, &s_capPct };
        const char* pId[4] = { "Nome", "Distancia", "Barra Vida", "% Vida" };
        const char* sideNm[4] = { "TOPO", "BASE", "ESQ", "DIR" };
        const char* alNm[3] = { "INI", "CENTRO", "FIM" };

        // Texto lado+alinhamento com retangulo medido (fora do box, com gap).
        auto dragEl = [&](int e, const char* txt, ImU32 col) {
            ImVec2 tsz = ImGui::CalcTextSize(txt);
            ImVec2 tp = PlaceEl(*pSide[e], *pAl[e], tsz.x, tsz.y, ElGap(*pSide[e], *pPr[e], bw, bh), ImVec2(0, 0), ImVec2(bw, bh));
            tp.x += *pNx[e]; tp.y += *pNy[e];
            ImVec2 gp = ImVec2(b0.x + tp.x, b0.y + tp.y);
            ImVec2 r0 = ImVec2(gp.x - 4, gp.y - 3), r1 = ImVec2(gp.x + tsz.x + 4, gp.y + tsz.y + 3);
            dl->AddRectFilled(r0, r1, ImGui::IsMouseHoveringRect(r0, r1) ? IM_COL32(50, 90, 140, 160) : IM_COL32(30, 34, 42, 160));
            dl->AddText(gp, col, txt);
            char tag[32]; _snprintf_s(tag, _TRUNCATE, "%s %s", sideNm[*pSide[e] < 0 || *pSide[e] > 3 ? 0 : *pSide[e]], alNm[*pAl[e] < 0 || *pAl[e] > 2 ? 1 : *pAl[e]]);
            DragWin(pId[e], r0, r1, pNx[e], pNy[e], *pCap[e], tag);
            if (pCap[e]->released) {
                pCap[e]->released = false;
                DropEl(*pSide[e], *pAl[e], *pNx[e], *pNy[e], *pPr[e], tsz.x, tsz.y, bw, bh, ImVec2(tp.x - b0.x, tp.y - b0.y));
                Config::iLayoutMode = 0; // arraste manual = Personalizado (A07)
            }
        };

        if (*pEn[0])
            dragEl(0, "zumbi_01", IM_COL32_WHITE);
        if (*pEn[1]) {
            char b[32]; _snprintf_s(b, _TRUNCATE, "%.0fm", 45.0f);
            dragEl(1, b, IM_COL32(200, 220, 255, 255));
        }
        if (*pEn[2]) {
            // Barra pelo LADO (esq/dir = vertical 5xH; topo/base = horizontal Wx5).
            bool horiz = BarHorizSide(*pSide[2]);
            float pct = Config::fPreviewHp / 100.0f;
            if (pct < 0) pct = 0; if (pct > 1) pct = 1;
            float hc[4]; HpColor(Config::fPreviewHp, hc);
            ImU32 fill = IM_COL32((int)(hc[0]*255), (int)(hc[1]*255), (int)(hc[2]*255), 255);
            float blen = horiz ? bw : bh;
            ImVec2 bp = PlaceEl(*pSide[2], *pAl[2], horiz ? blen : 5.0f, horiz ? 5.0f : blen,
                ElGap(*pSide[2], *pPr[2], bw, bh), ImVec2(0, 0), ImVec2(bw, bh));
            ImVec2 p0 = ImVec2(b0.x + bp.x + *pNx[2], b0.y + bp.y + *pNy[2]);
            ImVec2 p1 = horiz ? ImVec2(p0.x + blen, p0.y + 5) : ImVec2(p0.x + 5, p0.y + blen);
            dl->AddRectFilled(p0, p1, IM_COL32(40, 40, 44, 255));
            if (horiz) dl->AddRectFilled(p0, ImVec2(p0.x + blen * pct, p1.y), fill);
            else dl->AddRectFilled(ImVec2(p0.x, p0.y + blen * (1 - pct)), p1, fill);
            ImVec2 r0 = ImVec2(p0.x - 4, p0.y - 4), r1 = ImVec2(p1.x + 4, p1.y + 4);
            char tag[32]; _snprintf_s(tag, _TRUNCATE, "lado %s", sideNm[*pSide[2] < 0 || *pSide[2] > 3 ? 2 : *pSide[2]]);
            DragWin(pId[2], r0, r1, pNx[2], pNy[2], *pCap[2], tag);
            if (pCap[2]->released) {
                pCap[2]->released = false;
                DropEl(*pSide[2], *pAl[2], *pNx[2], *pNy[2], *pPr[2], p1.x - p0.x, p1.y - p0.y, bw, bh,
                    ImVec2(p0.x - b0.x, p0.y - b0.y));
                Config::iLayoutMode = 0;
            }
        }
        // Pilar 4: % independente da barra grafica.
        if (*pEn[3]) {
            char pb[16]; _snprintf_s(pb, _TRUNCATE, "%.0f%%", (double)Config::fPreviewHp);
            dragEl(3, pb, IM_COL32_WHITE);
        }
        char hpb[64]; _snprintf_s(hpb, _TRUNCATE, "HP preview: %.0f%%", (double)Config::fPreviewHp);
        dl->AddText(ImVec2(origin.x + 8, origin.y + size.y - 18), IM_COL32(150, 150, 160, 255), hpb);
        ImGui::PopClipRect(); // A11
    }

    static bool g_bEditMode = false; // P3: modo edicao de layout
    // A06: snapshot COMPLETO do layout (lados, alinhamentos, nudges, props, toggles).
    struct LayoutSnap { int side[4], al[4]; float nx[4], ny[4], pr[4]; bool en[4]; };
    static LayoutSnap s_snap;
    static void SnapshotLayout() {
        s_snap.side[0] = Config::iSideN; s_snap.al[0] = Config::iAlinN;
        s_snap.nx[0] = Config::fNameX; s_snap.ny[0] = Config::fNameY; s_snap.pr[0] = Config::fPropN; s_snap.en[0] = Config::bZombieName;
        s_snap.side[1] = Config::iSideD; s_snap.al[1] = Config::iAlinD;
        s_snap.nx[1] = Config::fDistX; s_snap.ny[1] = Config::fDistY; s_snap.pr[1] = Config::fPropD; s_snap.en[1] = Config::bZombieDist;
        s_snap.side[2] = Config::iSideH; s_snap.al[2] = Config::iAlinH;
        s_snap.nx[2] = Config::fHpX; s_snap.ny[2] = Config::fHpY; s_snap.pr[2] = Config::fPropH; s_snap.en[2] = Config::bZombieHp;
        s_snap.side[3] = Config::iSideP; s_snap.al[3] = Config::iAlinP;
        s_snap.nx[3] = Config::fPctX; s_snap.ny[3] = Config::fPctY; s_snap.pr[3] = Config::fPropP; s_snap.en[3] = Config::bZombiePct;
    }
    static void RestoreLayout() {
        Config::iSideN = s_snap.side[0]; Config::iAlinN = s_snap.al[0];
        Config::fNameX = s_snap.nx[0]; Config::fNameY = s_snap.ny[0]; Config::fPropN = s_snap.pr[0]; Config::bZombieName = s_snap.en[0];
        Config::iSideD = s_snap.side[1]; Config::iAlinD = s_snap.al[1];
        Config::fDistX = s_snap.nx[1]; Config::fDistY = s_snap.ny[1]; Config::fPropD = s_snap.pr[1]; Config::bZombieDist = s_snap.en[1];
        Config::iSideH = s_snap.side[2]; Config::iAlinH = s_snap.al[2];
        Config::fHpX = s_snap.nx[2]; Config::fHpY = s_snap.ny[2]; Config::fPropH = s_snap.pr[2]; Config::bZombieHp = s_snap.en[2];
        Config::iSideP = s_snap.side[3]; Config::iAlinP = s_snap.al[3];
        Config::fPctX = s_snap.nx[3]; Config::fPctY = s_snap.ny[3]; Config::fPropP = s_snap.pr[3]; Config::bZombiePct = s_snap.en[3];
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

    static bool SaveConfig(const char* name) {
        using namespace Config;
        char dir[MAX_PATH] = { 0 }, path[MAX_PATH] = { 0 }, tmp[MAX_PATH] = { 0 };
        CfgDir(dir, sizeof(dir));
        if (!dir[0]) { strncpy_s(s_cfgStatus, "Sem pasta de documentos.", _TRUNCATE); return false; }
        CreateDirectoryA(dir, nullptr);
        _snprintf_s(path, _TRUNCATE, "%s\\%s.json", dir, name);
        _snprintf_s(tmp, _TRUNCATE, "%s\\%s.json.tmp", dir, name);
        FILE* f = nullptr;
        if (fopen_s(&f, tmp, "w") != 0 || !f) { strncpy_s(s_cfgStatus, "Falha ao salvar.", _TRUNCATE); return false; }
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
        JB(bZombieEsp); JI(iZombieBox); JB(bZombieName); JB(bZombieDist); JB(bZombieHp); JB(bZombiePct); JF(fPctX); JF(fPctY); JI(iNameA); JI(iDistA); JI(iHpA); JI(iPctA); JF(fPropN); JF(fPropD); JF(fPropH); JF(fPropP);
        JI(iSideN); JI(iSideD); JI(iSideH); JI(iSideP); JI(iAlinN); JI(iAlinD); JI(iAlinH); JI(iAlinP);
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
        JF(fNoClipSpeed); JB(bNoFall); JI(iCfgVer);
#undef JB
#undef JI
#undef JF
#undef JV
#undef JS
        fprintf(f, "\"_v\":3\n}\n");
        fclose(f);
        // A12: publicacao atomica (tmp + rename); sem .bak espalhado no sucesso.
        if (!MoveFileExA(tmp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            _snprintf_s(s_cfgStatus, _TRUNCATE, "Falha ao publicar (tmp mantido).");
            return false;
        }
        _snprintf_s(s_cfgStatus, _TRUNCATE, "Salvo: %s.json", name);
        Log::Infof("Config salva: %s", path);
        return true;
    }

    static const char* CfgFind(const std::string& s, const char* k) {
        std::string q = std::string("\"") + k + "\"";
        size_t p = s.find(q);
        if (p == std::string::npos) return nullptr;
        p = s.find(':', p);
        if (p == std::string::npos) return nullptr;
        return s.c_str() + p + 1;
    }

    // A13: valida o modelo de layout apos carregar/migrar/resetar.
    static void ValidateLayout() {
        using namespace Config;
        int* sides[4] = { &iSideN, &iSideD, &iSideH, &iSideP };
        int* als[4] = { &iAlinN, &iAlinD, &iAlinH, &iAlinP };
        float* nxs[4] = { &fNameX, &fDistX, &fHpX, &fPctX };
        float* nys[4] = { &fNameY, &fDistY, &fHpY, &fPctY };
        float* prs[4] = { &fPropN, &fPropD, &fPropH, &fPropP };
        const int ds[4] = { 0, 1, 2, 3 }, da[4] = { 1, 1, 2, 1 };
        for (int i = 0; i < 4; ++i) {
            if (*sides[i] < 0 || *sides[i] > 3) *sides[i] = ds[i];
            if (*als[i] < 0 || *als[i] > 2) *als[i] = da[i];
            if (!(*nxs[i] == *nxs[i]) || *nxs[i] < -300 || *nxs[i] > 300) *nxs[i] = 0;
            if (!(*nys[i] == *nys[i]) || *nys[i] < -300 || *nys[i] > 300) *nys[i] = 0;
            if (!(*prs[i] == *prs[i]) || *prs[i] < 0 || *prs[i] > 1) *prs[i] = 0.05f;
        }
        if (!(fSnapSize == fSnapSize) || fSnapSize < 1 || fSnapSize > 20) fSnapSize = 5;
        if (!(fPreviewHp == fPreviewHp) || fPreviewHp < 0 || fPreviewHp > 100) fPreviewHp = 87;
        if (iLayoutMode < 0 || iLayoutMode > 2) iLayoutMode = 0;
    }

    // Migracao ancora(9) -> lado+alinhamento (v2 -> v3). Cantos: lado vence.
    static void MigrateAnchors() {
        using namespace Config;
        int* sides[4] = { &iSideN, &iSideD, &iSideH, &iSideP };
        int* als[4] = { &iAlinN, &iAlinD, &iAlinH, &iAlinP };
        int olds[4] = { iNameA, iDistA, iHpA, iPctA };
        for (int i = 0; i < 4; ++i) {
            int a = olds[i];
            if (a < 0 || a > 8) continue;
            int row = a / 3, col = a % 3;
            // TL/TR/BL/BR: lado pela fileira/coluna dominante (doc: lado vence empates).
            if (a == 0) { *sides[i] = 0; *als[i] = 0; }
            else if (a == 2) { *sides[i] = 0; *als[i] = 2; }
            else if (a == 6) { *sides[i] = 1; *als[i] = 0; }
            else if (a == 8) { *sides[i] = 1; *als[i] = 2; }
            else if (row == 0) { *sides[i] = 0; *als[i] = col; }
            else if (row == 2) { *sides[i] = 1; *als[i] = col; }
            else { *sides[i] = (col == 2) ? 3 : 2; *als[i] = 1; }
            (void)row; (void)col;
        }
    }

    static void LoadConfig(const char* name) {
        using namespace Config;
        char dir[MAX_PATH] = { 0 }, path[MAX_PATH] = { 0 }, bak[MAX_PATH] = { 0 };
        CfgDir(dir, sizeof(dir));
        if (!dir[0]) { strncpy_s(s_cfgStatus, "Sem pasta de documentos.", _TRUNCATE); return; }
        _snprintf_s(path, _TRUNCATE, "%s\\%s.json", dir, name);
        _snprintf_s(bak, _TRUNCATE, "%s\\%s.json.bak", dir, name);
        FILE* f = nullptr;
        if (fopen_s(&f, path, "r") != 0 || !f) { ApplyFactoryDefaults(); SaveConfig(name); _snprintf_s(s_cfgStatus, _TRUNCATE, "Preset novo (fabrica v3): %s.", name); return; }
        std::string s;
        char chunk[1024];
        size_t n;
        while ((n = fread(chunk, 1, sizeof(chunk), f)) > 0) s.append(chunk, n);
        fclose(f);
        { // A12: schema < v3 -> backup unico + parse legado + fabrica de layout + salva.
            const char* w = CfgFind(s, "_v");
            int ver = w ? atoi(w) : 0;
            if (ver < 3) {
                Log::Infof("[CONFIG] Legado v%d: backup + migracao p/ v3.", ver);
                CopyFileA(path, bak, FALSE);
                iCfgVer = ver; // marca: pos-parse migra
            } else iCfgVer = ver;
        }
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
        LB(bZombieEsp); LI(iZombieBox); LB(bZombieName); LB(bZombieDist); LB(bZombieHp); LB(bZombiePct); LF(fPctX); LF(fPctY); LI(iNameA); LI(iDistA); LI(iHpA); LI(iPctA); LF(fPropN); LF(fPropD); LF(fPropH); LF(fPropP);
        LI(iSideN); LI(iSideD); LI(iSideH); LI(iSideP); LI(iAlinN); LI(iAlinD); LI(iAlinH); LI(iAlinP);
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
        LF(fNoClipSpeed); LB(bNoFall); LI(iCfgVer);
#undef LB
#undef LI
#undef LF
#undef LV
#undef LS
        if (iCfgVer < 3) { // pos-parse: v2 tem ancoras (migra), v0/v1 nao (fabrica).
            const char* w = CfgFind(s, "iNameA");
            if (w) MigrateAnchors();
            else ApplyFactoryDefaults();
            iCfgVer = 3;
            ValidateLayout();
            SaveConfig(name);
            _snprintf_s(s_cfgStatus, _TRUNCATE, "Migrado p/ v3 (backup .bak).");
        } else {
            ValidateLayout();
            _snprintf_s(s_cfgStatus, _TRUNCATE, "Carregado: %s.json", name);
        }
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
                float bw2 = r1.x - r0.x, bh2 = r1.y - r0.y; // ref p/ gap proporcional
                if (Config::bZombieDist) {
                    char db[32]; _snprintf_s(db, _TRUNCATE, "%.0fm", (double)es[i].dist);
                    ImU32 dcol = ImGui::GetColorU32(ImVec4(Config::colZombieDistVis[0], Config::colZombieDistVis[1], Config::colZombieDistVis[2], Config::colZombieDistVis[3]));
                    ImVec2 dsz = ImGui::CalcTextSize(db);
                    ImVec2 dp = PlaceEl(Config::iSideD, Config::iAlinD, dsz.x, dsz.y, ElGap(Config::iSideD, Config::fPropD, bw2, bh2), r0, r1);
                    dl->AddText(ImVec2(dp.x + Config::fDistX, dp.y + Config::fDistY), dcol, db);
                }
                if (Config::bZombieHp && es[i].maxHp > 0) {
                    // Barra pelo LADO (esq/dir = vertical 5xH; topo/base = horizontal Wx5).
                    float pct = es[i].hp / es[i].maxHp;
                    if (pct < 0) pct = 0; if (pct > 1) pct = 1;
                    float hc[4]; HpColor(pct * 100.0f, hc);
                    ImU32 hfill = ImGui::GetColorU32(ImVec4(hc[0], hc[1], hc[2], hc[3]));
                    bool horiz = BarHorizSide(Config::iSideH);
                    float blen = horiz ? bw2 : bh2;
                    ImVec2 bp = PlaceEl(Config::iSideH, Config::iAlinH, horiz ? blen : 5.0f, horiz ? 5.0f : blen,
                        ElGap(Config::iSideH, Config::fPropH, bw2, bh2), r0, r1);
                    float bx = bp.x + Config::fHpX, by = bp.y + Config::fHpY;
                    if (horiz) {
                        dl->AddRectFilled(ImVec2(bx, by), ImVec2(bx + blen, by + 5), IM_COL32(40, 40, 44, 255));
                        dl->AddRectFilled(ImVec2(bx, by), ImVec2(bx + blen * pct, by + 5), hfill);
                    } else {
                        dl->AddRectFilled(ImVec2(bx, by), ImVec2(bx + 5, by + blen), IM_COL32(40, 40, 44, 255));
                        dl->AddRectFilled(ImVec2(bx, by + blen * (1 - pct)), ImVec2(bx + 5, by + blen), hfill);
                    }
                }
                // Pilar 4: % independente da barra grafica.
                if (Config::bZombiePct && es[i].maxHp > 0) {
                    float pct2 = es[i].hp / es[i].maxHp;
                    if (pct2 < 0) pct2 = 0; if (pct2 > 1) pct2 = 1;
                    char pb[16]; _snprintf_s(pb, _TRUNCATE, "%.0f%%", (double)(pct2 * 100.0));
                    ImVec2 psz = ImGui::CalcTextSize(pb);
                    ImVec2 pp = PlaceEl(Config::iSideP, Config::iAlinP, psz.x, psz.y, ElGap(Config::iSideP, Config::fPropP, bw2, bh2), r0, r1);
                    dl->AddText(ImVec2(pp.x + Config::fPctX, pp.y + Config::fPctY), IM_COL32_WHITE, pb);
                }
                if (Config::bZombieName && es[i].name[0]) {
                    ImU32 ncol = ImGui::GetColorU32(ImVec4(Config::colZombieNameVis[0], Config::colZombieNameVis[1], Config::colZombieNameVis[2], Config::colZombieNameVis[3]));
                    ImVec2 nsz = ImGui::CalcTextSize(es[i].name);
                    ImVec2 np = PlaceEl(Config::iSideN, Config::iAlinN, nsz.x, nsz.y, ElGap(Config::iSideN, Config::fPropN, bw2, bh2), r0, r1);
                    dl->AddText(ImVec2(np.x + Config::fNameX, np.y + Config::fNameY), ncol, es[i].name);
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
            // Fecha gesto sem soltar o clamp: ele equivale ao clamp do jogo
            // (mesma janela/mesmo rect) e o toggle reaplica o do jogo.
            s_capName.active = s_capDist.active = s_capHp.active = s_capPct.active = false;
            s_dragOwner = nullptr;
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
            // A05: UM EndTabItem incondicional por BeginTabItem (transicao nao controla pareamento).
            if (ImGui::BeginTabItem("VISUAL")) {
                if (!g_bEditMode) {
                    if (ImGui::Button("Editar Layout do ESP")) {
                        SnapshotLayout();
                        g_bEditMode = true;
                    }
                    Tip("Abre o editor de layout (arrasto pelo cursor do Windows).");
                } else {
                    ImGui::Text("MODO EDICAO - arraste Nome, Dist, Vida, %% (lado+alinhamento auto)");
                    ImVec2 epv = ImGui::GetCursorScreenPos();
                    ImVec2 epsz = ImVec2(ImGui::GetContentRegionAvail().x, 420);
                    ImGui::InvisibleButton("pv_edit_zone", epsz);
                    DrawEspPreview(epv, epsz);
                    if (ImGui::Button("Salvar Layout")) {
                        if (SaveConfig(s_cfgName)) g_bEditMode = false;
                        else _snprintf_s(s_cfgStatus, _TRUNCATE, "FALHA ao salvar (rascunho mantido).");
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancelar")) {
                        RestoreLayout();
                        g_bEditMode = false;
                    }
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
                if (Config::iLayoutMode == 1 || Config::iLayoutMode == 2) ApplyPreset(Config::iLayoutMode);
                ImGui::SliderFloat("HP simulado", &Config::fPreviewHp, 0, 100, "%.0f%%");
                if (ImGui::Button("Resetar Posicoes")) {
                    ApplyFactoryDefaults();
                }
                ImGui::TextDisabled("Lados: Nome[%d/%d] Dist[%d/%d] Vida[%d/%d] %%[%d/%d] (lado/alin)",
                    Config::iSideN, Config::iAlinN, Config::iSideD, Config::iAlinD,
                    Config::iSideH, Config::iAlinH, Config::iSideP, Config::iAlinP);
                ImGui::SliderFloat("Prop Nome", &Config::fPropN, 0.0f, 0.2f, "%.2f"); Tip("Extra proporcional do Nome.");
                ImGui::SliderFloat("Prop Dist", &Config::fPropD, 0.0f, 0.2f, "%.2f");
                ImGui::SliderFloat("Prop Vida", &Config::fPropH, 0.0f, 0.2f, "%.2f");
                ImGui::SliderFloat("Prop %%", &Config::fPropP, 0.0f, 0.2f, "%.2f");
                ImGui::Checkbox("Snap to Grid", &Config::bSnapGrid);
                ImGui::SliderFloat("Grid", &Config::fSnapSize, 1, 20, "%.0fpx");
                ImGui::Checkbox("Guias", &Config::bShowGuides);
                } // fim conteudo normal (modo edicao mostra so o preview)
                ImGui::EndTabItem(); // A05: unico e incondicional
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

















