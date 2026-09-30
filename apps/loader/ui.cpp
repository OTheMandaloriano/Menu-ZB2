#include "ui.h"
#include "../shared/theme.h"
#include "../shared/widgets.h"
#include "assets/IconsFontAwesome6.h"
#include "../../imgui/imgui_internal.h"
#include <algorithm>
#include <cfloat>

namespace {
using namespace UiTheme;
using Ui::P;
using Ui::Button;
using Ui::Hint;
void Status(const LoaderUiState& state){
    std::string message=state.localMessage.empty()?state.snapshot.message:state.localMessage;
    if(state.snapshot.phase==LoaderPhase::Activation && state.localMessage.empty())return;
    std::string shown=message;auto* face=caption;
    while(!shown.empty() && face->CalcTextSizeA(face->FontSize,10000,0,(shown+"...").c_str()).x>304*uiScale){size_t n=shown.size()-1;while(n && (static_cast<unsigned char>(shown[n])&0xc0)==0x80)--n;shown.resize(n);}
    if(shown!=message)shown+="...";
    auto color=state.snapshot.phase==LoaderPhase::Error?IM_COL32(224,150,148,255):muted;
    Ui::Text(48,360,304,20,shown.c_str(),color,caption,1);
    ImGui::SetCursorPos(P(48,360));ImGui::InvisibleButton("##status",P(304,20));
    if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)){ImGui::BeginTooltip();ImGui::PushTextWrapPos(330*uiScale);ImGui::TextUnformatted(message.c_str());ImGui::PopTextWrapPos();ImGui::EndTooltip();}
}
void Row(float y,const char* icon,const char* label,const char* value,ImU32 color=text){
    Ui::Text(56,y,16,28,icon,muted,regular,1);Ui::Text(84,y,88,28,label,muted,caption);
    Ui::Text(168,y,176,28,value,color,caption,2);
}
}

void DrawLoader(LoaderUiState& state){
    const auto& current=state.snapshot;
    if(state.lastPid!=current.pid){state.inMap=false;state.lastPid=current.pid;}
    const bool busy=current.phase==LoaderPhase::Checking||current.phase==LoaderPhase::Loading;
    ImGui::SetNextWindowPos(ImVec2(0,0));ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("ZB2 Menu",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
    Ui::Text(24,16,200,28,"ZB2",muted,caption);
    if(Button(312,16,28,28,ICON_FA_CIRCLE_INFO,true))state.about=!state.about;Hint("Sobre o ZB2 Menu");
    if(Button(356,16,28,28,ICON_FA_XMARK,true))state.close=true;Hint("Fechar");
    if(state.about){
        Ui::Text(32,64,336,40,state.help?"Como usar":"ZB2 Menu",text,heading,1);
        if(state.help){
            Ui::Wrapped(48,132,304,u8"1. Copie o ID e envie à equipe.\n\n2. Receba seu arquivo .zb2license, abra-o e clique em Ativar.\n\n3. Entre no mapa, marque a confirmação e carregue o menu. Use INSERT no jogo.");
            if(Button(48,320,304,32,"Voltar aos dados",false,true))state.help=false;
        }else{
            const auto version=current.version.empty()?std::string("1.2-local"):current.version;
            Ui::Text(48,108,304,18,version.c_str(),muted,caption,1);
            Ui::Text(48,132,304,16,"ID deste computador",muted,caption);
            Ui::Device(48,152,304,current.device);
            if(Button(48,212,304,24,"Copiar ID completo",true)&&!current.device.empty()){ImGui::SetClipboardText(current.device.c_str());state.localMessage="ID completo copiado.";}
            Ui::Text(48,244,100,20,"Jogo",muted,caption);Ui::Text(160,244,192,20,current.pid?"Detectado":"Fechado",text,caption,2);
            Ui::Text(48,272,100,20,u8"Licença",muted,caption);Ui::Text(152,272,200,20,current.licensed?current.expiry.c_str():u8"Não ativada",text,caption,2);
            if(Button(48,320,148,32,"Como usar",true))state.help=true;
            if(Button(204,320,148,32,"Voltar",false,true))state.about=false;
        }
    }else if(busy){
        Ui::Text(32,80,336,40,current.phase==LoaderPhase::Loading?"Carregando":"Verificando",text,heading,1);
        auto* draw=ImGui::GetWindowDrawList();const auto center=P(200,208);float angle=static_cast<float>(ImGui::GetTime()*4);
        draw->AddCircle(center,16*uiScale,IM_COL32(51,52,57,255),32,2*uiScale);
        draw->PathArcTo(center,16*uiScale,angle,angle+2.5f,24);draw->PathStroke(IM_COL32(187,192,202,255),0,2*uiScale);
        Ui::Text(48,252,304,24,"Aguarde um instante.",muted,caption,1);
    }else if(!current.licensed){
        Ui::Text(32,84,336,44,u8"Ativação",text,heading,1);
        Ui::Text(48,172,304,24,u8"Licença",muted,caption);
        std::string before=state.license;
        bool enter=Ui::Input(48,208,304,36,"##license",u8"Cole sua licença",state.license,sizeof(state.license),ImGuiInputTextFlags_EnterReturnsTrue);
        if(before!=state.license)state.localMessage.clear();
        ImGui::GetWindowDrawList()->AddLine(P(48,244),P(352,244),IM_COL32(69,70,77,255),uiScale);
        ImGui::BeginDisabled(state.license[0]=='\0');
        if(Button(48,264,304,40,"Ativar",false,true)||enter){state.activate=true;state.localMessage.clear();}
        ImGui::EndDisabled();
        if(Button(48,320,148,24,"Abrir arquivo",true))state.importLicense=true;
        if(Button(204,320,148,24,"Copiar ID",true)&&!current.device.empty()){ImGui::SetClipboardText(current.device.c_str());state.localMessage=u8"ID copiado. Envie-o à equipe.";}
    }else{
        Ui::Text(32,68,336,44,"ZB2 Menu",text,heading,1);
        auto* draw=ImGui::GetWindowDrawList();draw->AddRectFilled(P(40,136),P(360,276),IM_COL32(31,31,34,255),6*uiScale);
        draw->AddRect(P(40,136),P(360,276),IM_COL32(45,45,49,255),6*uiScale);
        Row(150,ICON_FA_SHIELD_HALVED,"Jogo",current.pid?"Detectado":"Aguardando",current.pid?IM_COL32(146,186,117,255):muted);
        Row(192,ICON_FA_KEY,"Expira em",current.expiry.c_str());Row(234,ICON_FA_CODE_BRANCH,u8"Versão",current.version.c_str());
        if(current.phase==LoaderPhase::Ready){
            ImGui::SetCursorPos(P(48,292));ImGui::PushFont(caption);ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,P(2,2));ImGui::Checkbox(u8"Já entrei no mapa",&state.inMap);ImGui::PopStyleVar();ImGui::PopFont();
            ImGui::BeginDisabled(!state.inMap);if(Button(48,320,304,36,"Carregar menu",false,true))state.load=true;ImGui::EndDisabled();
        }else if(current.phase==LoaderPhase::Error){if(Button(48,320,304,36,"Verificar novamente",false,true))state.retry=true;}
        else{ImGui::BeginDisabled();Button(48,320,304,36,current.phase==LoaderPhase::Success?"Menu carregado":"Aguardando jogo");ImGui::EndDisabled();}
    }
    Status(state);ImGui::End();
}
