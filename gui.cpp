#include "esp_layout.h"
#include "config_json.h"
#include <cmath>
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

    bool  bGodMode = false;      // default OFF (escrita direta, ver mono.cpp)
    bool  bInfStamina = false;   // default OFF
    bool  bInfItems = false;     // default OFF (trava pilhas por SubType)
    bool  bInfMoney = false;     // default OFF (3 moedas 99999)
    // REMOVIDO 17/09: bAntiFlood (guardiao bania o proprio infinito).
    bool  bUnlockSlots = false;  // default OFF (desbloqueia 1x)
    bool  bUnlockLoadout = false; // default OFF (vendedor desbloqueado)
    // REMOVIDO 17/09: Spawn de Itens (bGiveItem/iGiveItem/iGiveQty/iGiveDest).

    bool  bAimbot = false;
    int   iAimKey = VK_RBUTTON;
    int   iAimMode = 0;
    bool  bAutoAim = false;
    bool  bSilentAim = false;
    bool  bAutoFire = false;
    bool  bTriggerbot = false;
    int   iAimBone = 0;
    int   iAimPriority = 0;
    float fSmoothing = 1.0f;
    bool  bLimitFov = true;
    float fFovAngle = 90.0f;
    bool  b360Mode = false;
    bool  bDrawFov = true;
    float fEspDistance = 200.0f;
    float fAimDistance = 120.0f;
    bool  bPrediction = false;
    float fLagComp = 50.0f;

    bool  bNoRecoil = false;
    bool  bNoSpread = false;
    bool  bNoSway = false;
    bool  bTightAim = false;
    bool  bRapidFire = false;
    float fRapidMult = 2.0f;
    bool  bFastKnife = false;
    float fKnifeMult = 2.0f;
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
    bool  bRollSpeed = false;
    float fRollMult = 1.5f;

    bool  bZombieEsp = false;
    bool  bZombieBoxShow = true;
    int   iZombieBox = 0;
    bool  bZombieName = true;
    bool  bZombieDist = true;
    bool  bZombieHp = true;
    bool  bZombiePct = true;
    float fPctX = -38.0f, fPctY = -16.0f;
    bool  bZombieSkeleton = false;
    bool  bZombieSnap = false;
    bool  bZombieHeadDot = false;
    int   iSnapFrom = 0;           // 0=Base 1=Topo 2=Centro
    float colZombieSkel[4] = { 0, 1, 1, 1 };
    float colZombieSnap[4] = { 0, 1, 0, 1 };
    float colZombieDot[4] = { 1, 1, 0, 1 };
    float colZombieBox[4] = { 1, 0, 0, 1 };
    bool  bBossColor = true;               // default ON (boss se destaca)
    float colBossBox[4] = { 1, 0.4f, 0, 1 }; // laranja (visivel no ceu/chao)
    float colZombieName[4] = { 1, 1, 1, 1 };
    float colZombieDist[4] = { 1, 1, 1, 1 };
    float colZombieHp[4] = { 0, 1, 0, 1 };

    bool  bAllyEsp = false;
    int   iAllyBox = 0;
    bool  bAllyName = true;
    bool  bAllyDist = true;
    bool  bAllyHp = true;
    bool  bAllySkeleton = false;
    bool  bAllySnap = false;
    bool  bAllyHeadDot = false;
    float colAllyVis[4] = { 0.2f, 0.5f, 1.0f, 1 };

    bool  bChams = false;
    float colChamsVis[4] = { 1, 0, 0, 1 };
    float colChamsInv[4] = { 1, 1, 0, 1 };

    bool  bItemEsp = false;
    bool  bItemWeapons = true;
    bool  bItemRare = true;
    bool  bItemAmmo = true;
    bool  bItemSupply = true;
    float colItem[4] = { 1, 0.85f, 0.2f, 1 };
    float colItemRare[4] = { 0.7f, 0.2f, 1, 1 };
    float colItemAmmo[4] = { 0.7f, 0.7f, 0.7f, 1 };
    float colItemSupply[4] = { 0.2f, 1, 0.4f, 1 };
    float fItemRadius = 150.0f;

    bool  bPoiEsp = true;
    bool  bPoiHeli = true;
    bool  bPoiBoss = true;
    bool  bPoiMission = true;
    bool  bPoiWave = true;
    bool  bPoiLootFix = true;
    bool  bPoiBench = true;
    bool  bPoiFire = true;
    bool  bPoiShop = true;
    bool  bPoiRespawn = true;
    float colPoiHeli[4]     = { 0, 0.8f, 1, 1 };
    float colPoiBoss[4]     = { 1, 0.2f, 0.2f, 1 };
    float colPoiMission[4]  = { 1, 1, 0, 1 };
    float colPoiWave[4]     = { 1, 0.5f, 0, 1 };
    float colPoiLootFix[4]  = { 0.6f, 0.9f, 0.3f, 1 };
    float colPoiBench[4]    = { 0.8f, 0.6f, 0.2f, 1 };
    float colPoiFire[4]     = { 1, 0.4f, 0, 1 };
    float colPoiShop[4]     = { 0, 1, 0.6f, 1 };
    float colPoiRespawn[4]  = { 0.5f, 0.5f, 1, 1 };

    float fNameX = 0.0f, fNameY = -16.0f;
    float fDistX = 0.0f, fDistY = 4.0f;
    float fHpX   = -6.0f, fHpY = 0.0f;   // ZERO ABSOLUTO: topo da barra alinhado com o topo do box!
    int   iNameA = 1, iDistA = 7, iHpA = 0, iPctA = 5; // legado v2 (migracao)
    int   iSideN = 0, iSideD = 1, iSideH = 2, iSideP = 3; // topo, base, esq, dir
    int   iAlinN = 1, iAlinD = 1, iAlinH = 1, iAlinP = 1; // centro
    int   iCfgVer = 4;
    float fPropN = 0.05f, fPropD = 0.05f, fPropH = 0.06f, fPropP = 0.06f; // Pilar 2
    int   iLayoutMode = 0;
    int   iLayoutSide = 0;
    float fLayoutOffset = 4.0f;
    float fLayoutSpacing = 3.0f;
    bool  bSnapGrid = true;
    float fSnapSize = 1.0f;
    bool  bShowGuides = true;
    bool  bAlignList = true;
    int   iListDir = 0;
    float fListSpacing = 2.0f;
    float fPreviewHp = 87.0f;
    // FIX nome afastado: defaults de fabrica do layout de fabrica (preset 1).
    // Nome no TOPO centro (0.5), Dist na BASE centro (0.5), gaps zerados —
    // mesma regra p/ todos os textos (Lado + Posicao + gap base).
    float fAlongN = 0.5f, fAlongD = 0.5f, fAlongH = 0.5f, fAlongP = 0.5f;
    float fGapN = 0.0f, fGapD = 0.0f, fGapH = 0.0f, fGapP = 0.0f;
    int iOrderN = 0, iOrderD = 2, iOrderH = 0, iOrderP = 1;
    float fBarLength = 1.0f, fBarThickness = 3.0f;

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
    static char s_cfgName[64] = "default";
    static char s_cfgStatus[128] = { 0 };
    static EspLayout::Editor s_editor;
    static bool s_editorReady = false;
    static bool s_editorVisible = false;
    static float s_previewBoxHeight = 112.0f;
    static char s_previewName[96] = "Zombie";
    static float s_previewDistance = 34.0f;
    static EspLayout::Rect s_previewViewport;
    static EspLayout::Rect s_previewBox;
    static EspLayout::Result s_previewResult;
    static void LoadConfig(const char* name);
    static bool SaveConfig(const char* name);


    static bool ValidConfigPath(const char* directory, const char* name) {
        if (!directory || !directory[0] || !name || !name[0]) return false;
        if (std::strlen(name) >= sizeof(s_cfgName) || std::strlen(directory) + std::strlen(name) + 32 >= MAX_PATH) return false;
        for (const unsigned char* p = reinterpret_cast<const unsigned char*>(name); *p; ++p) {
            if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_' || *p == '-')) return false;
        }
        return true;
    }


    static EspLayout::Model ReadLayout() {
        using namespace Config;
        EspLayout::Model model;
        const int sides[4] = { iSideN, iSideD, iSideH, iSideP };
        const float along[4] = { fAlongN, fAlongD, fAlongH, fAlongP };
        const float gaps[4] = { fGapN, fGapD, fGapH, fGapP };
        const int order[4] = { iOrderN, iOrderD, iOrderH, iOrderP };
        const bool enabled[4] = { bZombieName, bZombieDist, bZombieHp, bZombiePct };
        for (int i = 0; i < 4; ++i) model.items[i] = { sides[i], along[i], gaps[i], order[i], enabled[i] };
        model.gap = fLayoutOffset;
        model.spacing = fLayoutSpacing;
        model.barLength = fBarLength;
        model.barThickness = fBarThickness;
        model.grid = bSnapGrid;
        model.gridSize = fSnapSize;
        model.stack = bAlignList;
        model.horizontalText = iListDir == 1;
        model.guides = bShowGuides;
        model.skeleton = bZombieSkeleton;
        model.headDot = bZombieHeadDot;
        model.snapline = bZombieSnap;
        model.boxStyle = iZombieBox;
        model.preset = iLayoutMode;
        EspLayout::Normalize(model);
        return model;
    }


    static void WriteLayout(EspLayout::Model model) {
        using namespace Config;
        EspLayout::Normalize(model);
        int* sides[4] = { &iSideN, &iSideD, &iSideH, &iSideP };
        int* align[4] = { &iAlinN, &iAlinD, &iAlinH, &iAlinP };
        float* along[4] = { &fAlongN, &fAlongD, &fAlongH, &fAlongP };
        float* gaps[4] = { &fGapN, &fGapD, &fGapH, &fGapP };
        int* order[4] = { &iOrderN, &iOrderD, &iOrderH, &iOrderP };
        bool* enabled[4] = { &bZombieName, &bZombieDist, &bZombieHp, &bZombiePct };
        for (int i = 0; i < 4; ++i) {
            *sides[i] = model.items[i].side;
            *along[i] = model.items[i].position;
            *align[i] = static_cast<int>(model.items[i].position * 2.0f + 0.5f);
            *gaps[i] = model.items[i].extraGap;
            *order[i] = model.items[i].order;
            *enabled[i] = model.items[i].enabled;
        }
        fLayoutOffset = model.gap;
        fLayoutSpacing = model.spacing;
        fBarLength = model.barLength;
        fBarThickness = model.barThickness;
        bSnapGrid = model.grid;
        fSnapSize = model.gridSize;
        bAlignList = model.stack;
        iListDir = model.horizontalText ? 1 : 0;
        bShowGuides = model.guides;
        bZombieSkeleton = model.skeleton;
        bZombieHeadDot = model.headDot;
        bZombieSnap = model.snapline;
        iZombieBox = model.boxStyle;
        iLayoutMode = model.preset;
        fNameX = fNameY = fDistX = fDistY = fHpX = fHpY = fPctX = fPctY = 0.0f;
        fPropN = fPropD = fPropH = fPropP = 0.0f;
        iCfgVer = 4;
    }


    static void ApplyFactoryDefaults() {
        WriteLayout(EspLayout::Preset(1));
    }


    static void CloseLayoutEditor() {
        EspLayout::CancelDrag(s_editor);
        s_editorReady = false;
        s_editorVisible = false;
    }


    static bool SaveLayoutDraft() {
        EspLayout::CancelDrag(s_editor);
        EspLayout::Model before = ReadLayout();
        WriteLayout(s_editor.draft);
        if (SaveConfig(s_cfgName)) { s_editor.dirty = false; return true; }
        WriteLayout(before);
        return false;
    }


    static EspLayout::Model VisibleLayout() {
        if (s_editorVisible && s_editorReady) {
            if (s_editor.active >= 0 && s_editor.moved && s_editor.candidateValid) return s_editor.candidate;
            return s_editor.draft;
        }
        return ReadLayout();
    }


    static EspLayout::Content LayoutContent(const char* name, float distance, float hp, float maxHp) {
        EspLayout::Content content;
        char buffer[64];
        content.text[EspLayout::Name] = name ? name : "";
        _snprintf_s(buffer, _TRUNCATE, "%.0fm", static_cast<double>(EspLayout::Clamp(distance, 0.0f, 999999.0f, 0.0f)));
        content.text[EspLayout::Distance] = buffer;
        content.healthAvailable = std::isfinite(maxHp) && maxHp > 0.0f && std::isfinite(hp);
        content.health = content.healthAvailable ? EspLayout::Clamp(hp / maxHp, 0.0f, 1.0f, 0.0f) : 0.0f;
        _snprintf_s(buffer, _TRUNCATE, "%.0f%%", static_cast<double>(content.health * 100.0f));
        content.text[EspLayout::Percent] = buffer;
        return content;
    }


    static EspLayout::Style LayoutStyle(float health) {
        EspLayout::Style style;
        style.font = ImGui::GetFont();
        style.fontSize = style.font->FontSize * ImGui::GetIO().FontGlobalScale;
        float(*nc)[4] = &Config::colZombieName;
        float(*dc)[4] = &Config::colZombieDist;
        float(*bc)[4] = &Config::colZombieBox;
        style.color[EspLayout::Name] = ImGui::GetColorU32(ImVec4((*nc)[0], (*nc)[1], (*nc)[2], (*nc)[3]));
        style.color[EspLayout::Distance] = ImGui::GetColorU32(ImVec4((*dc)[0], (*dc)[1], (*dc)[2], (*dc)[3]));
        style.boxColor = ImGui::GetColorU32(ImVec4((*bc)[0], (*bc)[1], (*bc)[2], (*bc)[3]));
        float color[4];
        HpColor(health * 100.0f, color);
        style.color[EspLayout::Health] = ImGui::GetColorU32(ImVec4(color[0], color[1], color[2], color[3]));
        return style;
    }


    void DrawEspPreview(ImVec2 origin, ImVec2 size) {
        if (!s_editorReady) { EspLayout::Reset(s_editor, ReadLayout()); s_editorReady = true; }
        // Preview espelha os toggles reais (menu manda, canvas mostra):
        // usuario ve como fica Skeleton/HeadDot/Snapline sem duplicar controle.
        s_editor.draft.skeleton = Config::bZombieSkeleton;
        s_editor.draft.headDot = Config::bZombieHeadDot;
        s_editor.draft.snapline = Config::bZombieSnap;
        s_editor.draft.boxStyle = Config::iZombieBox;
        float h = s_previewBoxHeight;
        ImVec2 center(origin.x + size.x * 0.5f, origin.y + size.y * 0.5f);
        s_previewViewport = { ImVec2(origin.x + 5.0f, origin.y + 5.0f), ImVec2(origin.x + size.x - 5.0f, origin.y + size.y - 5.0f) };
        s_previewBox = { ImVec2(center.x - h * 0.3f, center.y - h * 0.5f), ImVec2(center.x + h * 0.3f, center.y + h * 0.5f) };
        auto content = LayoutContent(s_previewName, s_previewDistance, Config::fPreviewHp, 100.0f);
        auto style = LayoutStyle(content.health);
        // Preview usa as cores reais do jogo (1:1 com RenderOverlay):
        // headDot faltava no canvas (skeleton/linha ja iam); sem isso o ponto
        // nunca aparecia no preview mesmo com o toggle ligado.
        style.skelColor = ImGui::GetColorU32(ImVec4(Config::colZombieSkel[0], Config::colZombieSkel[1], Config::colZombieSkel[2], Config::colZombieSkel[3]));
        style.snapColor = ImGui::GetColorU32(ImVec4(Config::colZombieSnap[0], Config::colZombieSnap[1], Config::colZombieSnap[2], Config::colZombieSnap[3]));
        style.dotColor  = ImGui::GetColorU32(ImVec4(Config::colZombieDot[0],  Config::colZombieDot[1],  Config::colZombieDot[2],  Config::colZombieDot[3]));
        EspLayout::Input input;
        input.mouse = ImGui::GetIO().MousePos;
        input.pressed = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        input.down = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        input.released = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
        input.hovered = ImGui::IsItemHovered();
        input.cancel = ImGui::GetIO().AppFocusLost || ImGui::IsKeyPressed(ImGuiKey_Escape, false);
        s_previewResult = EspLayout::Update(s_editor, s_previewBox, s_previewViewport, content, style, input);
        ImDrawList* draw = ImGui::GetWindowDrawList();
        EspLayout::Model model = s_editor.active >= 0 && s_editor.moved && s_editor.candidateValid ? s_editor.candidate : s_editor.draft;
        EspLayout::DrawPreview(draw, { origin, ImVec2(origin.x + size.x, origin.y + size.y) }, s_previewResult, model, style, s_editor.selected, s_editor.active >= 0, s_editor.candidateValid);
        draw->PushClipRect(s_previewViewport.min, s_previewViewport.max, true);
        EspLayout::Draw(draw, s_previewResult, content, style);
        draw->PopClipRect();
        if (input.hovered && s_editor.active < 0) {
            int hit = EspLayout::HitTest(s_previewResult, input.mouse, s_editor.selected);
            if (hit >= 0) ImGui::SetTooltip("%s: arraste para uma das quatro bordas.", EspLayout::ElementName(hit));
        }
    }


    static void DrawLayoutEditor() {
        s_editorVisible = true;
        if (!s_editorReady) { EspLayout::Reset(s_editor, ReadLayout()); s_editorReady = true; }
        auto& model = s_editor.draft;
        // FIX menu: sem Salvar/Cancelar — tudo aplica na hora (spec VISUAL-PRO).
        // Preset/layout escreve direto via SaveLayoutDraft silencioso.
        static const char* presets[] = { "Personalizado", "Classico esquerda", "Classico direita", "Vida acima", "Vida abaixo", "Tudo a esquerda", "Tudo a direita", "Tudo acima", "Tudo abaixo", "Cantos esquerda", "Cantos direita" };
        int preset = model.preset;
        ImGui::TextDisabled("Preset do layout (aplica na hora):");
        ImGui::SetNextItemWidth(220.0f);
        if (ImGui::Combo("Layout", &preset, presets, 11)) {
            EspLayout::CancelDrag(s_editor);
            if (preset > 0) {
                EspLayout::Model replacement = EspLayout::Preset(preset);
                for (int i = 0; i < EspLayout::Count; ++i) replacement.items[i].enabled = model.items[i].enabled;
                replacement.guides = model.guides;
                replacement.boxStyle = model.boxStyle;
                replacement.skeleton = model.skeleton;
                replacement.headDot = model.headDot;
                replacement.snapline = model.snapline;
                model = replacement;
            } else model.preset = 0;
            SaveLayoutDraft();
        }
        if (s_cfgStatus[0]) ImGui::TextWrapped("%s", s_cfgStatus);

        float maxExtra = 0.0f;
        for (const auto& item : model.items) if (item.extraGap > maxExtra) maxExtra = item.extraGap;
        float reserve = model.gap + maxExtra + model.barThickness + model.spacing * 3.0f + LayoutStyle(1.0f).fontSize * 3.0f + 16.0f;
        float canvasHeight = s_previewBoxHeight + reserve * 2.0f;
        if (canvasHeight < 260.0f) canvasHeight = 260.0f;
        float available = ImGui::GetContentRegionAvail().x;
        bool columns = available >= 730.0f && ImGui::BeginTable("layout_columns", 2, ImGuiTableFlags_SizingFixedFit);
        if (columns) { ImGui::TableSetupColumn("preview", ImGuiTableColumnFlags_WidthFixed, 420.0f); ImGui::TableSetupColumn("inspector", ImGuiTableColumnFlags_WidthStretch); ImGui::TableNextColumn(); }
        ImVec2 size(available < 420.0f ? available : 420.0f, canvasHeight);
        ImVec2 origin = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("esp_preview_canvas", size);
        DrawEspPreview(origin, size);
        ImGui::TextDisabled("Arraste ao redor do box. Esc cancela o arraste.");
        if (s_editor.active >= 0 && s_editor.moved && !s_editor.candidateValid) ImGui::TextColored(ImVec4(1, 0.6f, 0.4f, 1), "Destino invalido: solte para voltar.");
        if (!s_previewResult.valid) ImGui::TextWrapped("Amplie a janela ou reduza o tamanho do box para editar este layout.");
        if (columns) ImGui::TableNextColumn();
        EspLayout::Model beforeControls = model;
        bool changed = false;
        ImGui::Text("ELEMENTO");
        static const char* elements[] = { "Nome", "Distancia", "Barra de vida", "Porcentagem" };
        ImGui::SetNextItemWidth(190.0f);
        ImGui::Combo("Elemento", &s_editor.selected, elements, 4);
        auto& item = model.items[s_editor.selected];
        changed |= ImGui::Checkbox("Exibir", &item.enabled);
        static const char* sides[] = { "Acima", "Abaixo", "Esquerda", "Direita" };
        ImGui::SetNextItemWidth(160.0f);
        changed |= ImGui::Combo("Lado", &item.side, sides, 4);
        // Posicao como escolha discreta (Inicio/Centro/Fim) em vez de slider
        // 0-1: usuario quer centralizar/canto, nao digitar 0.37.
        {
            int posChoice = item.position < 0.25f ? 0 : (item.position > 0.75f ? 2 : 1);
            static const char* posNames[] = { "Inicio", "Centro", "Fim" };
            ImGui::SetNextItemWidth(160.0f);
            int posNew = posChoice;
            if (ImGui::Combo("Posicao", &posNew, posNames, 3)) {
                float want = posNew == 0 ? 0.0f : (posNew == 1 ? 0.5f : 1.0f);
                if (item.position != want) { item.position = want; changed = true; }
            }
        }
        if (s_editor.selected == EspLayout::Health) {
            ImGui::SetNextItemWidth(160.0f);
            changed |= ImGui::SliderFloat("Comprimento", &model.barLength, 0.25f, 1.0f, "%.2f");
            ImGui::SetNextItemWidth(160.0f);
            changed |= ImGui::SliderFloat("Espessura", &model.barThickness, 2.0f, 6.0f, "%.0fpx");
        }
        ImGui::SetNextItemWidth(160.0f);
        ImGui::SliderFloat("Altura", &s_previewBoxHeight, 32.0f, 180.0f, "%.0fpx");
        ImGui::SetNextItemWidth(160.0f);
        if (ImGui::Combo("Tipo de box", &model.boxStyle, kBoxType, 3)) SaveLayoutDraft();
        if (changed) {
            EspLayout::CancelDrag(s_editor);
            EspLayout::Normalize(model);
            auto content = LayoutContent(s_previewName, s_previewDistance, Config::fPreviewHp, 100.0f);
            auto candidate = EspLayout::Resolve(model, s_previewBox, s_previewViewport, content, LayoutStyle(content.health));
            if (candidate.valid) { model.preset = 0; SaveLayoutDraft(); }
            else { model = beforeControls; _snprintf_s(s_cfgStatus, _TRUNCATE, "Sem espaco. Amplie o canvas ou reduza o afastamento."); }
        }
        if (columns) ImGui::EndTable();
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
        LoadConfig("default");
        g_bInit = true;
    }

    void Shutdown() {
        if (!g_bInit) return;
        CloseLayoutEditor();
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

    // Cabecalho de secao padrao mercado: texto acento colorido, sem linha.
    // (SeparatorText risca a aba inteira e polui com 2 colunas.)
    static void Section(const char* label) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.45f, 0.75f, 1.0f, 1.0f), "%s", label);
    }

    // Swatch alinhado a direita da coluna atual. So apresentacao.
    // Comeca na mesma linha do widget anterior (SameLine interno).
    static void SwatchR(const char* id, float col[4], const char* tip = nullptr) {
        ImGui::SameLine();
        float avail = ImGui::GetContentRegionAvail().x;
        if (avail > 30.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail - 26.0f);
        ImGui::ColorEdit4(id, col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
        if (tip) Tip(tip);
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

    static void CfgDir(char* out, size_t cap) {
        out[0] = 0;
        const char* app = Log::GetDir();
        if (app && app[0]) _snprintf_s(out, cap, _TRUNCATE, "%s\\configs", app);
    }

    static bool SaveConfig(const char* name) {
        using namespace Config;
        iCfgVer = 4;
        char dir[MAX_PATH] = { 0 }, path[MAX_PATH] = { 0 }, tmp[MAX_PATH] = { 0 };
        CfgDir(dir, sizeof(dir));
        if (!ValidConfigPath(dir, name)) { strncpy_s(s_cfgStatus, "Nome invalido ou caminho longo. Use letras, numeros, _ e -.", _TRUNCATE); return false; }
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
#define JS(v) fprintf(f, "\"" #v "\":\"%s\",\n", ConfigJson::Escape(v).c_str())
        JB(bMenuOpen); JI(iMenuKey); JB(bWatermark); JB(bDebugOverlay); JB(bTooltips);
        JB(bGodMode); JB(bInfStamina); JB(bInfItems); JB(bInfMoney); JB(bUnlockSlots); JB(bUnlockLoadout);
        JB(bAimbot); JI(iAimKey); JI(iAimMode); JB(bAutoAim); JB(bSilentAim); JB(bAutoFire);
        JB(bTriggerbot); JI(iAimBone); JI(iAimPriority); JF(fSmoothing);
        JB(bLimitFov); JF(fFovAngle); JB(b360Mode); JB(bDrawFov); JF(fEspDistance); JF(fAimDistance);
        JB(bPrediction); JF(fLagComp);
        JB(bNoRecoil); JB(bNoSpread); JB(bNoSway); JB(bTightAim); JB(bRapidFire); JF(fRapidMult); JB(bFastKnife); JF(fKnifeMult);
        JB(bInfAmmo); JB(bInstantReload); JB(bFullAuto); JB(bSaitama); JF(fNadeTime);
        JF(fExplRadius); JF(fExplDamage); JB(bContactExpl); JB(bPowerDrop);
        JS(szItemSearch); JI(iItemAmount);
        JB(bSpeedHack); JF(fSpeedMult); JB(bSuperJump); JF(fJumpMult);
        JB(bRollSpeed); JF(fRollMult);
        JB(bZombieEsp); JB(bZombieBoxShow); JI(iZombieBox); JB(bZombieName); JB(bZombieDist); JB(bZombieHp); JB(bZombiePct); JF(fPctX); JF(fPctY); JI(iNameA); JI(iDistA); JI(iHpA); JI(iPctA); JF(fPropN); JF(fPropD); JF(fPropH); JF(fPropP);
        JI(iSideN); JI(iSideD); JI(iSideH); JI(iSideP); JI(iAlinN); JI(iAlinD); JI(iAlinH); JI(iAlinP);
        JF(fAlongN); JF(fAlongD); JF(fAlongH); JF(fAlongP); JF(fGapN); JF(fGapD); JF(fGapH); JF(fGapP);
        JI(iOrderN); JI(iOrderD); JI(iOrderH); JI(iOrderP); JF(fBarLength); JF(fBarThickness);
        JB(bZombieSkeleton); JB(bZombieSnap); JB(bZombieHeadDot); JV(colZombieSkel); JV(colZombieSnap); JV(colZombieDot);
        JI(iSnapFrom);
        JV(colZombieBox); JV(colZombieName); JV(colZombieDist); JV(colZombieHp); JB(bBossColor); JV(colBossBox);
        JB(bAllyEsp); JI(iAllyBox); JB(bAllyName); JB(bAllyDist); JB(bAllyHp);
        JB(bAllySkeleton); JB(bAllySnap); JB(bAllyHeadDot); JV(colAllyVis);
        JB(bChams); JV(colChamsVis); JV(colChamsInv); JB(bItemEsp); JB(bItemWeapons);
        JB(bItemRare); JB(bItemAmmo); JB(bItemSupply); JV(colItem); JV(colItemRare); JV(colItemAmmo); JV(colItemSupply); JF(fItemRadius);
        JB(bPoiEsp); JB(bPoiHeli); JB(bPoiBoss); JB(bPoiMission); JB(bPoiWave); JB(bPoiLootFix); JB(bPoiBench); JB(bPoiFire); JB(bPoiShop); JB(bPoiRespawn);
        JV(colPoiHeli); JV(colPoiBoss); JV(colPoiMission); JV(colPoiWave); JV(colPoiLootFix); JV(colPoiBench); JV(colPoiFire); JV(colPoiShop); JV(colPoiRespawn);
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
        fprintf(f, "\"_v\":4\n}\n");
        bool writeOk = fflush(f) == 0 && ferror(f) == 0;
        if (fclose(f) != 0) writeOk = false;
        if (!writeOk) {
            _snprintf_s(s_cfgStatus, _TRUNCATE, "Falha na gravacao. Configuracao anterior preservada.");
            return false;
        }
        if (!MoveFileExA(tmp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            _snprintf_s(s_cfgStatus, _TRUNCATE, "Falha ao publicar (tmp mantido).");
            return false;
        }
        _snprintf_s(s_cfgStatus, _TRUNCATE, "Salvo: %s.json", name);
        Log::Infof("Config salva: %s", path);
        return true;
    }

    static void LoadConfig(const char* name) {
        using namespace Config;
        char dir[MAX_PATH] = { 0 }, path[MAX_PATH] = { 0 };
        CfgDir(dir, sizeof(dir));
        if (!ValidConfigPath(dir, name)) { strncpy_s(s_cfgStatus, "Nome invalido ou caminho longo. Use letras, numeros, _ e -.", _TRUNCATE); return; }
        _snprintf_s(path, _TRUNCATE, "%s\\%s.json", dir, name);
        FILE* file = nullptr;
        if (fopen_s(&file, path, "rb") != 0 || !file) {
            if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) { strncpy_s(s_cfgStatus, "Falha ao ler o preset existente.", _TRUNCATE); return; }
            ApplyFactoryDefaults();
            SaveConfig(name);
            CloseLayoutEditor();
            return;
        }
        std::string input;
        char chunk[4096];
        size_t length;
        while ((length = fread(chunk, 1, sizeof(chunk), file)) > 0 && input.size() <= 2 * 1024 * 1024) input.append(chunk, length);
        bool readOk = ferror(file) == 0 && input.size() <= 2 * 1024 * 1024;
        fclose(file);
        ConfigJson::Document document;
        if (!readOk || !document.Parse(input) || document.Version() < 0 || document.Version() > 4) {
            strncpy_s(s_cfgStatus, "Preset invalido ou de uma versao futura. Estado anterior preservado.", _TRUNCATE);
            return;
        }
        int sourceVersion = document.Version();
        std::vector<ConfigJson::Field> fields;
#define LB(v) fields.push_back({ #v, ConfigJson::BoolField, &(v) })
#define LI(v) fields.push_back({ #v, ConfigJson::IntField, &(v) })
#define LF(v) fields.push_back({ #v, ConfigJson::FloatField, &(v) })
#define LV(v) fields.push_back({ #v, ConfigJson::ColorField, (v) })
#define LS(v) fields.push_back({ #v, ConfigJson::StringField, (v), sizeof(v) })
        LB(bMenuOpen); LI(iMenuKey); LB(bWatermark); LB(bDebugOverlay); LB(bTooltips);
        LB(bGodMode); LB(bInfStamina); LB(bInfItems); LB(bInfMoney); LB(bUnlockSlots); LB(bUnlockLoadout);
        LB(bAimbot); LI(iAimKey); LI(iAimMode); LB(bAutoAim); LB(bSilentAim); LB(bAutoFire);
        LB(bTriggerbot); LI(iAimBone); LI(iAimPriority); LF(fSmoothing);
        LB(bLimitFov); LF(fFovAngle); LB(b360Mode); LB(bDrawFov); LF(fEspDistance); LF(fAimDistance);
        LB(bPrediction); LF(fLagComp);
        LB(bNoRecoil); LB(bNoSpread); LB(bNoSway); LB(bTightAim); LB(bRapidFire); LF(fRapidMult); LB(bFastKnife); LF(fKnifeMult);
        LB(bInfAmmo); LB(bInstantReload); LB(bFullAuto); LB(bSaitama); LF(fNadeTime);
        LF(fExplRadius); LF(fExplDamage); LB(bContactExpl); LB(bPowerDrop);
        LS(szItemSearch); LI(iItemAmount);
        LB(bSpeedHack); LF(fSpeedMult); LB(bSuperJump); LF(fJumpMult);
        LB(bRollSpeed); LF(fRollMult);
        LB(bZombieEsp); LB(bZombieBoxShow); LI(iZombieBox); LB(bZombieName); LB(bZombieDist); LB(bZombieHp); LB(bZombiePct); LF(fPctX); LF(fPctY); LI(iNameA); LI(iDistA); LI(iHpA); LI(iPctA); LF(fPropN); LF(fPropD); LF(fPropH); LF(fPropP);
        LI(iSideN); LI(iSideD); LI(iSideH); LI(iSideP); LI(iAlinN); LI(iAlinD); LI(iAlinH); LI(iAlinP);
        LB(bZombieSkeleton); LB(bZombieSnap); LB(bZombieHeadDot); LV(colZombieSkel); LV(colZombieSnap); LV(colZombieDot);
        LI(iSnapFrom);
        LV(colZombieBox); LV(colZombieName); LV(colZombieDist); LV(colZombieHp); LB(bBossColor); LV(colBossBox);
        LB(bAllyEsp); LI(iAllyBox); LB(bAllyName); LB(bAllyDist); LB(bAllyHp);
        LB(bAllySkeleton); LB(bAllySnap); LB(bAllyHeadDot); LV(colAllyVis);
        LB(bChams); LV(colChamsVis); LV(colChamsInv); LB(bItemEsp); LB(bItemWeapons);
        LB(bItemRare); LB(bItemAmmo); LB(bItemSupply); LV(colItem); LV(colItemRare); LV(colItemAmmo); LV(colItemSupply); LF(fItemRadius);
        LB(bPoiEsp); LB(bPoiHeli); LB(bPoiBoss); LB(bPoiMission); LB(bPoiWave); LB(bPoiLootFix); LB(bPoiBench); LB(bPoiFire); LB(bPoiShop); LB(bPoiRespawn);
        LV(colPoiHeli); LV(colPoiBoss); LV(colPoiMission); LV(colPoiWave); LV(colPoiLootFix); LV(colPoiBench); LV(colPoiFire); LV(colPoiShop); LV(colPoiRespawn);
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
        LF(fAlongN); LF(fAlongD); LF(fAlongH); LF(fAlongP); LF(fGapN); LF(fGapD); LF(fGapH); LF(fGapP);
        LI(iOrderN); LI(iOrderD); LI(iOrderH); LI(iOrderP); LF(fBarLength); LF(fBarThickness);
#undef LB
#undef LI
#undef LF
#undef LV
#undef LS
        if (!document.Apply(fields)) {
            strncpy_s(s_cfgStatus, "Tipos invalidos no preset. Estado anterior preservado.", _TRUNCATE);
            return;
        }
        if (sourceVersion < 4) {
            EspLayout::Model model = ReadLayout();
            const int anchors[4] = { iNameA, iDistA, iHpA, iPctA };
            const int alignments[4] = { iAlinN, iAlinD, iAlinH, iAlinP };
            for (int id = 0; id < 4; ++id) {
                model.items[id].position = EspLayout::Clamp(alignments[id] * 0.5f, 0.0f, 1.0f, 0.5f);
                model.items[id].extraGap = 0.0f;
                if (sourceVersion < 3 && document.Has("iNameA")) {
                    int anchor = anchors[id];
                    if (anchor >= 0 && anchor <= 8 && anchor != 4) {
                        int row = anchor / 3, col = anchor % 3;
                        if ((id == EspLayout::Health && col != 1) || row == 1) {
                            model.items[id].side = col == 2 ? EspLayout::Right : EspLayout::Left;
                            model.items[id].position = row * 0.5f;
                        } else {
                            model.items[id].side = row == 2 ? EspLayout::Bottom : EspLayout::Top;
                            model.items[id].position = col * 0.5f;
                        }
                    }
                }
            }
            model.gap = 4.0f; model.spacing = 3.0f;
            model.barThickness = 3.0f; model.barLength = 1.0f;
            model.stack = true; model.preset = 0;
            model.items[EspLayout::Name].order = 0;
            model.items[EspLayout::Percent].order = 1;
            model.items[EspLayout::Distance].order = 2;
            WriteLayout(model);
            bool backedUp = false;
            char backup[MAX_PATH];
            for (int index = 0; index < 100 && !backedUp; ++index) {
                _snprintf_s(backup, _TRUNCATE, "%s.layout-v4-%03d.bak", path, index);
                if (GetFileAttributesA(backup) != INVALID_FILE_ATTRIBUTES) continue;
                backedUp = CopyFileA(path, backup, TRUE) != FALSE;
                if (!backedUp) break;
            }
            if (!backedUp) strncpy_s(s_cfgStatus, "Layout migrado em memoria. Backup falhou; arquivo original preservado.", _TRUNCATE);
            else if (SaveConfig(name)) strncpy_s(s_cfgStatus, "Layout migrado para v4. Backup do preset anterior preservado.", _TRUNCATE);
        } else {
            WriteLayout(ReadLayout());
            _snprintf_s(s_cfgStatus, _TRUNCATE, "Carregado: %s.json", name);
        }
        fPreviewHp = EspLayout::Clamp(fPreviewHp, 0.0f, 100.0f, 87.0f);
        CloseLayoutEditor();
        Log::Infof("Config carregada: %s", path);
    }

    void RenderOverlay() {
        if (!g_bInit) return;
        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        ImGuiIO& io = ImGui::GetIO();
        const EspLayout::Model layout = VisibleLayout();
        if (Config::bWatermark)
            dl->AddText(ImVec2(10, 10), IM_COL32(120, 200, 255, 220), "ZB2 Menu | D3D11 | INSERT");
        // Debug overlay (Fase 2 item 6): dados VIVOS via reflection C++.
        if (Config::bDebugOverlay) {
            const Mono::State& st = Mono::Get();
            char b[256];
            if (st.ready) {
                const char* modo = st.coopMode == 3 ? "HOST" : st.coopMode == 2 ? "CLIENTE" : st.coopMode == 1 ? "SINGLE" : "LOBBY";
                const char* ammoSt = Config::bInfAmmo ? (st.ammoOk ? "AMMO:ON" : "AMMO:OFF") : "AMMO:-";
                const char* slotSt = Config::bUnlockSlots ? (st.slotsOk ? "SLOTS:ON" : "SLOTS:OFF") : "SLOTS:-";
                const char* loadSt = Config::bUnlockLoadout ? (st.loadoutOn ? "LOAD:ON" : "LOAD:OFF") : "LOAD:-";
                _snprintf_s(b, _TRUNCATE, "LOCAL HP %.0f | STAM %.0f | ALIADO %.0f | ZUMBIS %d (hp0 %.0f) | DAY %.2fh | ESP %d %.1fms | %s | %s wr=%d | %s | %s | %.0f fps",
                    (double)st.localHp, (double)st.localStam, (double)st.allyHp,
                    st.zombies, (double)st.zHp0, (double)st.dayTime, st.espShown, (double)st.espMs, modo, ammoSt, st.ammoWrites, slotSt, loadSt, (double)io.Framerate);
            }
            else
                _snprintf_s(b, _TRUNCATE, "MONO aguardando cena... | %.0f fps", (double)io.Framerate);
            dl->AddText(ImVec2(10, 26), IM_COL32(160, 255, 160, 200), b);
        }
        // ESP Zumbis Box (Fase 3 itens 7/11): head/pes ja em pixels Unity; inverte Y.
        // Ciclo 2 (spec VISUAL §1+§3.13): toggles do menu mandam junto com o
        // layout (OR). Editor drag-drop continua valendo p/ posicao.
        if (Config::bZombieEsp) {
            // R2 (spec VISUAL �3.13): 1 fonte � o MENU manda. Sem OR com o
            // layout (desligar tem que desligar). O editor drag-drop continua
            // valendo p/ posicao; liga/desliga e aqui.
            const bool showSkel = Config::bZombieSkeleton;
            const bool showSnap = Config::bZombieSnap;
            const bool showDot = Config::bZombieHeadDot;
            Mono::EspEntry es[128];
            int n = Mono::GetEsp(es, 128);
            float H = io.DisplaySize.y;
            ImU32 col = ImGui::GetColorU32(ImVec4(Config::colZombieBox[0], Config::colZombieBox[1], Config::colZombieBox[2], Config::colZombieBox[3]));
            ImU32 colBoss = ImGui::GetColorU32(ImVec4(Config::colBossBox[0], Config::colBossBox[1], Config::colBossBox[2], Config::colBossBox[3]));
            // 1 cor por elemento: Skeleton/Linha/Dot usam
            // o picker proprio, nunca a cor do Box.
            ImU32 colSkel = ImGui::GetColorU32(ImVec4(Config::colZombieSkel[0], Config::colZombieSkel[1], Config::colZombieSkel[2], Config::colZombieSkel[3]));
            ImU32 colSnap = ImGui::GetColorU32(ImVec4(Config::colZombieSnap[0], Config::colZombieSnap[1], Config::colZombieSnap[2], Config::colZombieSnap[3]));
            ImU32 colDot = ImGui::GetColorU32(ImVec4(Config::colZombieDot[0], Config::colZombieDot[1], Config::colZombieDot[2], Config::colZombieDot[3]));
            for (int i = 0; i < n; ++i) {
                float h = 0, w = 0, cx = 0, hy = 0, fy = 0;
                ImVec2 r0, r1;
                // R4 (spec �1): Box tem flag propria � bZombieEsp e master,
                // bZombieBoxShow desenha o retangulo. Desmarcar Box nao mata o ESP.
                const bool showBox = Config::bZombieBoxShow;
                bool is3d = (layout.boxStyle == 1 && es[i].has3d);
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
                    // Boss (Riot/Rainha/Ceifador) eh largo: 0.9; comum 0.6.
                    w = h * (es[i].isBoss ? 0.9f : 0.6f);
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
                if (!std::isfinite(r0.x) || !std::isfinite(r0.y) || !std::isfinite(r1.x) || !std::isfinite(r1.y)) continue;
                w = r1.x - r0.x; h = r1.y - r0.y;
                if (w < 2.0f || h < 2.0f) continue;
                // Boss usa a cor propria (colBossBox) se bBossColor; senao cor do zumbi.
                bool useBossCol = es[i].isBoss && Config::bBossColor;
                ImU32 colBox = useBossCol ? colBoss : col;
                if (showBox && layout.boxStyle == 0) {
                    if (r0.x < -10000 || r0.x > 10000 || r0.y < -10000 || r0.y > 10000) continue; // P1 render
                    if (r1.x < -10000 || r1.x > 10000 || r1.y < -10000 || r1.y > 10000) continue;
                    dl->AddRect(r0, r1, colBox, 0.0f, 0, es[i].isBoss ? 2.5f : 1.5f);
                } else if (showBox && layout.boxStyle == 2) {
                    // Corners: 8 segmentos nos cantos (item 11).
                    float cl = h * 0.22f; if (cl > w * 0.45f) cl = w * 0.45f;
                    dl->AddLine(r0, ImVec2(r0.x + cl, r0.y), colBox, 1.5f);
                    dl->AddLine(r0, ImVec2(r0.x, r0.y + cl), colBox, 1.5f);
                    dl->AddLine(ImVec2(r1.x, r0.y), ImVec2(r1.x - cl, r0.y), colBox, 1.5f);
                    dl->AddLine(ImVec2(r1.x, r0.y), ImVec2(r1.x, r0.y + cl), colBox, 1.5f);
                    dl->AddLine(ImVec2(r0.x, r1.y), ImVec2(r0.x + cl, r1.y), colBox, 1.5f);
                    dl->AddLine(ImVec2(r0.x, r1.y), ImVec2(r0.x, r1.y - cl), colBox, 1.5f);
                    dl->AddLine(r1, ImVec2(r1.x - cl, r1.y), colBox, 1.5f);
                    dl->AddLine(r1, ImVec2(r1.x, r1.y - cl), colBox, 1.5f);
                } else if (showBox && layout.boxStyle == 1 && es[i].has3d) {
                    // 3D real: 12 arestas da AABB (frente 4 + fundo 4 + profundidade 4).
                    static const int E[12][2] = { {0,1},{1,3},{3,2},{2,0},{4,5},{5,7},{7,6},{6,4},{0,4},{1,5},{2,6},{3,7} };
                    for (int e = 0; e < 12; ++e) {
                        int a = E[e][0], b = E[e][1];
                        if (!es[i].pv[a] || !es[i].pv[b]) continue;
                        ImVec2 pa = ImVec2(es[i].px[a], H - es[i].py[a]), pb2 = ImVec2(es[i].px[b], H - es[i].py[b]);
                        if (pa.x < -10000 || pa.x > 10000 || pa.y < -10000 || pa.y > 10000) continue; // P1 render
                        if (pb2.x < -10000 || pb2.x > 10000 || pb2.y < -10000 || pb2.y > 10000) continue;
                        dl->AddLine(pa, pb2, colBox, 1.2f);
                    }
                } else {
                    // 3D fallback (bounds indisponivel): face traseira deslocada + arestas.
                    ImVec2 d = ImVec2(w * 0.28f, -h * 0.10f);
                    ImVec2 b0 = ImVec2(r0.x + d.x, r0.y + d.y), b1 = ImVec2(r1.x + d.x, r1.y + d.y);
                    dl->AddRect(b0, b1, colBox, 0.0f, 0, 1.0f);
                    dl->AddRect(r0, r1, colBox, 0.0f, 0, 1.5f);
                    dl->AddLine(r0, b0, colBox, 1.0f); dl->AddLine(ImVec2(r1.x, r0.y), ImVec2(b1.x, b0.y), colBox, 1.0f);
                    dl->AddLine(ImVec2(r0.x, r1.y), ImVec2(b0.x, b1.y), colBox, 1.0f); dl->AddLine(r1, b1, colBox, 1.0f);
                }
                // Item 12 Skeleton real: juntas auditadas ([BONE] 14/09), articulado.
                // Segmentos: coluna, pernas (quadril->pe), bracos (ombro->mao).
                // Boss (rig relativo: HEAD/NECK/SP3/SP1/FL/FR): espinha + pernas.
                ImU32 colSkelUse = (es[i].isBoss && Config::bBossColor) ? colBoss : colSkel;
                if (showSkel && es[i].isBoss) {
                    using MJ = Mono::SkJoint;
                    static const int BSEG[][2] = {
                        { MJ::SK_HEAD, MJ::SK_NECK }, { MJ::SK_NECK, MJ::SK_SP3 },
                        { MJ::SK_SP3, MJ::SK_SP1 },
                        { MJ::SK_SP1, MJ::SK_FL }, { MJ::SK_SP1, MJ::SK_FR }
                    };
                    for (int s = 0; s < 5; ++s) {
                        int a = BSEG[s][0], b = BSEG[s][1];
                        if (!es[i].skV[a] || !es[i].skV[b]) continue;
                        ImVec2 pa = ImVec2(es[i].skX[a], H - es[i].skY[a]);
                        ImVec2 pb = ImVec2(es[i].skX[b], H - es[i].skY[b]);
                        if (pa.x < -10000 || pa.x > 10000 || pa.y < -10000 || pa.y > 10000) continue;
                        if (pb.x < -10000 || pb.x > 10000 || pb.y < -10000 || pb.y > 10000) continue;
                        dl->AddLine(pa, pb, colSkelUse, 2.0f);
                    }
                }
                if (showSkel && !es[i].isBoss && es[i].skN == Mono::SK_COUNT) {
                    using MJ = Mono::SkJoint;
                    static const int SEG[][2] = {
                        { MJ::SK_HEAD, MJ::SK_NECK }, { MJ::SK_NECK, MJ::SK_SP3 },
                        { MJ::SK_SP3, MJ::SK_SP2 }, { MJ::SK_SP2, MJ::SK_SP1 },
                        { MJ::SK_SP1, MJ::SK_HL }, { MJ::SK_HL, MJ::SK_L1L },
                        { MJ::SK_L1L, MJ::SK_L2L }, { MJ::SK_L2L, MJ::SK_FL },
                        { MJ::SK_SP1, MJ::SK_L1R }, { MJ::SK_L1R, MJ::SK_L2R },
                        { MJ::SK_L2R, MJ::SK_FR },
                        { MJ::SK_SP3, MJ::SK_SL }, { MJ::SK_SL, MJ::SK_A1L },
                        { MJ::SK_A1L, MJ::SK_A2L },
                        { MJ::SK_SP3, MJ::SK_SR }, { MJ::SK_SR, MJ::SK_A1R },
                        { MJ::SK_A1R, MJ::SK_A2R },
                        { MJ::SK_A2L, MJ::SK_HL2L }, { MJ::SK_A2R, MJ::SK_HL2R } // mao estimada
                    };
                    constexpr int nSeg = (int)(sizeof(SEG) / sizeof(SEG[0]));
                    for (int s = 0; s < nSeg; ++s) {
                        int a = SEG[s][0], b = SEG[s][1];
                        if (!es[i].skV[a] || !es[i].skV[b]) continue;
                        ImVec2 pa = ImVec2(es[i].skX[a], H - es[i].skY[a]);
                        ImVec2 pb = ImVec2(es[i].skX[b], H - es[i].skY[b]);
                        if (pa.x < -10000 || pa.x > 10000 || pa.y < -10000 || pa.y > 10000) continue;
                        if (pb.x < -10000 || pb.x > 10000 || pb.y < -10000 || pb.y > 10000) continue;
                        dl->AddLine(pa, pb, colSkelUse, 1.5f);
                    }
                    // Sem circulo na cabeca: Head Dot (item 13) e funcao separada do menu.
                }
                // Item 13 Snapline: base da tela -> centro do pe da box (padrao grandes cheats).
                if (showSnap && w > 2.0f && h > 2.0f) {
                    ImVec2 base = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y);
                    ImVec2 tgt = ImVec2((r0.x + r1.x) * 0.5f, r1.y);
                    if (tgt.x > -10000 && tgt.x < 10000 && tgt.y > -10000 && tgt.y < 10000)
                        dl->AddLine(base, tgt, colSnap, 1.0f);
                }
                // Item 13 Head Dot: ponto na junta HEAD (projetada, nao fixa no box).
                // Boss: skN vem do rig relativo (HEAD sempre mapeado quando existe).
                if (showDot && es[i].skV[Mono::SK_HEAD] && (es[i].isBoss || es[i].skN == Mono::SK_COUNT)) {
                    ImVec2 hp = ImVec2(es[i].skX[Mono::SK_HEAD], H - es[i].skY[Mono::SK_HEAD]);
                    if (hp.x > -10000 && hp.x < 10000 && hp.y > -10000 && hp.y < 10000) {
                        float hr = h * 0.03f; if (hr < 2.0f) hr = 2.0f; if (hr > 6.0f) hr = 6.0f;
                        dl->AddCircleFilled(hp, hr, colDot);
                    }
                }
                EspLayout::Rect envelope = { r0, r1 };
                if (layout.boxStyle == 1 && !es[i].has3d) {
                    envelope.min.y -= h * 0.10f;
                    envelope.max.x += w * 0.28f;
                }
                // Nome real ja vem do snapshot (type 0..8 -> PT-BR no mono.cpp).
                // Sem sufixo: o nome do boss JA diz (Zumbi de Assalto/Rainha/Ceifador).
                auto content = LayoutContent(es[i].name, es[i].dist, es[i].hp, es[i].maxHp);
                auto style = LayoutStyle(content.health);
                if (es[i].isBoss && Config::bBossColor) {
                    // Nome do boss na cor propria (o layout usa a cor do zumbi).
                    style.color[EspLayout::Name] = colBoss;
                    style.boxColor = colBoss;
                }
                auto geometry = EspLayout::Resolve(layout, envelope, { ImVec2(2.0f, 2.0f), ImVec2(io.DisplaySize.x - 2.0f, io.DisplaySize.y - 2.0f) }, content, style, true);
                EspLayout::Draw(dl, geometry, content, style);
            }
        }
        // Circulo do raio (wohax show_aim_radius): so com raio > 0.
        if (Config::bDrawFov && Config::bAimbot && Config::bLimitFov && !Config::b360Mode && Config::fFovAngle > 0.0f) {
            ImVec2 sc = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
            float r = Config::fFovAngle * 4.0f;
            dl->AddCircle(sc, r, IM_COL32(120, 220, 255, 200), 64, 1.2f);
        }
    }

    void Render() {
        if (!g_bInit) return;
        ImGui::GetIO().MouseDrawCursor = Config::bMenuOpen;
        if (!Config::bMenuOpen) { CloseLayoutEditor(); return; }
        s_editorVisible = false;
        if (ImGui::GetIO().AppFocusLost) EspLayout::CancelDrag(s_editor);
        ImGui::SetNextWindowSize(ImVec2(860, 620), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(440, 400), ImVec2(4096, 4096));
        if (!ImGui::Begin("ZB2 Menu - Zumbi Blocks 2 (D3D11)", &Config::bMenuOpen)) { ImGui::End(); CloseLayoutEditor(); return; }

        if (ImGui::BeginTabBar("ZB2Tabs")) {
            // ---- 1 PLAYER ----
            // Categoria aimbot espelho wohax (mesma logica de funcionamento):
            // enable + radius + bone + mode; mira so com firing (tecla/modo).
            if (ImGui::BeginTabItem("PLAYER")) {
                ImGui::Checkbox("Aimbot", &Config::bAimbot); Tip("Igual ao wohax: elege por px, mira vetor 3D. Segure a Aim Key.");
                // Aim Key + Hold/Toggle + LED vivo (padrao BF/Warface/wohax firing).
                {
                    ImGui::Text("Aim Key"); ImGui::SameLine();
                    char nm[64] = { 0 };
                    KeyName(Config::iAimKey, nm, sizeof(nm));
                    if (ImGui::Button(nm, ImVec2(130, 0))) {
                        s_capKey = &Config::iAimKey;
                        ImGui::OpenPopup("hotkey_modal");
                    }
                    Tip("Tecla que ativa a mira (firing do wohax). Troque em SETTINGS > Aim Key.");
                    ImGui::SameLine();
                    static const char* kAimModes[2] = { "Hold", "Toggle" };
                    ImGui::SetNextItemWidth(90.0f);
                    ImGui::Combo("##AimMode", &Config::iAimMode, kAimModes, 2);
                    Tip("Hold = mira enquanto segura. Toggle = trava no 1o aperto.");
                    ImGui::SameLine();
                    bool down = (Config::iAimKey != 0) && ((GetAsyncKeyState(Config::iAimKey) & 0x8000) != 0);
                    ImGui::TextColored(down ? ImVec4(0.2f, 1.0f, 0.3f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                        down ? "(●)" : "(○)");
                    Tip("LED vivo: acende segurando a tecla.");
                }
                ImGui::Combo("Aim Bone", &Config::iAimBone, kAimBones, 4); Tip("Head/Neck/Chest/Pelvis (bone 3D, igual ao wohax).");
                ImGui::Combo("Aim Priority", &Config::iAimPriority, kAimPrio, 3); Tip("Crosshair = menor px (wohax); Nearest/HP = extensao.");
                ImGui::SliderFloat("Smoothing", &Config::fSmoothing, 1, 8, "%.0f"); Tip("1 = snap seco wohax (padrao). 2-8 = divide o passo (suave, mais lento).");
                ImGui::SliderFloat("Aim Radius", &Config::fFovAngle, 0, 360, "%.0f px"); Tip("Raio em px (aim_radius wohax, 0 = off = infinito).");
                ImGui::SliderFloat("Aim Distance", &Config::fAimDistance, 10, 500, "%.0fm"); Tip("Só o AIM obedece (ate onde mira). O ESP tem a propria distancia no VISUAL.");
                ImGui::Checkbox("Auto Aim", &Config::bAutoAim); Tip("Mira sozinho, sem tecla (firing sempre 1).");
                // Silent/AutoFire/Trigger: funcionam COM ou SEM AutoAim.
                // - Triggerbot: voce mira, ele atira (centro no inimigo).
                // - Auto Fire: ele mira + atira sozinho.
                // - Silent Aim: atira sem puxar a camera (tiro sai de onde
                //   a camera ja esta; sem forja de tiro — honesto).
                ImGui::Checkbox("Silent Aim", &Config::bSilentAim); Tip("Atira sem mover a camera. Exige Auto Aim junto (elege+mira invisivel+atira).");
                ImGui::Checkbox("Auto Fire", &Config::bAutoFire); Tip("Mira + atira sozinho quando ha alvo valido e visivel.");
                ImGui::Checkbox("Triggerbot", &Config::bTriggerbot); Tip("Voce mira, ele atira quando o centro encosta no inimigo (12px).");
                ImGui::Separator();
                // wohax nao tem 360/prediction/lag — mantidos, apagados do mira:
                // LimitFOV vira "raio on/off", Draw desenha o raio, 360 = raio
                // infinito. Prediction/Lag seguem vitrine (ciclo futuro).
                ImGui::Checkbox("Limit Radius", &Config::bLimitFov); Tip("On = mira so dentro do raio (wohax aim_radius). Off = infinito.");
                ImGui::Checkbox("360 Mode", &Config::b360Mode); Tip("Mira em qualquer direcao (ignora o raio). Só o AIM: o ESP continua desenhando normal.");
                ImGui::Checkbox("Draw Radius", &Config::bDrawFov); Tip("Desenha o circulo do raio (aim_can_show_radius wohax).");
                ImGui::Separator();
                ImGui::Text("Weapon Mods");
                ImGui::Checkbox("No Recoil", &Config::bNoRecoil);
                ImGui::Checkbox("No Spread", &Config::bNoSpread);
                ImGui::Checkbox("No Sway", &Config::bNoSway);
                ImGui::Checkbox("Mira Fechada", &Config::bTightAim); Tip("Crosshair junto: tamanho minimo (visual) + spread zero (tiro).");
                ImGui::Checkbox("Rapid Fire", &Config::bRapidFire);
                ImGui::SliderFloat("Rapid Mult", &Config::fRapidMult, 1, 8, "%.1fx"); Tip("8x = minigun (pistola 1-tiro vira rajada).");
                ImGui::Checkbox("Fast Knife", &Config::bFastKnife); Tip("Arma branca rapida: pa, pa, facao, faca, taco (Duration curto).");
                ImGui::SliderFloat("Knife Mult", &Config::fKnifeMult, 1, 5, "%.1fx");
                ImGui::Checkbox("Instant Reload", &Config::bInstantReload); Tip("Recarga instantanea: completa o pente na hora (sem animacao). Single/host. Como cliente use a recarga normal (R).");
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
                ImGui::SliderFloat("Jump Mult", &Config::fJumpMult, 1, 10, "%.1fx");
                ImGui::Checkbox("Roll Speed", &Config::bRollSpeed);
                ImGui::SliderFloat("Roll Mult", &Config::fRollMult, 1, 5, "%.1fx");
                ImGui::EndTabItem();
            }
            // ---- 2 VISUAL ----
            if (ImGui::BeginTabItem("VISUAL")) {
                // Padrao mercado: 2 colunas (ESP | Mundo), cabecalho por grupo,
                // filhos indentados, swatch a direita. So apresentacao.
                static const char* kSnapFrom[] = { "Base", "Topo", "Centro" };
                if (ImGui::BeginTable("visual_cols", 2)) {
                    ImGui::TableNextColumn();
                    Section("Zumbis");
                    ImGui::Checkbox("Ativo##Z", &Config::bZombieEsp); Tip("Master do ESP de zumbis (worker + desenho).");
                    if (Config::bZombieEsp) {
                        ImGui::Indent();
                        ImGui::Checkbox("Box", &Config::bZombieBoxShow); Tip("Desenha o retangulo. Desmarcar nao mata nome/vida.");
                        ImGui::SameLine(); ImGui::SetNextItemWidth(96.0f);
                        ImGui::Combo("Tipo", &Config::iZombieBox, kBoxType, 3); Tip("2D / 3D real (AABB) / Cantos.");
                        SwatchR("##CorBoxZ", Config::colZombieBox, "Cor do box.");
                        ImGui::Checkbox("Boss (cor)", &Config::bBossColor); Tip("Boss usa cor propria (box+nome+esqueleto). Desligado = cor do zumbi.");
                        SwatchR("##CorBossZ", Config::colBossBox, "Cor do boss (Assalto/Rainha/Ceifador).");
                        ImGui::Checkbox("Nome", &Config::bZombieName); Tip("Nome real (tipo do zumbi).");
                        SwatchR("##CorNomeZ", Config::colZombieName, "Cor do nome.");
                        ImGui::Checkbox("Distancia", &Config::bZombieDist); Tip("Distancia em metros ate a camera.");
                        SwatchR("##CorDistZ", Config::colZombieDist, "Cor da distancia.");
                        ImGui::Checkbox("Barra de vida", &Config::bZombieHp);
                        ImGui::SameLine(); ImGui::Checkbox("%", &Config::bZombiePct); Tip("Numero arrastavel no Avancado.");
                        SwatchR("##CorVidaZ", Config::colZombieHp, "Cor do texto de vida (a barra usa HpColor).");
                        ImGui::Checkbox("Esqueleto", &Config::bZombieSkeleton); Tip("19 segmentos articulados (< 50m).");
                        SwatchR("##CorSkelZ", Config::colZombieSkel, "Cor do esqueleto.");
                        ImGui::Checkbox("Linha", &Config::bZombieSnap);
                        ImGui::SameLine(); ImGui::SetNextItemWidth(80.0f);
                        ImGui::Combo("Origem", &Config::iSnapFrom, kSnapFrom, 3); Tip("De onde sai a linha: base/topo/centro da tela.");
                        SwatchR("##CorLinhaZ", Config::colZombieSnap, "Cor da linha.");
                        ImGui::Checkbox("Ponto na cabeca", &Config::bZombieHeadDot); Tip("Head dot projetado na junta HEAD.");
                        SwatchR("##CorDotZ", Config::colZombieDot, "Cor do ponto.");
                        ImGui::SetNextItemWidth(160.0f);
                        ImGui::SliderFloat("Dist. maxima", &Config::fEspDistance, 10, 500, "%.0fm"); Tip("Só o ESP obedece (ate onde desenha). O AIM tem a propria distancia no PLAYER.");
                        ImGui::Unindent();
                    }
                    Section("Aliados");
                    ImGui::Checkbox("Ativo##A", &Config::bAllyEsp); Tip("ESP dos jogadores (sempre azul).");
                    if (Config::bAllyEsp) {
                        ImGui::Indent();
                        ImGui::SetNextItemWidth(96.0f);
                        ImGui::Combo("Box", &Config::iAllyBox, kBoxType, 3); Tip("2D / 3D real (AABB) / Cantos.");
                        SwatchR("##CorAlly", Config::colAllyVis, "Cor unica do aliado.");
                        ImGui::Checkbox("Nome##A", &Config::bAllyName);
                        ImGui::SameLine(); ImGui::Checkbox("Dist.##A", &Config::bAllyDist);
                        ImGui::SameLine(); ImGui::Checkbox("Vida##A", &Config::bAllyHp);
                        ImGui::Checkbox("Esqueleto##A", &Config::bAllySkeleton);
                        ImGui::SameLine(); ImGui::Checkbox("Linha##A", &Config::bAllySnap);
                        ImGui::SameLine(); ImGui::Checkbox("Ponto##A", &Config::bAllyHeadDot);
                        ImGui::Unindent();
                    }
                    Section("Chams");
                    ImGui::Checkbox("Corpo (XQZ)", &Config::bChams); Tip("Atravessa parede. O proprio checkbox e o enable.");
                    if (Config::bChams) {
                        ImGui::Indent();
                        ImGui::TextDisabled("Visivel"); SwatchR("##ChVis", Config::colChamsVis, "Cor com linha de visada livre.");
                        ImGui::TextDisabled("Oculto"); SwatchR("##ChInv", Config::colChamsInv, "Cor atras de parede.");
                        ImGui::Unindent();
                    }
                    ImGui::TableNextColumn();
                    Section("Itens");
                    ImGui::Checkbox("Ativo##I", &Config::bItemEsp); Tip("Master do ESP de itens no chao.");
                    if (Config::bItemEsp) {
                        ImGui::Indent();
                        ImGui::Checkbox("Armas", &Config::bItemWeapons);
                        SwatchR("##CArma", Config::colItem, "Cor das armas.");
                        ImGui::SameLine(); ImGui::Checkbox("Raros", &Config::bItemRare);
                        SwatchR("##CRaro", Config::colItemRare, "Cor dos raros.");
                        ImGui::Checkbox("Municao", &Config::bItemAmmo);
                        SwatchR("##CMunic", Config::colItemAmmo, "Cor da municao.");
                        ImGui::SameLine(); ImGui::Checkbox("Suprimento", &Config::bItemSupply);
                        SwatchR("##CSupri", Config::colItemSupply, "Cor do suprimento.");
                        ImGui::SetNextItemWidth(160.0f);
                        ImGui::SliderFloat("Raio", &Config::fItemRadius, 10, 500, "%.0fm"); Tip("So mostra item dentro deste raio.");
                        ImGui::Unindent();
                    }
                    Section("Pontos / Mundo");
                    ImGui::Checkbox("Ativo##P", &Config::bPoiEsp); Tip("Master dos pontos de interesse.");
                    if (Config::bPoiEsp) {
                        ImGui::Indent();
                        ImGui::Checkbox("Helicoptero", &Config::bPoiHeli);
                        SwatchR("##PHeli", Config::colPoiHeli, "Sempre visivel (dinamico).");
                        ImGui::SameLine(); ImGui::Checkbox("Chefao", &Config::bPoiBoss);
                        SwatchR("##PBoss", Config::colPoiBoss, "Sempre visivel (dinamico).");
                        ImGui::Checkbox("Missao", &Config::bPoiMission);
                        SwatchR("##PMiss", Config::colPoiMission, "Sempre visivel (dinamico).");
                        ImGui::SameLine(); ImGui::Checkbox("Onda", &Config::bPoiWave);
                        SwatchR("##PWave", Config::colPoiWave, "Mostra a direcao da onda.");
                        ImGui::Checkbox("Loot fixo", &Config::bPoiLootFix);
                        SwatchR("##PLoot", Config::colPoiLootFix, "Dentro do raio (estatico).");
                        ImGui::SameLine(); ImGui::Checkbox("Bancadas", &Config::bPoiBench);
                        SwatchR("##PBench", Config::colPoiBench, "Dentro do raio (estatico).");
                        ImGui::Checkbox("Fogueira", &Config::bPoiFire);
                        SwatchR("##PFire", Config::colPoiFire, "Dentro do raio (estatico).");
                        ImGui::SameLine(); ImGui::Checkbox("Mercador", &Config::bPoiShop);
                        SwatchR("##PShop", Config::colPoiShop, "Dentro do raio (estatico).");
                        ImGui::Checkbox("Respawn", &Config::bPoiRespawn);
                        SwatchR("##PResp", Config::colPoiRespawn, "Sempre visivel (dinamico).");
                        ImGui::Unindent();
                    }
                    ImGui::EndTable();
                }
                // ======== LAYOUT AVANCADO (colapsado) ========
                if (ImGui::CollapsingHeader("Avancado (layout drag-drop)")) {
                    DrawLayoutEditor();
                }
                ImGui::EndTabItem();
            }
            // ---- 3 MISC (sempre 3a aba) ----
            if (ImGui::BeginTabItem("MISC")) {
                ImGui::Text("Sobrevivencia");
                ImGui::Checkbox("God Mode (HP travado)", &Config::bGodMode); Tip("Trava HP local em 100.");
                ImGui::Checkbox("Infinite Stamina", &Config::bInfStamina); Tip("Stamina no maximo (correr sem cansar).");
                ImGui::Checkbox("Infinite Ammo (pente+reserva)", &Config::bInfAmmo); Tip("Pente no maxAmmo + reserva do tipo no stackMax.");
                ImGui::Checkbox("Infinite Items (por categoria)", &Config::bInfItems); Tip("Tudo no teto: bala, granada, bandagem, madeira 999x.");
                ImGui::Checkbox("Infinite Money (3 moedas)", &Config::bInfMoney); Tip("Dolar/prata/ouro em 99999 (compra Loadout I/II/III).");
                // REMOVIDO 17/09: Anti-Flood (quarentena bania o proprio infinito).
                ImGui::Checkbox("Desbloquear Slots", &Config::bUnlockSlots); Tip("Storage+misc cheios (1x por sessao).");
                ImGui::Checkbox("Desbloquear Loadout", &Config::bUnlockLoadout); Tip("Kits do loadout livres (a loja compra com Money: use Infinite Money).");
                ImGui::Separator();
                // REMOVIDO 17/09: Spawn de Itens (nao util; IDs ficam no historico p/ fase futura).
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
        if (!s_editorVisible) CloseLayoutEditor();
        ImGui::End();
    }
}

















