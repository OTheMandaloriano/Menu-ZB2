#include "ui.h"
#include "theme.h"
#include "assets/IconsFontAwesome6.h"
#include "../../imgui/imgui_internal.h"
#include <algorithm>
#include <cfloat>

namespace {
using namespace LoaderTheme;
ImVec2 P(float x,float y){return ImVec2(x*uiScale,y*uiScale);}
struct Ink {float left=FLT_MAX,top=FLT_MAX,right=-FLT_MAX,bottom=-FLT_MAX;};
Ink Measure(ImFont* face,const char* content){
    Ink ink;float pen=0;
    while(*content){unsigned int code=0;int bytes=ImTextCharFromUtf8(&code,content,nullptr);if(bytes<=0)break;content+=bytes;
        const ImFontGlyph* glyph=face->FindGlyph(static_cast<ImWchar>(code));if(!glyph)continue;
        if(glyph->Visible){ink.left=std::min(ink.left,pen+glyph->X0);ink.right=std::max(ink.right,pen+glyph->X1);ink.top=std::min(ink.top,glyph->Y0);ink.bottom=std::max(ink.bottom,glyph->Y1);}pen+=glyph->AdvanceX;
    }
    if(ink.left==FLT_MAX)return {0,0,0,0};return ink;
}
void Aligned(float x,float y,float width,float height,const char* value,ImU32 color=text,ImFont* face=nullptr,int align=0){
    face=face?face:regular;auto ink=Measure(face,value);
    float left=x*uiScale-ink.left;
    if(align==1)left=(x+width*.5f)*uiScale-(ink.left+ink.right)*.5f;
    if(align==2)left=(x+width)*uiScale-ink.right;
    float top=(y+height*.5f)*uiScale-(ink.top+ink.bottom)*.5f;
    ImGui::GetWindowDrawList()->AddText(face,face->FontSize,ImVec2(IM_ROUND(left),IM_ROUND(top)),color,value);
}
bool Button(float x,float y,float width,float height,const char* label,bool quiet=false,bool primary=false){
    ImGui::SetCursorPos(P(x,y));ImGui::PushFont(regular);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0);
    ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_Button,quiet?ImVec4(0,0,0,0):ImVec4(.135f,.135f,.145f,1));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(.18f,.18f,.195f,1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(.20f,.20f,.215f,1));
    bool clicked=ImGui::Button(label,P(width,height));
    bool disabled=(ImGui::GetItemFlags()&ImGuiItemFlags_Disabled)!=0;
    auto color=disabled?IM_COL32(100,103,111,255):primary?text:muted;
    Aligned(x,y,width,height,label,color,regular,1);
    ImGui::PopStyleColor(4);ImGui::PopStyleVar();ImGui::PopFont();return clicked;
}
void Hint(const char* message){if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))ImGui::SetTooltip("%s",message);}
void Status(const LoaderUiState& state){
    std::string message=state.localMessage.empty()?state.snapshot.message:state.localMessage;
    if(state.snapshot.phase==LoaderPhase::Activation && state.localMessage.empty())return;
    std::string shown=message;auto* face=small;
    while(!shown.empty() && face->CalcTextSizeA(face->FontSize,10000,0,(shown+"...").c_str()).x>304*uiScale){size_t n=shown.size()-1;while(n && (static_cast<unsigned char>(shown[n])&0xc0)==0x80)--n;shown.resize(n);}
    if(shown!=message)shown+="...";
    auto color=state.snapshot.phase==LoaderPhase::Error?IM_COL32(224,150,148,255):muted;
    Aligned(48,360,304,20,shown.c_str(),color,small,1);
    ImGui::SetCursorPos(P(48,360));ImGui::InvisibleButton("##status",P(304,20));
    if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)){ImGui::BeginTooltip();ImGui::PushTextWrapPos(330*uiScale);ImGui::TextUnformatted(message.c_str());ImGui::PopTextWrapPos();ImGui::EndTooltip();}
}
void Row(float y,const char* icon,const char* label,const char* value,ImU32 color=text){
    Aligned(56,y,16,28,icon,muted,regular,1);Aligned(84,y,88,28,label,muted,small);
    Aligned(168,y,176,28,value,color,small,2);
}
}

void DrawLoader(LoaderUiState& state){
    const auto& current=state.snapshot;
    if(state.lastPid!=current.pid){state.inMap=false;state.lastPid=current.pid;}
    const bool busy=current.phase==LoaderPhase::Checking||current.phase==LoaderPhase::Loading;
    ImGui::SetNextWindowPos(ImVec2(0,0));ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("ZB2 Menu",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
    Aligned(24,16,200,28,"ZB2",muted,small);
    if(Button(312,16,28,28,ICON_FA_CIRCLE_INFO,true))state.about=!state.about;Hint("Sobre este acesso");
    if(Button(356,16,28,28,ICON_FA_XMARK,true))state.close=true;Hint("Fechar");
    if(state.about){
        Aligned(32,72,336,40,"Seu acesso",text,heading,1);
        if(state.legal){
            static const auto notices=LoaderServices::Resource(205);
            ImGui::SetCursorPos(P(40,136));ImGui::BeginChild("Licencas",P(320,152),false);ImGui::TextWrapped("%s",notices.c_str());ImGui::EndChild();
            if(Button(48,312,304,40,"Voltar",false,true))state.legal=false;
        }else{
            Aligned(48,140,304,24,"Computador",muted,small);
            auto id=current.device.empty()?std::string("Verificando..."):current.device.substr(0,28)+"...";
            Aligned(48,172,304,24,id.c_str(),text,small);
            if(Button(48,212,304,32,"Copiar ID",true)&&!current.device.empty()){ImGui::SetClipboardText(current.device.c_str());state.localMessage="ID copiado.";}
            if(Button(48,252,304,32,u8"Licenças de terceiros",true))state.legal=true;
            if(Button(48,312,304,40,"Voltar",false,true))state.about=false;
        }
    }else if(busy){
        Aligned(32,80,336,40,current.phase==LoaderPhase::Loading?"Carregando":"Verificando",text,heading,1);
        auto* draw=ImGui::GetWindowDrawList();const auto center=P(200,208);float angle=static_cast<float>(ImGui::GetTime()*4);
        draw->AddCircle(center,16*uiScale,IM_COL32(51,52,57,255),32,2*uiScale);
        draw->PathArcTo(center,16*uiScale,angle,angle+2.5f,24);draw->PathStroke(IM_COL32(187,192,202,255),0,2*uiScale);
        Aligned(48,252,304,24,"Aguarde um instante.",muted,small,1);
    }else if(!current.licensed){
        Aligned(32,84,336,44,u8"Ativação",text,heading,1);
        Aligned(48,172,304,24,u8"Licença",muted,small);
        ImGui::SetCursorPos(P(48,208));ImGui::PushFont(regular);
        auto ink=Measure(regular,"Ag");ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(12*uiScale,18*uiScale-(ink.top+ink.bottom)*.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0);
        std::string before=state.license;
        bool enter=ImGui::InputTextEx("##license",u8"Cole sua licença",state.license,sizeof(state.license),P(304,36),ImGuiInputTextFlags_EnterReturnsTrue);
        if(before!=state.license)state.localMessage.clear();
        ImGui::PopStyleVar(2);ImGui::PopFont();
        ImGui::GetWindowDrawList()->AddLine(P(48,244),P(352,244),IM_COL32(69,70,77,255),uiScale);
        ImGui::BeginDisabled(state.license[0]=='\0');
        if(Button(48,264,304,40,"Ativar",false,true)||enter){state.activate=true;state.localMessage.clear();}
        ImGui::EndDisabled();
        if(Button(48,320,148,24,"Abrir arquivo",true))state.importLicense=true;
        if(Button(204,320,148,24,"Copiar ID",true)&&!current.device.empty()){ImGui::SetClipboardText(current.device.c_str());state.localMessage=u8"ID copiado. Envie-o à equipe.";}
    }else{
        Aligned(32,68,336,44,"ZB2 Menu",text,heading,1);
        auto* draw=ImGui::GetWindowDrawList();draw->AddRectFilled(P(40,136),P(360,276),IM_COL32(31,31,34,255),6*uiScale);
        draw->AddRect(P(40,136),P(360,276),IM_COL32(45,45,49,255),6*uiScale);
        Row(150,ICON_FA_SHIELD_HALVED,"Jogo",current.pid?"Detectado":"Aguardando",current.pid?IM_COL32(146,186,117,255):muted);
        Row(192,ICON_FA_KEY,"Expira em",current.expiry.c_str());Row(234,ICON_FA_CODE_BRANCH,u8"Versão",current.version.c_str());
        if(current.phase==LoaderPhase::Ready){
            ImGui::SetCursorPos(P(48,292));ImGui::PushFont(small);ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,P(2,2));ImGui::Checkbox(u8"Já entrei no mapa",&state.inMap);ImGui::PopStyleVar();ImGui::PopFont();
            ImGui::BeginDisabled(!state.inMap);if(Button(48,320,304,36,"Carregar menu",false,true))state.load=true;ImGui::EndDisabled();
        }else if(current.phase==LoaderPhase::Error){if(Button(48,320,304,36,"Verificar novamente",false,true))state.retry=true;}
        else{ImGui::BeginDisabled();Button(48,320,304,36,current.phase==LoaderPhase::Success?"Menu carregado":"Aguardando jogo");ImGui::EndDisabled();}
    }
    Status(state);ImGui::End();
}
