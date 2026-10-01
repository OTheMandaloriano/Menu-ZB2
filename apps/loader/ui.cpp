#include "ui.h"
#include "../shared/widgets.h"
#include "../shared/cover.h"
#include "assets/IconsFontAwesome6.h"
#include <algorithm>
#include <cmath>
namespace {
using namespace UiTheme;
const char* GameState(const LoaderSnapshot& s){
    if(s.phase==LoaderPhase::Error)return "Precisa de atenção";
    if(s.phase==LoaderPhase::Success)return "Menu carregado";
    if(s.phase==LoaderPhase::Loading)return "Carregando";
    if(!s.pid)return "Jogo fechado";
    return s.sceneReady?"Partida pronta":"Aguardando partida";
}
void Go(LoaderUiState& s,LoaderPage page){s.navigation.Go(page);s.localMessage.clear();}
void Field(float y,const char* label,const char* value){Ui::Text(428,y,100,24,label,muted,caption);Ui::Text(532,y,188,24,value,text,caption,2);}
void Status(LoaderUiState& s){
    std::string message=s.localMessage.empty()?s.snapshot.message:s.localMessage,shown=message;
    while(!shown.empty()&&caption->CalcTextSizeA(caption->FontSize,10000,0,(shown+"...").c_str()).x>452*uiScale){size_t n=shown.size()-1;while(n&&(static_cast<unsigned char>(shown[n])&0xc0)==0x80)--n;shown.resize(n);}if(shown!=message)shown+="...";
    Ui::Text(176,460,452,24,shown.c_str(),s.snapshot.phase==LoaderPhase::Error?IM_COL32(240,158,153,255):muted,caption);
    ImGui::SetCursorPos(Ui::P(176,460));ImGui::InvisibleButton("##status",Ui::P(452,24));Ui::Hint(message.c_str());
    if(Ui::Button(640,456,100,28,"Detalhes",true, false,ICON_FA_CIRCLE_INFO))Go(s,LoaderPage::Diagnostics);
}
}
void DrawLoader(LoaderUiState& s){
    const auto& current=s.snapshot;bool busy=current.phase==LoaderPhase::Loading||current.phase==LoaderPhase::Checking;s.autoValue=current.autoInject;
    ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("DEADBLOCK",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar);
    Ui::Brand(20,16,"BIBLIOTECA");Ui::Text(476,20,212,24,current.version.c_str(),muted,caption,2);
    if(Ui::Button(712,20,28,28,ICON_FA_XMARK,true))s.close=true;
    auto* draw=ImGui::GetWindowDrawList();draw->AddLine(Ui::P(20,68),Ui::P(740,68),borderColor);draw->AddLine(Ui::P(156,84),Ui::P(156,480),borderColor);
    const LoaderPage pages[]={LoaderPage::Library,LoaderPage::Access,LoaderPage::Help};
    const char* labels[]={"Biblioteca","Meu acesso","Ajuda"};const char* navIcons[]={ICON_FA_LAYER_GROUP,ICON_FA_KEY,ICON_FA_CIRCLE_QUESTION};
    for(int i=0;i<3;++i)if(Ui::Button(16,96+i*48.f,128,36,labels[i],true,s.navigation.page==pages[i],navIcons[i]))Go(s,pages[i]);
    Ui::Text(24,414,112,20,"1 jogo disponível",muted,caption);Ui::Text(24,444,112,20,"Windows x64",muted,caption);
    if(s.navigation.page!=LoaderPage::Library){if(Ui::Button(176,84,104,28,"Voltar",true,false,ICON_FA_ARROW_LEFT)){s.navigation.Back();s.localMessage.clear();}}
    if(s.navigation.page!=s.lastRendered){s.lastRendered=s.navigation.page;s.pageOpacity=s.reducedMotion?1.f:.45f;}
    s.pageOpacity=s.reducedMotion?1.f:std::min(1.f,s.pageOpacity+ImGui::GetIO().DeltaTime/0.12f);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha,s.pageOpacity);
    switch(s.navigation.page){
    case LoaderPage::Library:
        Ui::Text(176,88,564,28,"Sua biblioteca",text,heading);Ui::Text(176,124,564,20,"Selecione o jogo para ver seu acesso e iniciar.",muted,regular);
        Cover::Draw(176,160,564,216);Ui::Panel(176,376,564,68);
        Ui::Text(192,386,320,24,"Zumbi Blocks 2",text,heading);Ui::Text(192,414,320,18,current.licensed?"Acesso ativo neste computador":"Ativação necessária",muted,caption);
        if(Ui::Button(576,392,148,36,"Selecionar",false,true,ICON_FA_ARROW_RIGHT))Go(s,LoaderPage::Game);
        break;
    case LoaderPage::Game:
        Cover::Draw(176,128,224,304);Ui::Panel(412,128,328,304);Ui::Text(428,144,296,28,"Zumbi Blocks 2",text,heading);
        if(!current.licensed){
            Ui::Wrapped(428,184,296,"Ative o acesso recebido da equipe para carregar o menu.");
            Ui::Text(428,240,296,20,"Licença de acesso",muted,regular);
            bool enter=Ui::Input(428,272,296,40,"##license","Cole sua chave",s.license,sizeof(s.license),ImGuiInputTextFlags_EnterReturnsTrue,ICON_FA_KEY);
            ImGui::BeginDisabled(!s.license[0]||busy);if(Ui::Button(428,328,296,40,busy?"Verificando...":"Ativar acesso",false,true,ICON_FA_BOLT)||enter){s.activate=true;s.localMessage.clear();}ImGui::EndDisabled();
            if(Ui::Button(428,380,144,32,"Abrir arquivo",false,false,ICON_FA_FOLDER_OPEN))s.importLicense=true;
            if(Ui::Button(584,380,140,32,"Copiar ID",false,false,ICON_FA_COPY)&&!current.device.empty()){ImGui::SetClipboardText(current.device.c_str());s.localMessage="ID copiado. Envie-o à equipe.";}
        }else{
            Field(188,"Estado",GameState(current));Field(224,"Validade",current.expiry.c_str());
            if(Ui::Toggle(428,280,296,"AUTO-INJECT",s.autoValue))s.autoChanged=true;
            ImGui::BeginDisabled(busy||!current.pid||!current.gameVerified||current.phase==LoaderPhase::Success);
            const char* action=current.phase==LoaderPhase::Success?"Menu carregado":busy?"Carregando...":!current.gameVerified?"Aguardando o jogo":"Carregar menu";
            if(Ui::Button(428,336,296,40,action,false,true,ICON_FA_BOLT))s.load=true;ImGui::EndDisabled();
            Ui::Wrapped(428,392,296,current.phase==LoaderPhase::Success?"Use INSERT no jogo para abrir o menu.":"Entre em uma partida. O programa verifica quando ela está pronta.");
        }break;
    case LoaderPage::Access:
        Ui::Text(176,128,564,28,"Meu acesso",text,heading);Ui::Wrapped(176,172,564,"Copie o ID para solicitar uma licença. Seus dados ficam neste computador.");
        Ui::Device(176,224,392,current.device);if(Ui::Button(580,224,160,36,"Copiar ID",false,false,ICON_FA_COPY)&&!current.device.empty()){ImGui::SetClipboardText(current.device.c_str());s.localMessage="ID completo copiado.";}
        Ui::Panel(176,280,564,80);Ui::Text(192,292,220,24,"Situação",muted,regular);Ui::Text(416,292,308,24,current.licensed?"Licença ativa":"Sem ativação",text,regular,2);
        Ui::Text(192,324,180,20,"Validade",muted,caption);Ui::Text(372,324,352,20,current.licensed?current.expiry.c_str():"Solicite à equipe",text,caption,2);
        if(Ui::Button(176,388,236,36,current.licensed?"Ver carregamento":"Ativar acesso",false,true,ICON_FA_GAMEPAD))Go(s,LoaderPage::Game);
        if(Ui::Button(428,388,180,36,"Créditos",false,false,ICON_FA_USERS))Go(s,LoaderPage::Credits);break;
    case LoaderPage::Help:
        Ui::Text(176,128,564,28,"Como começar",text,heading);
        Ui::Wrapped(176,180,550,"1. Em Meu acesso, copie o ID e envie para a equipe.\n\n2. Extraia o ZIP ativado recebido e abra o programa que veio nele.\n\n3. Abra Zumbi Blocks 2 e entre em uma partida.\n\n4. Abra o jogo na Biblioteca e aguarde Menu carregado. Use INSERT.",text);
        if(Ui::Toggle(176,370,340,"Reduzir movimento",s.reducedMotion))Ui::SetReducedMotion(s.reducedMotion);
        if(Ui::Button(176,416,180,28,"Créditos",false,false,ICON_FA_USERS))Go(s,LoaderPage::Credits);break;
    case LoaderPage::Credits:
        Ui::Panel(176,132,564,276);Ui::Text(196,164,524,32,"DEADBLOCK",text,heading,1);Ui::Text(196,216,524,24,"OTheMandaloriano & Equipe",text,regular,1);
        Ui::Text(196,256,524,20,"Zumbi Blocks 2 / Windows x64",muted,caption,1);Ui::Text(196,292,524,20,(current.version+" · Preview").c_str(),muted,caption,1);
        Ui::Text(196,340,524,20,"Arte de capa original. Projeto independente do jogo.",muted,caption,1);break;
    case LoaderPage::Diagnostics:{
        Ui::Text(176,128,564,28,"Detalhes do estado atual",text,heading);
        ImGui::SetCursorPos(Ui::P(176,180));ImGui::BeginChild("diagnostic-details",Ui::P(564,164));
        const auto message=s.localMessage.empty()?current.message:s.localMessage;ImGui::TextWrapped("%s",message.c_str());ImGui::EndChild();
        Ui::Text(176,364,564,20,"Envie os detalhes à equipe se o problema continuar.",muted,regular);
        if(Ui::Button(176,408,220,36,"Copiar detalhes",false,false,ICON_FA_COPY)){auto report="DEADBLOCK "+current.version+"\nPID: "+std::to_string(current.pid)+"\n"+message;ImGui::SetClipboardText(report.c_str());}break;}
    }
    ImGui::PopStyleVar();
    if(busy){float angle=s.reducedMotion?0.f:static_cast<float>(ImGui::GetTime()*4.0);draw->PathArcTo(Ui::P(696,34),6*uiScale,angle,angle+4.7f,24);draw->PathStroke(IM_COL32(90,140,250,255),0,2*uiScale);}
    draw->AddLine(Ui::P(176,450),Ui::P(740,450),borderColor);Status(s);ImGui::End();
}
