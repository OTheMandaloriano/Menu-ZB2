#include "../apps/loader/preview_ui.cpp"
#include "software_renderer.h"
#include <stdexcept>
int main(){
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize=ImVec2(760,530);io.DeltaTime=1.f/60;
    ImGui::StyleColorsDark();auto& style=ImGui::GetStyle();style.WindowPadding=ImVec2(22,18);style.FramePadding=ImVec2(10,8);style.ItemSpacing=ImVec2(10,10);style.FrameRounding=5;
    style.Colors[ImGuiCol_WindowBg]=ImVec4(.055f,.065f,.075f,1);style.Colors[ImGuiCol_ChildBg]=ImVec4(.07f,.08f,.09f,1);
    style.Colors[ImGuiCol_Button]=ImVec4(.10f,.30f,.29f,1);
    io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf",18.f);io.Fonts->Build();
    for(int tab=0;tab<3;++tab){
        page=tab;
        for(int i=0;i<2;++i){ImGui::NewFrame();DrawLoaderPreview();ImGui::Render();}
        if(ImGui::GetDrawData()->TotalVtxCount==0)throw std::runtime_error("empty preview");
        if(!RenderPpm("loader-"+std::to_string(tab)+".ppm",1))throw std::runtime_error("render failed");
    }
    ImGui::DestroyContext();std::puts("PASS: three loader pages rendered without opening desktop windows");
}
