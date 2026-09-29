#pragma once
#include "../../imgui/imgui.h"
namespace LoaderTheme {
extern ImFont* regular;
extern ImFont* heading;
extern ImFont* icons;
extern ImFont* button;
extern float uiScale;
constexpr ImU32 text=IM_COL32(243,244,246,255);
constexpr ImU32 muted=IM_COL32(157,164,178,255);
constexpr ImU32 borderColor=IM_COL32(255,255,255,15);
}
void ConfigureLoaderTheme(float dpi);
