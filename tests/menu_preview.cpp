// Render the production menu without a game, input automation or personal profile.
#include "../src/menu/gui.cpp"
#include "software_renderer.h"
#include <filesystem>
namespace Log { const char* GetDir(){return "";} void Infof(const char*,...){} }
namespace Mono {
    static State state;
    const State& Get(){return state;}
    int GetCatalog(CatalogEntry*,int){return 0;}
    void RequestIcon(int){}
    bool GetIcon(IconPixels&){return false;}
    int GetEsp(EspEntry*,int){return 0;}
    int GetWorldEsp(WorldMarker*,int){return 0;}
    int GetDistantEsp(DistantMarker*,int){return 0;}
}
bool ImGui_ImplWin32_Init(void*){return true;}
void ImGui_ImplWin32_Shutdown(){}
bool ImGui_ImplDX11_Init(ID3D11Device*,ID3D11DeviceContext*){return true;}
void ImGui_ImplDX11_Shutdown(){}
int main(int argc,char** argv){
    if(argc!=2)return 2;
    std::filesystem::path directory=argv[1];std::filesystem::create_directories(directory);
    GUI::Initialize(nullptr,nullptr,nullptr);auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;
    io.DisplaySize={908,684};io.DeltaTime=1.f/60;io.MousePos={-100,-100};io.Fonts->Build();
    Config::bMenuOpen=true;Config::bWatermark=false;Config::bDebugOverlay=false;
    for(int i=0;i<4;++i){ImGui::NewFrame();ImGui::SetNextWindowPos({24,24});GUI::Render();ImGui::Render();}
    bool ok=RenderPpm((directory/"menu.ppm").string(),1);GUI::Shutdown();return ok?0:1;
}
