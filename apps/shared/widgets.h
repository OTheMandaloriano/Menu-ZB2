#pragma once
#include "theme.h"
#include <string>
namespace Ui {
ImVec2 P(float x,float y);
void Brand(float x,float y,const char* role);
void Text(float x,float y,float width,float height,const char* content,ImU32 color=UiTheme::text,ImFont* face=nullptr,int align=0);
bool Button(float x,float y,float width,float height,const char* label,bool quiet=false,bool selected=false,const char* icon=nullptr);
float ButtonWidth(const char* label,const char* icon=nullptr);
bool Input(float x,float y,float width,float height,const char* id,const char* hint,char* buffer,int capacity,ImGuiInputTextFlags flags=0,const char* icon=nullptr);
bool Toggle(float x,float y,float width,const char* label,bool& enabled);
void Badge(const char* label,bool active);
void Tabular(const char* value);
float TabularWidth(const char* value);
bool RowCopy(const char* icon);
bool RowAction(const char* icon);
void Hint(const char* text);
void Panel(float x,float y,float width,float height);
void Device(float x,float y,float width,const std::string& device);
bool DeviceFits(const std::string& device,float width);
void Wrapped(float x,float y,float width,const char* content,ImU32 color=UiTheme::muted);
bool ReducedMotion();
void SetReducedMotion(bool enabled);
}
