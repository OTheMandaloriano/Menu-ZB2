#include "ui.h"
#include "../shared/widgets.h"
#include "assets/IconsFontAwesome6.h"
#include "../shared/resources.h"
namespace {
using namespace UiTheme;
void Status(const LoaderUiState& state){
    std::string message=state.localMessage.empty()?state.snapshot.message:state.localMessage;
    if(state.snapshot.phase==LoaderPhase::Activation&&state.localMessage.empty())return;
    std::string shown=message;
    while(!shown.empty()&&caption->CalcTextSizeA(caption->FontSize,10000,0,(shown+"...").c_str()).x>400*uiScale){size_t n=shown.size()-1;while(n&&(static_cast<unsigned char>(shown[n])&0xc0)==0x80)--n;shown.resize(n);}
    if(shown!=message)shown+="...";
    Ui::Text(20,244,400,12,shown.c_str(),state.snapshot.phase==LoaderPhase::Error?IM_COL32(235,153,150,255):muted,caption);
    ImGui::SetCursorPos(Ui::P(20,240));ImGui::InvisibleButton("##status",Ui::P(400,20));Ui::Hint(message.c_str());
}
void Row(float y,const char* icon,const char* label,const char* value){Ui::Text(32,y,16,16,icon,muted,regular,1);Ui::Text(56,y,100,16,label,muted,caption);Ui::Text(168,y,240,16,value,text,caption,2);}
}
void DrawLoader(LoaderUiState& state){
    const auto& current=state.snapshot;const bool busy=current.phase==LoaderPhase::Checking||current.phase==LoaderPhase::Loading;
    state.autoValue=current.autoInject;
    ImGui::SetNextWindowPos(ImVec2(0,0));ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("ZB2 Menu",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar);
    Ui::Text(20,12,260,32,state.help?"Como usar":"ZB2 Menu",text,heading);
    if(Ui::Button(360,16,24,24,ICON_FA_CIRCLE_INFO,true)){state.about=!state.about;state.help=false;}Ui::Hint("Sobre o menu");
    if(Ui::Button(396,16,24,24,ICON_FA_XMARK,true))state.close=true;
    if(state.about){
        if(state.credits){
            Ui::Text(20,56,400,24,u8"ZB2 Pro Menu — Créditos & Equipe",text,regular);
            Ui::Text(20,92,400,20,"WeFagundes & Equipe de Modding",text,regular);
            Ui::Text(20,124,400,16,"Unity Mono / DirectX 11 Hook (x64)",muted,caption);
            Ui::Text(20,152,400,16,(current.version+u8" · Alpha Build").c_str(),muted,caption);
            Ui::Text(20,180,400,16,current.phase==LoaderPhase::Success?"Menu carregado":"Aguardando carregamento",text,caption);
            if(Ui::Button(20,208,194,28,"Voltar ao menu"))state.credits=false;
            if(Ui::Button(226,208,194,28,u8"Dependências",true))ImGui::OpenPopup("Dependencias");
            if(ImGui::BeginPopup("Dependencias")){ImGui::BeginChild("avisos",Ui::P(340,156));static const auto notices=AppResources::Read(205);ImGui::TextWrapped("%s",notices.c_str());ImGui::EndChild();ImGui::EndPopup();}
        }else if(state.help){
            Ui::Wrapped(20,64,400,u8"1. Copie seu ID e solicite a licença à equipe.\n2. Abra o arquivo recebido e ative o acesso.\n3. Com AUTO-INJECT ligado, o menu carrega quando a partida estiver pronta.\n\nPrefere acionar? Use Injetar agora. No jogo, abra o menu com INSERT.");
            if(Ui::Button(20,204,400,28,"Voltar aos dados",false,false,ICON_FA_ARROW_LEFT))state.help=false;
        }else{
            Ui::Device(20,64,256,current.device);
            if(Ui::Button(288,64,132,36,"Copiar ID",false,false,ICON_FA_COPY)&&!current.device.empty()){ImGui::SetClipboardText(current.device.c_str());state.localMessage="ID completo copiado.";}
            Ui::Text(20,120,100,16,u8"Licença",muted,caption);Ui::Text(132,120,288,16,current.licensed?current.expiry.c_str():u8"Não ativada",text,caption,2);
            Ui::Text(20,152,100,16,u8"Versão",muted,caption);Ui::Text(132,152,288,16,current.version.c_str(),text,caption,2);
            if(Ui::Button(20,196,194,36,u8"Créditos",false,false,ICON_FA_USERS))state.credits=true;
            if(Ui::Button(226,196,194,36,"Como usar",false,false,ICON_FA_CIRCLE_QUESTION))state.help=true;
        }
    }else if(!current.licensed){
        Ui::Text(20,64,400,16,u8"Licença de acesso",muted,caption);
        std::string before=state.license;
        bool enter=Ui::Input(20,92,400,40,"##license",u8"Cole sua licença ou abra o arquivo",state.license,sizeof(state.license),ImGuiInputTextFlags_EnterReturnsTrue,ICON_FA_KEY);
        if(before!=state.license)state.localMessage.clear();
        ImGui::BeginDisabled(state.license[0]=='\0'||busy);
        if(Ui::Button(20,144,400,40,busy?"Verificando...":"Ativar",false,true,ICON_FA_BOLT)||enter){state.activate=true;state.localMessage.clear();}ImGui::EndDisabled();
        if(Ui::Button(20,196,194,28,"Abrir arquivo",false,false,ICON_FA_FOLDER_OPEN))state.importLicense=true;
        if(Ui::Button(226,196,194,28,"Copiar ID",false,false,ICON_FA_COPY)&&!current.device.empty()){ImGui::SetClipboardText(current.device.c_str());state.localMessage=u8"ID copiado. Envie-o à equipe.";}
    }else{
        Ui::Panel(20,56,400,80);
        Row(64,ICON_FA_DESKTOP,"Jogo",current.pid?(current.sceneReady?"Cena pronta":"Inicializando") :"Aguardando");
        Row(88,ICON_FA_KEY,"Expira em",current.expiry.c_str());Row(112,ICON_FA_CODE_BRANCH,u8"Versão",current.version.c_str());
        if(Ui::Toggle(20,148,400,"AUTO-INJECT",state.autoValue))state.autoChanged=true;
        ImGui::BeginDisabled(busy||!current.pid||current.phase==LoaderPhase::Success);
        if(Ui::Button(20,192,400,40,current.phase==LoaderPhase::Success?"Menu carregado":busy?"Carregando...":"Injetar agora",false,true,current.phase==LoaderPhase::Success?ICON_FA_CHECK:ICON_FA_BOLT))state.load=true;
        ImGui::EndDisabled();
    }
    Status(state);ImGui::End();
}
