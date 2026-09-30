#include "widgets.h"
#include "../../imgui/imgui_internal.h"
#include <Windows.h>
#include <algorithm>
#include <cfloat>
#include <cmath>
namespace Ui {
using namespace UiTheme;
namespace {
struct Ink {float left=FLT_MAX,top=FLT_MAX,right=-FLT_MAX,bottom=-FLT_MAX;};
Ink Measure(ImFont* face,const char* content){
    Ink ink;float pen=0;
    while(*content){unsigned int code=0;int bytes=ImTextCharFromUtf8(&code,content,nullptr);if(bytes<=0)break;content+=bytes;
        const ImFontGlyph* glyph=face->FindGlyph(static_cast<ImWchar>(code));if(!glyph)continue;
        if(glyph->Visible){ink.left=std::min(ink.left,pen+glyph->X0);ink.right=std::max(ink.right,pen+glyph->X1);ink.top=std::min(ink.top,glyph->Y0);ink.bottom=std::max(ink.bottom,glyph->Y1);}pen+=glyph->AdvanceX;
    }
    if(ink.left==FLT_MAX)return {0,0,0,0};return ink;
}
ImU32 Alpha(ImU32 color){const auto alpha=static_cast<unsigned>((color>>24)*ImGui::GetStyle().Alpha);return (color&0xffffffu)|(alpha<<24);}
}
ImVec2 P(float x,float y){return ImVec2(x*uiScale,y*uiScale);}
void Text(float x,float y,float width,float height,const char* content,ImU32 color,ImFont* face,int align){
    face=face?face:regular;auto ink=Measure(face,content);
    float left=x*uiScale-ink.left;
    if(align==1)left=(x+width*.5f)*uiScale-(ink.left+ink.right)*.5f;
    if(align==2)left=(x+width)*uiScale-ink.right;
    float top=(y+height*.5f)*uiScale-(ink.top+ink.bottom)*.5f;
    ImGui::GetWindowDrawList()->AddText(face,face->FontSize,ImVec2(IM_ROUND(left),IM_ROUND(top)),Alpha(color),content);
}
bool ReducedMotion(){BOOL animated=TRUE;SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION,0,&animated,0);return !animated;}
bool Button(float x,float y,float width,float height,const char* label,bool quiet,bool selected){
    ImGui::SetCursorPos(P(x,y));ImGui::PushFont(regular);ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0);
    ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0,0,0,0));ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(0,0,0,0));ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(0,0,0,0));
    bool clicked=ImGui::Button(label,P(width,height));const bool disabled=(ImGui::GetItemFlags()&ImGuiItemFlags_Disabled)!=0;
    const bool hot=ImGui::IsItemHovered(),pressed=ImGui::IsItemActive();
    auto* storage=ImGui::GetStateStorage();const auto id=ImGui::GetItemID();float fade=storage->GetFloat(id,0);
    float target=selected?1.f:hot?.6f:0.f;
    static const bool reduced=ReducedMotion();
    fade=reduced?target:ImLerp(fade,target,1.f-std::exp(-ImGui::GetIO().DeltaTime/.04f));storage->SetFloat(id,fade);
    ImU32 fill=quiet?IM_COL32(47,47,52,static_cast<int>(fade*180)):IM_COL32(static_cast<int>(34+fade*15),static_cast<int>(34+fade*15),static_cast<int>(38+fade*17),255);
    if(pressed&&!disabled)fill=IM_COL32(62,62,68,255);
    ImGui::GetWindowDrawList()->AddRectFilled(P(x,y),P(x+width,y+height),Alpha(fill),5*uiScale);
    Text(x,y,width,height,label,disabled?IM_COL32(108,111,120,255):selected||!quiet?text:muted,regular,1);
    if(ImGui::IsItemFocused()&&ImGui::GetIO().NavVisible)ImGui::GetWindowDrawList()->AddRect(P(x+2,y+2),P(x+width-2,y+height-2),Alpha(IM_COL32(170,180,200,255)),4*uiScale);
    ImGui::PopStyleColor(4);ImGui::PopStyleVar();ImGui::PopFont();return clicked;
}
bool Input(float x,float y,float width,float height,const char* id,const char* hint,char* buffer,int capacity,ImGuiInputTextFlags flags){
    ImGui::SetCursorPos(P(x,y));ImGui::PushFont(regular);auto ink=Measure(regular,"Ag");
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(12*uiScale,height*.5f*uiScale-(ink.top+ink.bottom)*.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,1*uiScale);
    bool result=ImGui::InputTextEx(id,hint,buffer,capacity,P(width,height),flags);
    ImGui::PopStyleVar(2);ImGui::PopFont();return result;
}
void Hint(const char* content){if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))ImGui::SetTooltip("%s",content);}
void Panel(float x,float y,float width,float height){auto* draw=ImGui::GetWindowDrawList();draw->AddRectFilled(P(x,y),P(x+width,y+height),Alpha(IM_COL32(31,31,35,255)),6*uiScale);draw->AddRect(P(x,y),P(x+width,y+height),Alpha(IM_COL32(47,47,52,255)),6*uiScale);}
void Device(float x,float y,float width,const std::string& device){
    Panel(x,y,width,52);const auto first=device.substr(0,32),second=device.size()>32?device.substr(32):std::string();
    Text(x+12,y+6,width-24,18,first.empty()?"Aguardando ID":first.c_str(),text,caption);
    Text(x+12,y+28,width-24,18,second.c_str(),text,caption);
}
bool DeviceFits(const std::string& device,float width){return device.size()==64 && caption->CalcTextSizeA(caption->FontSize,10000,0,device.substr(0,32).c_str()).x<=(width-24)*uiScale && caption->CalcTextSizeA(caption->FontSize,10000,0,device.substr(32).c_str()).x<=(width-24)*uiScale;}
void Wrapped(float x,float y,float width,const char* content,ImU32 color){ImGui::GetWindowDrawList()->AddText(regular,regular->FontSize,P(x,y),Alpha(color),content,nullptr,width*uiScale);}
}
