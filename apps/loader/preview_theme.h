#pragma once
#include "../../imgui/imgui.h"
namespace LoaderTheme {
extern ImFont* regular;
extern ImFont* heading;
extern ImFont* icons;
extern float uiScale;
constexpr ImU32 text=IM_COL32(238,238,241,255);
constexpr ImU32 muted=IM_COL32(171,174,181,255);
constexpr ImU32 borderColor=IM_COL32(53,55,60,255);
}
void ConfigureLoaderPreview(float dpi);
