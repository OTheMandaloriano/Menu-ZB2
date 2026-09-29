#include "preview_ui.h"
#include "../../imgui/imgui.h"
#include "preview_theme.h"
namespace {
using namespace LoaderTheme;
ImVec2 P(float x,float y){return ImVec2(x*uiScale,y*uiScale);}
void Text(float x,float y,const char* value,float size=14,ImU32 color=text,ImFont* face=nullptr){
    ImGui::GetWindowDrawList()->AddText(face?face:regular,size*uiScale,P(x,y),color,value);
}
void Center(float y,const char* value,float size=14,ImU32 color=text,ImFont* face=nullptr){
    auto f=face?face:regular;auto width=f->CalcTextSizeA(size*uiScale,10000,0,value).x;
    ImGui::GetWindowDrawList()->AddText(f,size*uiScale,ImVec2((420*uiScale-width)*.5f,y*uiScale),color,value);
}
bool Button(float x,float y,float width,float height,const char* label,bool primary=false){
    ImGui::SetCursorPos(P(x,y));
    ImGui::PushStyleColor(ImGuiCol_Button,primary?ImVec4(.89f,.90f,.91f,1):ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,primary?ImVec4(.98f,.98f,.99f,1):ImVec4(.16f,.17f,.19f,1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,primary?ImVec4(.75f,.77f,.79f,1):ImVec4(.20f,.21f,.23f,1));
    ImGui::PushStyleColor(ImGuiCol_Text,primary?ImVec4(.10f,.11f,.12f,1):ImVec4(.71f,.73f,.77f,1));
    bool result=ImGui::Button(label,P(width,height));ImGui::PopStyleColor(4);return result;
}
void Row(float y,const char* icon,const char* label,const char* value){
    if(icons)Text(54,y,icon,16,muted,icons);
    Text(84,y,label,13,muted);
    float w=regular->CalcTextSizeA(13*uiScale,10000,0,value).x;
    ImGui::GetWindowDrawList()->AddText(regular,13*uiScale,ImVec2(365*uiScale-w,y*uiScale),text,value);
}
}
void DrawLoaderPreview(LoaderPreviewState& state){
    ImGui::SetNextWindowPos(ImVec2(0,0));ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("ZB2 Menu",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
    Text(22,18,"ZB2",13,muted);
    if(Button(336,6,36,32,"_"))state.minimize=true;
    if(Button(375,6,36,32,"x"))state.close=true;
    if(state.page==0){
        Center(92,u8"Ativação",29,text,heading);Center(137,"Seu acesso ao ZB2 Menu.",14,muted);
        Text(46,201,u8"Licença",12,muted);
        ImGui::SetCursorPos(P(46,226));ImGui::SetNextItemWidth(328*uiScale);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,1*uiScale);
        ImGui::InputTextWithHint("##license",u8"Cole sua licença",state.license,sizeof(state.license));
        ImGui::PopStyleVar();
        if(Button(46,294,328,44,u8"Ativar licença",true))state.attempted=true;
        if(state.attempted)Center(349,u8"Prévia visual: ativação ainda não disponível.",12,IM_COL32(224,180,120,255));
        if(Button(46,373,328,30,u8"Ver painel de demonstração")){state.page=1;state.attempted=false;}
    }else{
        Center(92,"ZB2 Menu",29,text,heading);Center(137,u8"Painel de demonstração.",14,muted);
        auto* draw=ImGui::GetWindowDrawList();draw->AddRectFilled(P(38,187),P(382,340),IM_COL32(34,35,39,255),7*uiScale);draw->AddRect(P(38,187),P(382,340),borderColor,7*uiScale);
        Row(205,u8"\ue72e",u8"Licença",u8"Não validada");Row(252,u8"\ue787","Expira em",u8"Após ativação");Row(299,u8"\ue895",u8"Versão",u8"Prévia 0.2");
        draw->AddLine(P(55,237),P(365,237),borderColor);draw->AddLine(P(55,284),P(365,284),borderColor);
        if(Button(38,363,344,44,"Carregar menu",true))state.attempted=true;
        if(state.attempted)Center(416,u8"Carregamento indisponível nesta prévia.",11,IM_COL32(224,180,120,255));
    }
    if(Button(98,426,224,24,state.page==0?u8"v0.2  ·  Prévia visual":u8"Voltar à ativação")){if(state.page==1){state.page=0;state.attempted=false;}}
    ImGui::End();
}
