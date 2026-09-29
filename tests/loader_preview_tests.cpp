#include "../apps/loader/preview_ui.h"
#include "software_renderer.h"
#include <stdexcept>
int main(){
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DeltaTime=1.f/60;
    for(int dpi=1;dpi<=2;++dpi){
        ConfigureLoaderPreview(float(dpi));io.Fonts->Build();io.DisplaySize=ImVec2(420.f*dpi,460.f*dpi);
        for(int tab=0;tab<2;++tab){
            LoaderPreviewState state;state.page=tab;
            for(int i=0;i<2;++i){ImGui::NewFrame();DrawLoaderPreview(state);ImGui::Render();}
            if(ImGui::GetDrawData()->TotalVtxCount==0)throw std::runtime_error("empty preview");
            if(!RenderPpm("loader-v2-"+std::to_string(tab)+"-"+std::to_string(dpi)+".ppm",1))throw std::runtime_error("render failed");
        }
    }
    ConfigureLoaderPreview(1);io.Fonts->Build();io.DisplaySize=ImVec2(420,460);
    LoaderPreviewState state;
    auto frame=[&](){ImGui::NewFrame();DrawLoaderPreview(state);ImGui::Render();};
    auto click=[&](float x,float y){io.MousePos=ImVec2(x,y);io.MouseDown[0]=false;frame();io.MouseDown[0]=true;frame();io.MouseDown[0]=false;frame();};
    frame();frame();click(200,313);
    if(!state.attempted || state.page!=0)throw std::runtime_error("preview must not activate a license");
    click(200,387);if(state.page!=1 || state.attempted)throw std::runtime_error("demo navigation");
    click(200,380);if(!state.attempted)throw std::runtime_error("load shows explicit preview feedback");
    click(200,438);if(state.page!=0)throw std::runtime_error("return navigation");
    click(350,20);if(!state.minimize)throw std::runtime_error("minimize action");
    click(390,20);if(!state.close)throw std::runtime_error("close action");
    ImGui::DestroyContext();std::puts("PASS: two compact loader pages rendered at 100% and 200% DPI");
}
