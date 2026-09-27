#include "../gui.cpp"
#include <cstdlib>
#include <fstream>
namespace Log { const char* GetDir(){return ".";} void Infof(const char*,...){} }
namespace Mono {
    static State state; static EspEntry entry; static WorldMarker marker; static int worldCount=0;
    const State& Get(){return state;}
    int GetEsp(EspEntry* out,int count){if(count<=0)return 0;out[0]=entry;return 1;}
    int GetWorldEsp(WorldMarker* out,int count){if(!worldCount || count<=0)return 0;out[0]=marker;return 1;}
    static DistantMarker distant;static int distantCount=0;
    int GetDistantEsp(DistantMarker* out,int count){if(!distantCount || count<=0)return 0;out[0]=distant;return 1;}
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
    Mono::worldCount=0;Mono::entry.onScreen=false;Config::bZombieEsp=true;
    Mono::distantCount=1;Mono::distant={.5f,.5f,210,0,0};Config::bZombieBoxShow=true;
    Config::fEspDistance=200;Check(Frame()==0,"distant records excluded beyond slider");
    Config::fEspDistance=220;Check(Frame()>0,"slider includes dormant record beyond 75m");
    Config::bZombieBoxShow=false;Check(Frame()==0,"dormant record respects box off");
    Config::bZombieDist=true;Check(Frame()>0,"dormant distance independent of box");
    Config::bZombieDist=false;Mono::distantCount=0;Mono::entry.onScreen=true;Mono::entry.dist=210;Config::bZombieBoxShow=true;
    Config::fEspDistance=200;Check(Frame()==0,"stale active snapshot filtered by new smaller limit");
    Config::fItemRadius=35;Config::fPoiRadius=280;
    Config::iNoClipKey=118;Config::iMagnetKey=119;
    Config::iItemFilter0=123;Config::iItemFilter3=1<<19;Config::iPoiFilter=1<<17;
    Check(GUI::SaveConfig("independent-ranges-test"),"save separate radii");
    Config::fItemRadius=80;Config::fPoiRadius=90;GUI::LoadConfig("independent-ranges-test");
    Check(Config::fItemRadius==35 && Config::fPoiRadius==280,"preset restores distinct radii");
    Check(Config::iNoClipKey==118 && Config::iMagnetKey==119,"preset restores custom NoClip and Magnet keys");
    Check(Config::iItemFilter0==123 && Config::iItemFilter3==(1<<19) && Config::iPoiFilter==(1<<17),"preset preserves individual item and POI masks");
    Config::fItemRadius=10;Check(Config::fPoiRadius==280,"item slider independent");
    Config::fPoiRadius=20;Check(Config::fItemRadius==10,"POI slider independent");
    Config::bZombieEsp=false;Mono::worldCount=1;Mono::marker.kind=10;Mono::marker.distance=100;
    Config::bPoiEsp=Config::bPoiFire=true;
    Check(Frame()==0,"reduced POI range filters stale marker immediately");
    Config::fPoiRadius=150;Check(Frame()>0,"POI independent of short item range");
    ImGui::DestroyContext();std::printf("PASS: %d real ImGui overlay checks\n",checks);
}
