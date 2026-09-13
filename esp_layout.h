#pragma once

#include "imgui/imgui.h"
#include <array>
#include <string>

namespace EspLayout {

enum Element { Name, Distance, Health, Percent, Count };
enum Side { Top, Bottom, Left, Right };

struct Rect {
    ImVec2 min;
    ImVec2 max;

    float Width() const;
    float Height() const;
    bool Contains(ImVec2 point) const;
    bool Contains(const Rect& rect) const;
};

struct Placement {
    int side = Top;
    float position = 0.5f;
    float extraGap = 0.0f;
    int order = 0;
    bool enabled = true;
};

struct Model {
    std::array<Placement, Count> items;
    float gap = 4.0f;
    float spacing = 3.0f;
    float barLength = 1.0f;
    float barThickness = 3.0f;
    float gridSize = 1.0f;
    bool grid = true;
    bool stack = true;
    bool horizontalText = false;
    bool guides = true;
    bool skeleton = false;
    bool headDot = false;
    bool snapline = false;
    int boxStyle = 0;
    int preset = 1;
};

struct Content {
    std::array<std::string, Count> text;
    float health = 0.87f;
    bool healthAvailable = true;
};

struct Style {
    ImFont* font = nullptr;
    float fontSize = 13.0f;
    std::array<ImU32, Count> color = {{ IM_COL32_WHITE, IM_COL32(185, 215, 255, 255), IM_COL32(65, 230, 95, 255), IM_COL32_WHITE }};
    ImU32 boxColor = IM_COL32(255, 65, 75, 255);
    ImU32 textBackground = IM_COL32(14, 18, 25, 185);
};

struct Item {
    Rect rect;
    std::string text;
    int side = Top;
    bool visible = false;
};

struct Result {
    Rect box;
    std::array<Item, Count> items;
    bool valid = false;
    bool relocated = false;
    int hidden = 0;
};

struct Input {
    ImVec2 mouse;
    bool pressed = false;
    bool down = false;
    bool released = false;
    bool cancel = false;
    bool hovered = false;
};

struct Editor {
    Model draft;
    Model origin;
    Model candidate;
    ImVec2 press;
    ImVec2 grab;
    Rect dragBox;
    Rect dragViewport;
    float grabAlong = 0.5f;
    int active = -1;
    int selected = Name;
    int candidateSide = Top;
    bool dirty = false;
    bool originDirty = false;
    bool moved = false;
    bool candidateValid = false;
};

float Clamp(float value, float low, float high, float fallback);
float Snap(float value, float step);
Rect Expand(const Rect& rect, float amount);
bool Intersects(const Rect& first, const Rect& second);
bool IsHorizontal(int side);
Model Preset(int index);
void Normalize(Model& model);
int PickSide(ImVec2 point, const Rect& box, int previous, float hysteresis = 3.0f);
Result Resolve(const Model& model, const Rect& box, const Rect& viewport, const Content& content, const Style& style, bool viewportFallback = false);
bool CheckGeometry(const Result& result, float minimumGap);
int HitTest(const Result& result, ImVec2 point, int selected);
void CancelDrag(Editor& editor);
void Reset(Editor& editor, const Model& model);
Result Update(Editor& editor, const Rect& box, const Rect& viewport, const Content& content, const Style& style, const Input& input);
void Draw(ImDrawList* draw, const Result& result, const Content& content, const Style& style);
void DrawPreview(ImDrawList* draw, const Rect& viewport, const Result& result, const Model& model, const Style& style, int selected, bool dragging, bool validDrop);
const char* ElementName(int element);
const char* SideName(int side);

}
