#include "theme.h"
#include "../shared/resources.h"
#include "../loader/assets/IconsFontAwesome6.h"
#include <array>
#include "cover.h"
#include <algorithm>
#include "../../imgui/misc/freetype/imgui_freetype.h"
namespace UiTheme {
ImFont* regular=nullptr;
ImFont* heading=nullptr;
ImFont* icons=nullptr;
ImFont* button=nullptr;
ImFont* caption=nullptr;
float uiScale=1;
}
using namespace UiTheme;
void ConfigureUiTheme(float dpi){
    uiScale=dpi;ImGui::GetStyle()=ImGuiStyle();ImGui::StyleColorsDark();auto& style=ImGui::GetStyle();
    style.WindowPadding=ImVec2(20,18);style.FramePadding=ImVec2(14,10);style.ItemSpacing=ImVec2(12,12);
    style.FrameRounding=4;style.WindowRounding=8;style.WindowBorderSize=1;style.FrameBorderSize=1;
    style.Colors[ImGuiCol_WindowBg]=ImVec4(9/255.f,11/255.f,14/255.f,1);
    style.Colors[ImGuiCol_Border]=ImVec4(1,1,1,.06f);
    style.Colors[ImGuiCol_FrameBg]=ImVec4(25/255.f,29/255.f,40/255.f,1);
    style.Colors[ImGuiCol_FrameBgHovered]=ImVec4(.15f,.15f,.17f,1);
    style.Colors[ImGuiCol_FrameBgActive]=ImVec4(.18f,.18f,.20f,1);
    style.Colors[ImGuiCol_Header]=ImVec4(.19f,.19f,.22f,1);
    style.Colors[ImGuiCol_HeaderHovered]=ImVec4(.24f,.24f,.27f,1);
    style.Colors[ImGuiCol_HeaderActive]=ImVec4(.28f,.28f,.31f,1);
    style.Colors[ImGuiCol_PopupBg]=ImVec4(.12f,.12f,.14f,1);
    style.Colors[ImGuiCol_TextSelectedBg]=ImVec4(.33f,.34f,.39f,.6f);
    style.Colors[ImGuiCol_Text]=ImVec4(243/255.f,244/255.f,246/255.f,1);
    style.Colors[ImGuiCol_TextDisabled]=ImVec4(157/255.f,164/255.f,178/255.f,1);
    style.Colors[ImGuiCol_Button]=ImVec4(37/255.f,99/255.f,235/255.f,1);
    style.Colors[ImGuiCol_ButtonHovered]=ImVec4(48/255.f,112/255.f,246/255.f,1);
    style.Colors[ImGuiCol_ButtonActive]=ImVec4(27/255.f,77/255.f,190/255.f,1);
    style.Colors[ImGuiCol_CheckMark]=ImVec4(16/255.f,185/255.f,129/255.f,1);
    style.Colors[ImGuiCol_NavHighlight]=ImVec4(.14f,.38f,.85f,.60f);
    style.ScaleAllSizes(uiScale);
    static std::array<std::string,6> fonts={AppResources::Read(201),AppResources::Read(202),AppResources::Read(203),AppResources::Read(204),AppResources::Read(206),AppResources::Read(207)};
    auto& atlas=*ImGui::GetIO().Fonts;atlas.Clear();atlas.FontBuilderIO=ImGuiFreeType::GetBuilderForFreeType();atlas.FontBuilderFlags=ImGuiFreeTypeBuilderFlags_LightHinting;
    static const ImWchar textRanges[]={0x0020,0x00ff,0x2000,0x206f,0};
    auto add=[&](size_t index,float size){ImFontConfig config;config.FontDataOwnedByAtlas=false;
        auto* result=atlas.AddFontFromMemoryTTF(fonts[index].data(),static_cast<int>(fonts[index].size()),size*uiScale,&config,textRanges);
        return result;};
    auto merge=[&](){
        static const ImWchar ranges[]={ICON_MIN_FA,ICON_MAX_16_FA,0};ImFontConfig config;
        config.FontDataOwnedByAtlas=false;config.MergeMode=true;config.PixelSnapH=true;config.GlyphMinAdvanceX=16*uiScale;
        // Baseline for native ImGui text. Custom widgets center glyph ink bounds,
        // so their icon/text pairs do not accumulate a second offset.
        config.GlyphOffset=ImVec2(0,-2.5f*uiScale);
        atlas.AddFontFromMemoryTTF(fonts[3].data(),static_cast<int>(fonts[3].size()),14*uiScale,&config,ranges);
    };
    regular=add(0,14);merge();button=regular;heading=add(1,18);caption=add(5,12);icons=regular;
    ImGui::GetIO().FontDefault=regular;
    Cover::Prepare();
    style.HoverDelayShort=.15f;style.HoverStationaryDelay=0.f;
    style.Colors[ImGuiCol_TableHeaderBg]=ImVec4(1,1,1,.025f);
    style.Colors[ImGuiCol_TableRowBg]=ImVec4(0,0,0,0);
    style.Colors[ImGuiCol_TableRowBgAlt]=ImVec4(1,1,1,.015f);
    style.Colors[ImGuiCol_TableBorderLight]=ImVec4(1,1,1,.06f);
    style.Colors[ImGuiCol_HeaderHovered]=ImVec4(1,1,1,.045f);
    style.Colors[ImGuiCol_Header]=ImVec4(37/255.f,99/255.f,235/255.f,.10f);
}
