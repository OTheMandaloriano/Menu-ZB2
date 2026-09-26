#include <fstream>
#include <functional>
#include <sstream>
#include "../gui.cpp"
#include "software_renderer.h"
namespace Log { const char* GetDir(){return TestPlatform::directory.c_str();}void Infof(const char*,...){} }
namespace Mono { static State state;static std::vector<EspEntry> entries;const State& Get(){return state;}int GetEsp(EspEntry* out,int count){int total=std::min(count,static_cast<int>(entries.size()));std::copy_n(entries.data(),total,out);return total;} int GetWorldEsp(WorldMarker*,int){return 0;} }
bool ImGui_ImplWin32_Init(void*){return true;}void ImGui_ImplWin32_Shutdown(){}
bool ImGui_ImplDX11_Init(ID3D11Device*,ID3D11DeviceContext*){return true;}void ImGui_ImplDX11_Shutdown(){}

using namespace EspLayout;
static int assertions=0,uiDrags=0;
static std::string output;
static void Require(bool ok,const char* text){++assertions;if(!ok){std::fprintf(stderr,"FAIL integration: %s (assertion %d)\n",text,assertions);std::exit(1);}}
static ImVec2 Center(Rect r){return ImVec2((r.min.x+r.max.x)*0.5f,(r.min.y+r.max.y)*0.5f);}
static bool Near(float a,float b){return std::fabs(a-b)<0.04f;}
static bool Same(Rect a,Rect b){return Near(a.min.x,b.min.x)&&Near(a.min.y,b.min.y)&&Near(a.max.x,b.max.x)&&Near(a.max.y,b.max.y);}
static std::string Read(const std::string& path){std::ifstream file(path,std::ios::binary);return std::string(std::istreambuf_iterator<char>(file),{});}
static void Write(const std::string& path,const std::string& value){std::ofstream file(path,std::ios::binary);file<<value;Require(file.good(),"fixture write");}
static void Start(int width,int height){auto& io=ImGui::GetIO();io.DisplaySize=ImVec2(static_cast<float>(width),static_cast<float>(height));io.DeltaTime=1.0f/60;ImGui::NewFrame();}
static void Finish(const char* image=nullptr){ImGui::Render();if(image)Require(RenderPpm(output+"/"+image),"render ImGui draw data");}

static void ConfigurationTests(){
    std::filesystem::create_directories(TestPlatform::directory+"/configs");
    std::string dir=TestPlatform::directory+"/configs/";
    std::string legacy="{\n \"_v\":3,\"iCfgVer\":2,\"iSideN\":0,\"iSideD\":1,\"iSideH\":3,\"iSideP\":2,\"iAlinN\":2,\"iAlinD\":0,\"iAlinH\":1,\"iAlinP\":1,\"fNameX\":-60,\"fNameY\":-60,\"fDistX\":60,\"fDistY\":60,\"fHpX\":60,\"fHpY\":-60,\"fPctX\":-60,\"fPctY\":60,\"fPropN\":0.05,\"fLayoutOffset\":8,\"bWatermark\":false,\"fCamFov\":91.25,\"szItemSearch\":\"nome \\\"A\\\"\\\\B\"\n}";
    Write(dir+"legacy.json",legacy);GUI::LoadConfig("legacy");
    Require(Config::iCfgVer==4 && Config::iSideH==Right && Config::iSideP==Left,"v3 migration preserves sides despite stale iCfgVer");
    Require(Config::fAlongN==1 && Config::fAlongD==0 && Config::fNameX==0 && Config::fDistY==0 && Config::fHpX==0 && Config::fPctY==0 && Config::fPropN==0 && Config::fLayoutOffset==4,"migration discards corrupt offsets and doubles margins no longer");
    Require(!Config::bWatermark && Config::fCamFov==91.25f && std::string(Config::szItemSearch)=="nome \"A\"\\B","unrelated known settings and escaped strings preserved");
    Require(Read(dir+"legacy.json.layout-v4-000.bak")==legacy,"backup is byte-identical to original");
    ConfigJson::Document document;Require(document.Parse(Read(dir+"legacy.json")) && document.Version()==4,"migration writes valid v4 JSON");
    GUI::LoadConfig("legacy");Require(!std::filesystem::exists(dir+"legacy.json.layout-v4-001.bak") && Config::fAlongN==1,"v4 reload does not repeat migration");
    std::string legacy2="{\"_v\":2,\"iNameA\":2,\"iDistA\":6,\"iHpA\":0,\"iPctA\":5,\"fNameY\":-16}";
    Write(dir+"legacy2.json",legacy2);GUI::LoadConfig("legacy2");Require(Config::iSideN==Top && Config::fAlongN==1 && Config::iSideD==Bottom && Config::fAlongD==0 && Config::iSideH==Left,"v2 anchors migrate to external sides and alignments");
    GUI::WriteLayout(Preset(2));Config::fCamFov=93;
    for(const char* value : {"{\"_v\":99,\"fCamFov\":110}","{\"_v\":4,\"fCamFov\":110,\"fAlongN\":\"wrong\"}","{\"_v\":4,", "{\"_v\":4,\"fAlongN\":NaN}"}){
        Write(dir+"bad.json",value);GUI::LoadConfig("bad");Require(Config::fCamFov==93 && Config::iSideH==Right && Read(dir+"bad.json")==value,"invalid/future config cannot partially change state or disk");
    }
    Write(dir+"copyfail.json",legacy);TestPlatform::failCopy=true;GUI::LoadConfig("copyfail");TestPlatform::failCopy=false;Require(Read(dir+"copyfail.json")==legacy,"failed backup preserves original file");
    Write(dir+"movefail.json",legacy);TestPlatform::failMove=true;GUI::LoadConfig("movefail");TestPlatform::failMove=false;Require(Read(dir+"movefail.json")==legacy && Read(dir+"movefail.json.layout-v4-000.bak")==legacy,"failed atomic replace preserves original and backup");
    GUI::WriteLayout(Preset(1));strncpy_s(GUI::s_cfgName,"draft",_TRUNCATE);Require(GUI::SaveConfig("draft"),"initial config save");std::string saved=Read(dir+"draft.json");
    Reset(GUI::s_editor,Preset(2));GUI::s_editorReady=GUI::s_editorVisible=true;GUI::s_editor.dirty=true;
    TestPlatform::failMove=true;Require(!GUI::SaveLayoutDraft(),"draft save reports publishing failure");TestPlatform::failMove=false;
    Require(Config::iSideH==Left && GUI::s_editor.draft.items[Health].side==Right && GUI::s_editor.dirty && Read(dir+"draft.json")==saved,"failed save retains committed state and editable draft");
    Require(GUI::SaveLayoutDraft() && Config::iSideH==Right && !GUI::s_editor.dirty,"successful save commits draft");
    GUI::CloseLayoutEditor();GUI::LoadConfig("draft");Require(Config::iSideH==Right,"saved drag survives reopening");
    Require(!GUI::SaveConfig("../outside") && !GUI::SaveConfig("bad/name") && !GUI::SaveConfig(""),"invalid preset paths refused");
    GUI::ApplyFactoryDefaults();Require(Config::iSideH==Left && Config::iSideP==Right && Config::fAlongN==0.5f && Config::iCfgVer==4,"reset restores full new model");
}

static void EditorFrame(ImVec2 mouse,bool down,const char* image=nullptr,ImVec2 position=ImVec2(24,20),ImVec2 size=ImVec2(860,700)){
    auto& io=ImGui::GetIO();io.AddMousePosEvent(mouse.x,mouse.y);io.AddMouseButtonEvent(0,down);
    Start(1160,800);ImGui::SetNextWindowPos(position);ImGui::SetNextWindowSize(size);
    ImGui::Begin("ZB2 | Editor de layout",nullptr,ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoSavedSettings);
    GUI::DrawLayoutEditor();ImGui::End();Finish(image);
}

static void GuiInteractionTests(){
    GUI::WriteLayout(Preset(1));GUI::CloseLayoutEditor();GUI::s_cfgStatus[0]=0;
    EditorFrame(ImVec2(-100,-100),false);EditorFrame(ImVec2(-100,-100),false);
    Require(GUI::s_previewResult.valid,"real editor initial layout fits");
    for(int id=0;id<Count;++id)for(int side : {Right,Left,Top,Bottom}){
        auto press=Center(GUI::s_previewResult.items[id].rect);
        EditorFrame(press,false);EditorFrame(press,true);
        Require(GUI::s_editor.active==id,"real ImGui mouse event captures exact object");
        Rect b=GUI::s_previewBox;ImVec2 target=Center(b);
        if(side==Top)target.y=b.min.y-35;if(side==Bottom)target.y=b.max.y+35;if(side==Left)target.x=b.min.x-50;if(side==Right)target.x=b.max.x+50;
        EditorFrame(target,true);
        Require(GUI::s_editor.candidateValid && GUI::s_editor.candidate.items[id].side==side,"real ImGui move selects side");
        auto candidate=GUI::s_previewResult;
        Require(GUI::VisibleLayout().items[id].side==side,"overlay sees same live draft candidate");
        EditorFrame(target,false);
        Require(GUI::s_editor.active==-1 && GUI::s_editor.draft.items[id].side==side,"real ImGui release commits without lock");
        for(int element=0;element<Count;++element)Require(Same(candidate.items[element].rect,GUI::s_previewResult.items[element].rect),"real UI candidate equals release geometry");
        ++uiDrags;
    }
    int side=GUI::s_editor.draft.items[Name].side;
    for(int i=0;i<25;++i)EditorFrame(ImVec2(-100,-100),false);
    Require(GUI::s_editor.draft.items[Name].side==side,"idle frames cannot reapply a preset");
    GUI::CloseLayoutEditor();Require(GUI::VisibleLayout().items[Health].side==Left,"closing discards unsaved preview and releases capture");
    EditorFrame(ImVec2(-100,-100),false);
    auto press=Center(GUI::s_previewResult.items[Name].rect);
    EditorFrame(press,false);EditorFrame(press,true);
    auto& io=ImGui::GetIO();io.AddKeyEvent(ImGuiKey_Escape,true);EditorFrame(press,true);Require(GUI::s_editor.active==-1,"Escape releases real UI capture");io.AddKeyEvent(ImGuiKey_Escape,false);EditorFrame(press,false);
    EditorFrame(press,true);io.AddFocusEvent(false);EditorFrame(press,true);Require(GUI::s_editor.active==-1,"focus loss releases real UI capture");io.AddFocusEvent(true);EditorFrame(press,false);
    EditorFrame(ImVec2(-100,-100),false,nullptr,ImVec2(240,65),ImVec2(440,690));
    Require(GUI::s_previewResult.valid,"narrow responsive editor fits default layout");
    GUI::CloseLayoutEditor();
    Config::bMenuOpen=true;
    for(int i=0;i<5;++i){Start(1160,800);GUI::Render();GUI::RenderOverlay();Finish();}
    Config::bMenuOpen=false;Start(1160,800);GUI::Render();Finish();Require(!GUI::s_editorReady,"closing menu resets editor lifecycle");Config::bMenuOpen=true;
}

static void Text(ImDrawList* draw,float x,float y,const char* value,float size=16,ImU32 color=IM_COL32(220,231,248,255)){draw->AddText(ImGui::GetFont(),size,ImVec2(x,y),color,value);}
static void Card(ImDrawList* draw,Rect rect,const Model& model,const char* title,const char* subtitle,float height=112){
    Text(draw,rect.min.x+14,rect.min.y+12,title,17);
    Text(draw,rect.min.x+14,rect.min.y+37,subtitle,13,IM_COL32(145,169,200,255));
    Rect canvas={{rect.min.x+10,rect.min.y+63},{rect.max.x-10,rect.max.y-12}};
    ImVec2 center=Center(canvas);Rect b={{center.x-height*0.3f,center.y-height*0.5f},{center.x+height*0.3f,center.y+height*0.5f}};
    auto content=GUI::LayoutContent("Zombie",34,87,100);auto style=GUI::LayoutStyle(content.health);
    auto geometry=Resolve(model,b,canvas,content,style);Require(geometry.valid,"preview image uses valid production geometry");
    DrawPreview(draw,canvas,geometry,model,style,-1,false,true);Draw(draw,geometry,content,style);
}

static void Gallery(){
    Start(1120,770);auto* draw=ImGui::GetBackgroundDrawList();
    Text(draw,26,19,"BARRA DE VIDA | quatro lados, entidade preservada",24);
    Text(draw,26,53,"Render do codigo corrigido - margem de 4 px - HP 87% - arraste com orientacao automatica",15);
    const char* titles[]={"01 | Barra a esquerda","02 | Barra a direita","03 | Barra acima","04 | Barra abaixo"};
    const char* subtitles[]={"Vertical / preenchimento de baixo para cima","Vertical / nome e distancia permanecem externos","Horizontal / nome ocupa a proxima faixa","Horizontal / distancia ocupa a proxima faixa"};
    for(int i=0;i<4;++i){float x=18+(i%2)*552,y=88+(i/2)*333;Card(draw,{{x,y},{x+532,y+320}},Preset(i+1),titles[i],subtitles[i]);}
    Finish("01_quatro_lados.ppm");
    Start(1120,1030);draw=ImGui::GetBackgroundDrawList();
    Text(draw,26,19,"ALINHAMENTOS | variacoes do mesmo motor de layout",24);
    Text(draw,26,53,"Textos medidos antes de posicionar. Colisoes afastam o grupo da entidade.",15);
    const char* labels[]={"05 | Tudo a esquerda","06 | Tudo a direita","07 | Tudo acima","08 | Tudo abaixo","09 | Cantos da esquerda","10 | Cantos da direita"};
    for(int i=0;i<6;++i){float x=18+(i%2)*552,y=88+(i/2)*309;Card(draw,{{x,y},{x+532,y+295}},Preset(i+5),labels[i],i<4?"Vida na faixa interna / textos em grupo":"Nome e distancia alinhados a extremidade",96);}
    Finish("02_alinhamentos.ppm");
    for(int i=1;i<=10;++i){Start(520,390);draw=ImGui::GetBackgroundDrawList();char label[80],path[80];std::snprintf(label,sizeof(label),"Preset %02d | %s",i,SideName(Preset(i).items[Health].side));Card(draw,{{4,4},{516,386}},Preset(i),label,"Mesmo Resolve / Draw usados pelo jogo",150);std::snprintf(path,sizeof(path),"preset_%02d.ppm",i);Finish(path);}
    Model row=Preset(7);row.horizontalText=true;Start(720,410);draw=ImGui::GetBackgroundDrawList();Card(draw,{{4,4},{716,406}},row,"11 | Informacoes em linha no topo","Nome, porcentagem e distancia sem sobreposicao",112);Finish("03_linha_no_topo.ppm");
}

static void OverlayEvidence(){
    GUI::CloseLayoutEditor();GUI::WriteLayout(Preset(2));Config::bWatermark=false;Config::bDebugOverlay=false;Config::bZombieEsp=true;
    Start(1100,470);auto* draw=ImGui::GetBackgroundDrawList();
    Text(draw,25,18,"OVERLAY | tamanho do box muda; a margem continua em pixels",23);
    Text(draw,25,54,"GUI::RenderOverlay com entidades simuladas. Nao e uma captura do jogo em execucao.",14);
    Mono::entries.clear();
    const float heights[]={24,60,112,180};
    for(int i=0;i<4;++i){float x=140+i*270,h=heights[i];Mono::EspEntry entry={};entry.headX=entry.footX=x;entry.headY=470-(260-h*0.5f);entry.footY=470-(260+h*0.5f);entry.hp=87;entry.maxHp=100;entry.dist=34;entry.onScreen=true;std::strcpy(entry.name,"Zombie");Mono::entries.push_back(entry);char label[80];std::snprintf(label,sizeof(label),"Box %.0f px | gap 4 px",h);Text(draw,x-84,385,label,14);}
    GUI::RenderOverlay();Finish("04_overlay_tamanhos.ppm");Mono::entries.clear();
    GUI::WriteLayout(Preset(1));GUI::CloseLayoutEditor();GUI::s_cfgStatus[0]=0;GUI::s_previewBoxHeight=112;
    EditorFrame(ImVec2(-100,-100),false);EditorFrame(ImVec2(-100,-100),false,"05_editor_real.ppm");
    EditorFrame(ImVec2(-100,-100),false,nullptr,ImVec2(28,20),ImVec2(440,740));EditorFrame(ImVec2(-100,-100),false,"06_editor_estreito.ppm",ImVec2(28,20),ImVec2(440,740));
}

static void DragEvidence(){
    GUI::CloseLayoutEditor();GUI::WriteLayout(Preset(1));GUI::s_cfgStatus[0]=0;
    EditorFrame(ImVec2(-100,-100),false);EditorFrame(ImVec2(-100,-100),false,"arraste_01_inicio.ppm");
    ImVec2 press=Center(GUI::s_previewResult.items[Health].rect);EditorFrame(press,false);EditorFrame(press,true);
    Rect b=GUI::s_previewBox;ImVec2 target(b.max.x+32,Center(b).y);
    EditorFrame(target,true,"arraste_02_direita.ppm");EditorFrame(target,false,"arraste_03_solto.ppm");
    press=Center(GUI::s_previewResult.items[Health].rect);EditorFrame(press,false);EditorFrame(press,true);target=ImVec2(Center(b).x,b.max.y+26);
    EditorFrame(target,true,"arraste_04_base.ppm");EditorFrame(target,false,"arraste_05_solto_base.ppm");
    press=Center(GUI::s_previewResult.items[Name].rect);EditorFrame(press,false);EditorFrame(press,true);target=Center(b);
    EditorFrame(target,true,"arraste_06_invalido.ppm");EditorFrame(target,false,"arraste_07_restaurado.ppm");
}

int main(int argc,char** argv){
    if(argc!=2){std::fprintf(stderr,"Usage: gui_harness OUTPUT_DIR\n");return 2;}
    output=std::filesystem::absolute(argv[1]).string();std::filesystem::create_directories(output);
    TestPlatform::directory=output+"/config-fixtures";
    std::filesystem::remove_all(TestPlatform::directory);
    GUI::Initialize(nullptr,nullptr,nullptr);auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.Fonts->Build();io.Fonts->SetTexID(reinterpret_cast<void*>(1));io.BackendFlags|=ImGuiBackendFlags_RendererHasVtxOffset;
    ConfigurationTests();GuiInteractionTests();Gallery();OverlayEvidence();DragEvidence();
    std::printf("PASS integration: %d assertions; %d real ImGui drag transitions; migration/backup/save/reopen and images\n",assertions,uiDrags);
    GUI::Shutdown();
}
