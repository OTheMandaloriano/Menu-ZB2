#include "../gui.cpp"
#include <cstdlib>
namespace Log { const char* GetDir(){return ".";} void Infof(const char*,...){} }
namespace Mono {
    static State state; static EspEntry entry; static WorldMarker marker; static int worldCount=0;
    const State& Get(){return state;}
    int GetEsp(EspEntry* out,int count){if(count<=0)return 0;out[0]=entry;return 1;}
    int GetWorldEsp(WorldMarker* out,int count){if(!worldCount || count<=0)return 0;out[0]=marker;return 1;}
}
bool ImGui_ImplWin32_Init(void*){return true;}
void ImGui_ImplWin32_Shutdown(){}
bool ImGui_ImplDX11_Init(ID3D11Device*,ID3D11DeviceContext*){return true;}
void ImGui_ImplDX11_Shutdown(){}
static int checks=0;
static void Check(bool value,const char* name){++checks;if(!value){std::fprintf(stderr,"FAIL: %s\n",name);std::exit(1);}}
static int Frame(){ImGui::NewFrame();GUI::RenderOverlay();int count=ImGui::GetBackgroundDrawList()->VtxBuffer.Size;ImGui::Render();return count;}
int main(){
    ImGui::CreateContext();GUI::g_bInit=true;
    auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize=ImVec2(800,600);io.DeltaTime=1.0f/60;io.Fonts->Build();
    Config::bWatermark=Config::bDebugOverlay=Config::bDrawFov=false;
    Config::bZombieEsp=true;Config::bZombieBoxShow=false;
    Config::bZombieName=Config::bZombieDist=Config::bZombieHp=Config::bZombiePct=false;
    Config::bZombieSnap=Config::bZombieSkeleton=Config::bZombieHeadDot=false;
    Mono::entry.headX=Mono::entry.footX=400;Mono::entry.headY=400;Mono::entry.footY=200;
    Mono::entry.onScreen=true;Mono::entry.hp=Mono::entry.maxHp=100;Mono::entry.dist=10;
    for(int style=0;style<3;++style){Config::iZombieBox=style;Check(Frame()==0,"master only never draws fallback box");}
    Config::bZombieBoxShow=true;for(int style=0;style<3;++style){Config::iZombieBox=style;Check(Frame()>0,"each enabled box renders");}
    Config::bZombieBoxShow=false;Config::bZombieSnap=true;
    for(int origin=0;origin<3;++origin){Config::iSnapFrom=origin;Check(Frame()>0,"snapline independently rendered");}
    Config::bZombieSnap=false;Config::bZombieEsp=false;
    Mono::worldCount=1;Mono::marker.kind=0;Mono::marker.x=Mono::marker.y=.5f;strcpy_s(Mono::marker.name,"Item");
    Config::bItemEsp=true;Config::bItemWeapons=true;Check(Frame()>0,"items independent of zombie master");
    Config::bItemWeapons=false;Check(Frame()==0,"category off hides stale marker immediately");
    Config::bItemWeapons=true;Config::bItemEsp=false;Check(Frame()==0,"item master off hides stale marker");
    Mono::marker.kind=4;Config::bPoiEsp=Config::bPoiHeli=true;Check(Frame()>0,"POI independent");
    Config::bPoiEsp=false;Check(Frame()==0,"POI master off hides stale marker");
    ImGui::DestroyContext();std::printf("PASS: %d real ImGui overlay checks\n",checks);
}
