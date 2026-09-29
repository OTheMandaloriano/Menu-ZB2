#include "ui.h"
#include "../../imgui/imgui.h"
#include "theme.h"
#include "assets/IconsFontAwesome6.h"
#include <cmath>
namespace {
using namespace LoaderTheme;
ImVec2 P(float x,float y){return ImVec2(x*uiScale,y*uiScale);}
void Text(float x,float y,const char* value,ImU32 color=text,ImFont* face=nullptr){
    auto font=face?face:regular;ImGui::GetWindowDrawList()->AddText(font,font->FontSize,P(x,y),color,value);
}
bool Button(float x,float y,float width,float height,const char* label,bool quiet=false){
    ImGui::SetCursorPos(P(x,y));ImGui::PushFont(button);
    if(quiet){ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0,0,0,0));ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(.12f,.14f,.18f,1));}
    bool clicked=ImGui::Button(label,P(width,height));
    if(quiet)ImGui::PopStyleColor(2);ImGui::PopFont();return clicked;
}
void Row(float y,const char* icon,const char* label,const char* value,ImU32 color=text){
    Text(32,y,icon,muted);Text(56,y,label,muted);
    auto w=regular->CalcTextSizeA(regular->FontSize,10000,0,value).x;
    ImGui::GetWindowDrawList()->AddText(regular,regular->FontSize,ImVec2(428*uiScale-w,y*uiScale),color,value);
}
void Footer(const std::string& message,ImU32 color){
    std::string shown=message;
    while(regular->CalcTextSizeA(regular->FontSize,10000,0,(shown+"...").c_str()).x>420*uiScale && !shown.empty()){
        size_t start=shown.size()-1;while(start>0 && (static_cast<unsigned char>(shown[start])&0xc0)==0x80)--start;
        shown.resize(start);
    }
    if(shown!=message)shown+="...";
    Text(20,228,shown.c_str(),color);ImGui::SetCursorPos(P(20,224));ImGui::InvisibleButton("##status",P(420,24));
    if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)){ImGui::BeginTooltip();ImGui::PushTextWrapPos(400*uiScale);ImGui::TextUnformatted(message.c_str());ImGui::PopTextWrapPos();ImGui::EndTooltip();}
}
}
void DrawLoader(LoaderUiState& state){
    const auto& current=state.snapshot;
    if(state.lastPid!=current.pid){state.inMap=false;state.lastPid=current.pid;}
    const bool busy=current.phase==LoaderPhase::Checking||current.phase==LoaderPhase::Loading;
    ImGui::SetNextWindowPos(ImVec2(0,0));ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("ZB2 Menu",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
    Text(20,16,"ZB2",text,heading);Text(80,24,"MENU",muted);Text(296,24,"v1.0",muted);
    if(Button(364,12,32,32,ICON_FA_CIRCLE_INFO,true))state.about=!state.about;
    if(Button(408,12,32,32,ICON_FA_XMARK,true))state.close=true;
    auto* draw=ImGui::GetWindowDrawList();draw->AddLine(P(20,56),P(440,56),borderColor);
    if(state.about){
        if(state.legal){
            static const auto notices=LoaderServices::Resource(205);
            ImGui::SetCursorPos(P(20,68));ImGui::BeginChild("Licencas",P(420,108),false);ImGui::TextWrapped("%s",notices.c_str());ImGui::EndChild();
            if(Button(20,184,420,36,"Voltar",true))state.legal=false;
        }else{
            Text(20,72,"Este computador",muted);
            std::string shortDevice=current.device.empty()?"Verificando...":current.device.substr(0,24)+"...";Text(20,96,shortDevice.c_str());
            if(Button(312,76,128,36,"Copiar ID",true)&&!current.device.empty())ImGui::SetClipboardText(current.device.c_str());
            Text(20,128,("Processo: "+(current.pid?std::to_string(current.pid):"aguardando")).c_str(),muted);
            Text(20,152,u8"Licença offline por PC. Atualizações manuais.",muted);
            if(Button(20,184,204,36,u8"Licenças de terceiros",true))state.legal=true;
            if(Button(236,184,204,36,"Voltar",true))state.about=false;
        }
        Footer("ZB2 Menu / Loader nativo x64",muted);
    }else if(busy){
        auto center=P(230,116);float angle=static_cast<float>(ImGui::GetTime()*4.5);
        draw->AddCircle(center,16*uiScale,IM_COL32(32,40,54,255),32,3*uiScale);
        draw->PathArcTo(center,16*uiScale,angle,angle+2.5f,24);draw->PathStroke(IM_COL32(37,99,235,255),0,3*uiScale);
        Text(144,156,current.phase==LoaderPhase::Loading?"Carregando menu...":"Verificando acesso...",muted);
        Footer(current.message,muted);
    }else if(!current.licensed){
        Text(20,72,u8"Ative seu acesso",text,button);Text(20,100,u8"Copie o ID em Sobre para solicitar sua licença.",muted);
        ImGui::SetCursorPos(P(20,132));ImGui::SetNextItemWidth(420*uiScale);
        bool enter=ImGui::InputTextWithHint("##license",u8"Cole sua chave de licença",state.license,sizeof(state.license),ImGuiInputTextFlags_EnterReturnsTrue|ImGuiInputTextFlags_Password);
        if(Button(20,184,420,36,u8"Verificar licença")||enter)state.activate=true;
        Footer(current.message,current.phase==LoaderPhase::Error?IM_COL32(255,139,139,255):muted);
    }else{
        draw->AddRectFilled(P(20,68),P(440,164),IM_COL32(18,21,28,255),4*uiScale);
        Row(80,ICON_FA_SHIELD_HALVED,"Jogo",current.pid?"Detectado":"Aguardando",current.pid?IM_COL32(16,185,129,255):IM_COL32(245,158,11,255));
        Row(108,ICON_FA_KEY,u8"Licença",current.expiry.c_str());Row(136,ICON_FA_CODE_BRANCH,"Build",current.version.c_str());
        if(current.phase==LoaderPhase::Ready){
            ImGui::SetCursorPos(P(20,184));ImGui::Checkbox("No mapa",&state.inMap);
            ImGui::BeginDisabled(!state.inMap);
            if(Button(156,184,284,36,ICON_FA_BOLT "  Carregar menu"))state.load=true;
            ImGui::EndDisabled();
        }else if(current.phase==LoaderPhase::Success){
            draw->AddRectFilled(P(20,184),P(440,220),IM_COL32(13,40,32,255),4*uiScale);
            Text(144,194,ICON_FA_CHECK "  Menu carregado",IM_COL32(16,185,129,255));
        }else if(current.phase==LoaderPhase::Error){
            if(Button(20,184,420,36,"Verificar novamente"))state.retry=true;
        }else{
            ImGui::BeginDisabled();Button(20,184,420,36,current.phase==LoaderPhase::Success?ICON_FA_CHECK "  Menu carregado":current.phase==LoaderPhase::Waiting?"Aguardando jogo":"Confira o aviso abaixo");ImGui::EndDisabled();
        }
        Footer(current.message,current.phase==LoaderPhase::Error?IM_COL32(255,139,139,255):muted);
    }
    ImGui::End();
}
