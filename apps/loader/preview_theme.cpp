#include "preview_theme.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <cstdio>
namespace LoaderTheme {
ImFont* regular=nullptr;
ImFont* heading=nullptr;
ImFont* icons=nullptr;
float uiScale=1;
}
using namespace LoaderTheme;
static ImFont* LoadFont(const char* path,float size,const ImWchar* ranges=nullptr){
    DWORD attributes=GetFileAttributesA(path);
    if(attributes==INVALID_FILE_ATTRIBUTES || (attributes&FILE_ATTRIBUTE_DIRECTORY))return nullptr;
    return ImGui::GetIO().Fonts->AddFontFromFileTTF(path,size,nullptr,ranges);
}
void ConfigureLoaderPreview(float dpi){
    uiScale=dpi;ImGui::GetStyle()=ImGuiStyle();ImGui::StyleColorsDark();auto& style=ImGui::GetStyle();
    style.WindowPadding=ImVec2(0,0);style.FramePadding=ImVec2(12,12);style.FrameRounding=6;style.WindowRounding=10;style.WindowBorderSize=1;
    style.Colors[ImGuiCol_WindowBg]=ImVec4(27/255.f,28/255.f,31/255.f,1);
    style.Colors[ImGuiCol_Border]=ImVec4(53/255.f,55/255.f,60/255.f,1);
    style.Colors[ImGuiCol_FrameBg]=ImVec4(20/255.f,21/255.f,24/255.f,1);
    style.Colors[ImGuiCol_FrameBgHovered]=ImVec4(.11f,.12f,.14f,1);
    style.Colors[ImGuiCol_FrameBgActive]=ImVec4(.11f,.12f,.14f,1);
    style.Colors[ImGuiCol_Text]=ImVec4(.94f,.94f,.95f,1);
    style.Colors[ImGuiCol_TextDisabled]=ImVec4(.63f,.65f,.69f,1);
    style.Colors[ImGuiCol_NavHighlight]=ImVec4(.66f,.73f,.84f,1);style.ScaleAllSizes(uiScale);
    wchar_t windows[MAX_PATH]={};GetWindowsDirectoryW(windows,MAX_PATH);char base[MAX_PATH*3]={},path[MAX_PATH*3]={};
    WideCharToMultiByte(CP_UTF8,0,windows,-1,base,sizeof(base),nullptr,nullptr);
    auto& atlas=*ImGui::GetIO().Fonts;atlas.Clear();
    snprintf(path,sizeof(path),"%s/Fonts/segoeui.ttf",base);
    regular=LoadFont(path,14*uiScale);if(!regular)regular=atlas.AddFontDefault();
    snprintf(path,sizeof(path),"%s/Fonts/seguisb.ttf",base);heading=LoadFont(path,29*uiScale);if(!heading)heading=regular;
    static const ImWchar range[]={0xe000,0xf8ff,0};snprintf(path,sizeof(path),"%s/Fonts/segmdl2.ttf",base);
    icons=LoadFont(path,16*uiScale,range);
    ImGui::GetIO().FontDefault=regular;
}
