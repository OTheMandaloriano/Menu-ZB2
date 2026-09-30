#include "widgets.h"
#include "../../imgui/imgui_internal.h"
#include <Windows.h>
#include <algorithm>
#include <cfloat>
#include <cmath>
namespace Ui {
using namespace UiTheme;
namespace {
bool forceReducedMotion=false;
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
bool ReducedMotion(){BOOL animated=TRUE;SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION,0,&animated,0);return forceReducedMotion||!animated;}
void SetReducedMotion(bool enabled){forceReducedMotion=enabled;}
bool Button(float x,float y,float width,float height,const char* label,bool quiet,bool selected,const char* icon){
    ImGui::SetCursorPos(P(x,y));ImGui::PushFont(regular);ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0);
    ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0,0,0,0));ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(0,0,0,0));ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(0,0,0,0));
    bool clicked=ImGui::Button(label,P(width,height));const bool disabled=(ImGui::GetItemFlags()&ImGuiItemFlags_Disabled)!=0;
    const bool hot=ImGui::IsItemHovered(),pressed=ImGui::IsItemActive();
    auto* storage=ImGui::GetStateStorage();const auto id=ImGui::GetItemID();float fade=storage->GetFloat(id,0);
    float target=selected?1.f:hot?.6f:0.f;
    const bool reduced=ReducedMotion();
    fade=reduced?target:ImLerp(fade,target,1.f-std::exp(-ImGui::GetIO().DeltaTime/.04f));storage->SetFloat(id,fade);
    const bool primary=!quiet&&selected;const float scale=!quiet&&pressed&&!disabled?.98f:1.f;
    ImU32 fill=quiet?IM_COL32(47,47,52,static_cast<int>(fade*180)):IM_COL32(static_cast<int>(26+fade*15),static_cast<int>(30+fade*15),static_cast<int>(38+fade*17),255);
    if(primary)fill=pressed?IM_COL32(29,78,190,255):hot?IM_COL32(48,111,246,255):IM_COL32(37,99,235,255);
    else if(pressed&&!disabled)fill=IM_COL32(62,62,68,255);
    auto a=P(x+width*(1-scale)*.5f,y+height*(1-scale)*.5f),b=P(x+width*(1+scale)*.5f,y+height*(1+scale)*.5f);
    auto* draw=ImGui::GetWindowDrawList();draw->AddRectFilled(a,b,Alpha(fill),5*uiScale);
    if(!quiet&&!primary)draw->AddRect(a,b,Alpha(IM_COL32(255,255,255,15)),5*uiScale,0,uiScale);
    const auto color=primary?IM_COL32(255,255,255,255):disabled?IM_COL32(140,145,154,255):selected||!quiet?text:muted;
    float textWidth=regular->CalcTextSizeA(regular->FontSize,10000,0,label).x;
    float iconWidth=icon?16*uiScale:0, gap=icon?8*uiScale:0;
    const auto labelInk=Measure(regular,label);float left=(x+width*.5f)*uiScale-(textWidth+iconWidth+gap)*scale*.5f;
    auto emit=[&](const char* content,float at,float boxWidth){auto ink=Measure(regular,content);float px=at+(boxWidth-(ink.right-ink.left)*scale)*.5f-ink.left*scale;float py=(y+height*.5f)*uiScale-(ink.top+ink.bottom)*scale*.5f;draw->AddText(regular,regular->FontSize*scale,ImVec2(IM_ROUND(px),IM_ROUND(py)),Alpha(color),content);};
    if(icon){emit(icon,left,iconWidth*scale);left+=(iconWidth+gap)*scale;}
    (void)labelInk;emit(label,left,textWidth*scale);
    if(ImGui::IsItemFocused()&&ImGui::GetIO().NavVisible)ImGui::GetWindowDrawList()->AddRect(P(x+2,y+2),P(x+width-2,y+height-2),Alpha(IM_COL32(170,180,200,255)),4*uiScale);
    ImGui::PopStyleColor(4);ImGui::PopStyleVar();ImGui::PopFont();return clicked;
}
bool Input(float x,float y,float width,float height,const char* id,const char* hint,char* buffer,int capacity,ImGuiInputTextFlags flags,const char* icon){
    auto* draw=ImGui::GetWindowDrawList();draw->AddRectFilled(P(x,y),P(x+width,y+height),Alpha(IM_COL32(24,28,38,255)),4*uiScale);
    const float gutter=icon?32.f:0.f;
    ImGui::SetCursorPos(P(x+gutter,y));ImGui::PushFont(regular);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,P(14,10));ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(0,0,0,0));ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,ImVec4(0,0,0,0));ImGui::PushStyleColor(ImGuiCol_FrameBgActive,ImVec4(0,0,0,0));
    bool result=ImGui::InputTextEx(id,hint,buffer,capacity,P(width-gutter,height),flags);
    bool active=ImGui::IsItemActive();ImGui::PopStyleColor(3);ImGui::PopStyleVar(2);ImGui::PopFont();
    draw->AddRect(P(x,y),P(x+width,y+height),Alpha(active?IM_COL32(90,130,210,255):IM_COL32(255,255,255,15)),4*uiScale,0,uiScale);
    if(icon)Text(x+12,y,16,height,icon,muted,regular,1);return result;
}
bool Toggle(float x,float y,float width,const char* label,bool& enabled){
    ImGui::SetCursorPos(P(x,y));bool changed=ImGui::InvisibleButton(label,P(width,32));if(changed)enabled=!enabled;
    Text(x,y,width-112,32,label,text,regular);
    Text(x+width-124,y,68,32,enabled?"ATIVADO":"DESLIGADO",enabled?IM_COL32(110,210,170,255):muted,caption,2);
    auto* draw=ImGui::GetWindowDrawList();const float left=x+width-48;draw->AddRectFilled(P(left,y+4),P(left+48,y+28),Alpha(enabled?IM_COL32(37,99,235,255):IM_COL32(70,73,80,255)),12*uiScale);
    draw->AddCircleFilled(P(left+(enabled?36:12),y+16),9*uiScale,Alpha(IM_COL32(255,255,255,255)));return changed;
}
float TabularWidth(const char* value){float cell=0;for(char c='0';c<='9';++c)cell=std::max(cell,caption->FindGlyph(c)->AdvanceX);float width=0;for(const char* p=value;*p;++p)width+=*p>='0'&&*p<='9'?cell:caption->FindGlyph(static_cast<ImWchar>(*p))->AdvanceX;return width;}
void Tabular(const char* value){
    auto pos=ImGui::GetCursorScreenPos();float cell=0;for(char c='0';c<='9';++c)cell=std::max(cell,caption->FindGlyph(c)->AdvanceX);
    for(const char* p=value;*p;++p){char one[2]={*p,0};auto glyph=caption->FindGlyph(static_cast<ImWchar>(*p));float advance=*p>='0'&&*p<='9'?cell:glyph->AdvanceX;ImGui::GetWindowDrawList()->AddText(caption,caption->FontSize,ImVec2(pos.x+(advance-glyph->AdvanceX)*.5f,pos.y),ImGui::GetColorU32(ImGuiCol_Text),one);pos.x+=advance;}
    ImGui::Dummy(ImVec2(TabularWidth(value),caption->FontSize));
}
void Badge(const char* label,bool active){
    auto at=ImGui::GetCursorScreenPos();float width=caption->CalcTextSizeA(caption->FontSize,10000,0,label).x+26*uiScale,height=24*uiScale;
    auto* draw=ImGui::GetWindowDrawList();auto foreground=active?IM_COL32(124,222,169,255):IM_COL32(235,158,149,255);
    draw->AddRectFilled(at,ImVec2(at.x+width,at.y+height),Alpha(active?IM_COL32(16,185,129,38):IM_COL32(239,68,68,38)),12*uiScale);
    draw->AddCircleFilled(ImVec2(at.x+10*uiScale,at.y+height*.5f),2.5f*uiScale,Alpha(foreground));auto ink=Measure(caption,label);
    draw->AddText(caption,caption->FontSize,ImVec2(at.x+18*uiScale,at.y+height*.5f-(ink.top+ink.bottom)*.5f),Alpha(foreground),label);
    ImGui::Dummy(ImVec2(width,height));
}
bool RowCopy(const char* icon){
    auto at=ImGui::GetCursorScreenPos();ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(.15f,.15f,.17f,1));ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,P(6,4));ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0,0,0,0));
    bool clicked=ImGui::Button("##copy",P(62,24));ImGui::PopStyleColor(2);ImGui::PopStyleVar();
    auto* draw=ImGui::GetWindowDrawList();auto ink=Measure(regular,icon);draw->AddText(regular,regular->FontSize,ImVec2(at.x+7*uiScale,at.y+12*uiScale-(ink.top+ink.bottom)*.5f),Alpha(text),icon);
    ink=Measure(caption,"Copiar");draw->AddText(caption,caption->FontSize,ImVec2(at.x+27*uiScale,at.y+12*uiScale-(ink.top+ink.bottom)*.5f),Alpha(text),"Copiar");return clicked;
}
bool RowAction(const char* icon){
    auto at=ImGui::GetCursorScreenPos();bool clicked=ImGui::InvisibleButton(icon,P(28,28));
    bool pressed=ImGui::IsItemActive(),hot=ImGui::IsItemHovered();float scale=pressed?.98f:1.f;
    auto* draw=ImGui::GetWindowDrawList();float inset=14*(1-scale)*uiScale;
    draw->AddRectFilled(ImVec2(at.x+inset,at.y+inset),ImVec2(at.x+28*uiScale-inset,at.y+28*uiScale-inset),Alpha(hot?IM_COL32(43,51,66,255):IM_COL32(26,30,38,255)),4*uiScale);
    draw->AddRect(at,ImVec2(at.x+28*uiScale,at.y+28*uiScale),Alpha(IM_COL32(255,255,255,15)),4*uiScale);
    auto ink=Measure(regular,icon);draw->AddText(regular,regular->FontSize*scale,ImVec2(at.x+14*uiScale-(ink.left+ink.right)*scale*.5f,at.y+14*uiScale-(ink.top+ink.bottom)*scale*.5f),Alpha(text),icon);
    return clicked;
}
void Hint(const char* content){if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))ImGui::SetTooltip("%s",content);}
void Panel(float x,float y,float width,float height){auto* draw=ImGui::GetWindowDrawList();draw->AddRectFilled(P(x,y),P(x+width,y+height),Alpha(IM_COL32(18,21,28,255)),6*uiScale);draw->AddRect(P(x,y),P(x+width,y+height),Alpha(IM_COL32(255,255,255,15)),6*uiScale);}
void Device(float x,float y,float width,const std::string& device){
    Panel(x,y,width,36);const auto shown=device.size()==64?"HWID: "+device.substr(0,8)+"..."+device.substr(55):std::string("HWID indisponível");
    Text(x+12,y,16,36,"\xef\x8b\x9b",muted,regular,1);
    Text(x+36,y,width-48,36,shown.c_str(),text,caption);
    ImGui::SetCursorPos(P(x,y));ImGui::InvisibleButton("##device-info",P(width,36));Hint(device.c_str());
}
bool DeviceFits(const std::string& device,float width){auto shown="HWID: "+device.substr(0,8)+"..."+(device.size()>55?device.substr(55):"");return device.size()==64&&caption->CalcTextSizeA(caption->FontSize,10000,0,shown.c_str()).x<=(width-48)*uiScale;}
void Wrapped(float x,float y,float width,const char* content,ImU32 color){ImGui::GetWindowDrawList()->AddText(regular,regular->FontSize,P(x,y),Alpha(color),content,nullptr,width*uiScale);}
}
