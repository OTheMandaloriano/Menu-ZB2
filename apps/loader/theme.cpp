#include "theme.h"
#include "services.h"
#include "assets/IconsFontAwesome6.h"
#include <array>
namespace LoaderTheme {
ImFont* regular=nullptr;
ImFont* heading=nullptr;
ImFont* icons=nullptr;
ImFont* button=nullptr;
ImFont* small=nullptr;
float uiScale=1;
}
using namespace LoaderTheme;
void ConfigureLoaderTheme(float dpi){
    uiScale=dpi;ImGui::GetStyle()=ImGuiStyle();ImGui::StyleColorsDark();auto& style=ImGui::GetStyle();
    style.WindowPadding=ImVec2(20,18);style.FramePadding=ImVec2(14,10);style.ItemSpacing=ImVec2(12,12);
    style.FrameRounding=4;style.WindowRounding=8;style.WindowBorderSize=1;style.FrameBorderSize=1;
    style.Colors[ImGuiCol_WindowBg]=ImVec4(26/255.f,26/255.f,28/255.f,1);
    style.Colors[ImGuiCol_Border]=ImVec4(1,1,1,.06f);
    style.Colors[ImGuiCol_FrameBg]=ImVec4(24/255.f,24/255.f,26/255.f,1);
    style.Colors[ImGuiCol_FrameBgHovered]=ImVec4(.12f,.14f,.18f,1);
    style.Colors[ImGuiCol_FrameBgActive]=ImVec4(.14f,.16f,.20f,1);
    style.Colors[ImGuiCol_Text]=ImVec4(243/255.f,244/255.f,246/255.f,1);
    style.Colors[ImGuiCol_TextDisabled]=ImVec4(157/255.f,164/255.f,178/255.f,1);
    style.Colors[ImGuiCol_Button]=ImVec4(37/255.f,99/255.f,235/255.f,1);
    style.Colors[ImGuiCol_ButtonHovered]=ImVec4(48/255.f,112/255.f,246/255.f,1);
    style.Colors[ImGuiCol_ButtonActive]=ImVec4(27/255.f,77/255.f,190/255.f,1);
    style.Colors[ImGuiCol_CheckMark]=ImVec4(16/255.f,185/255.f,129/255.f,1);
    style.Colors[ImGuiCol_NavHighlight]=ImVec4(.5f,.7f,1,1);
    style.ScaleAllSizes(uiScale);
    static std::array<std::string,5> fonts={LoaderServices::Resource(201),LoaderServices::Resource(202),LoaderServices::Resource(203),LoaderServices::Resource(204),LoaderServices::Resource(206)};
    auto& atlas=*ImGui::GetIO().Fonts;atlas.Clear();
    auto add=[&](size_t index,float size){ImFontConfig config;config.FontDataOwnedByAtlas=false;
        return atlas.AddFontFromMemoryTTF(fonts[index].data(),static_cast<int>(fonts[index].size()),size*uiScale,&config,atlas.GetGlyphRangesDefault());};
    auto merge=[&](){
        static const ImWchar ranges[]={ICON_MIN_FA,ICON_MAX_16_FA,0};ImFontConfig config;
        config.FontDataOwnedByAtlas=false;config.MergeMode=true;config.PixelSnapH=true;config.GlyphMinAdvanceX=16*uiScale;
        atlas.AddFontFromMemoryTTF(fonts[3].data(),static_cast<int>(fonts[3].size()),14*uiScale,&config,ranges);
    };
    regular=add(4,14);merge();button=regular;heading=add(4,28);small=add(4,12);icons=regular;
    ImGui::GetIO().FontDefault=regular;
    style.HoverDelayShort=.15f;
}
