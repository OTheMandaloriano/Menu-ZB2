#include "esp_layout.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace EspLayout {

static constexpr float kPaddingX = 2.0f;
static constexpr float kPaddingY = 1.0f;
static constexpr float kEpsilon = 0.01f;


float Rect::Width() const { return max.x - min.x; }
float Rect::Height() const { return max.y - min.y; }


bool Rect::Contains(ImVec2 point) const {
    return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y;
}


bool Rect::Contains(const Rect& rect) const {
    return rect.min.x >= min.x - kEpsilon && rect.min.y >= min.y - kEpsilon && rect.max.x <= max.x + kEpsilon && rect.max.y <= max.y + kEpsilon;
}


float Clamp(float value, float low, float high, float fallback) {
    return std::isfinite(value) ? std::max(low, std::min(high, value)) : fallback;
}


float Snap(float value, float step) {
    if (!std::isfinite(value)) return 0.0f;
    if (!std::isfinite(step) || step <= 0.0f) return value;
    return std::round(value / step) * step;
}


Rect Expand(const Rect& rect, float amount) {
    return { ImVec2(rect.min.x - amount, rect.min.y - amount), ImVec2(rect.max.x + amount, rect.max.y + amount) };
}


bool Intersects(const Rect& a, const Rect& b) {
    return a.min.x < b.max.x - kEpsilon && a.max.x > b.min.x + kEpsilon && a.min.y < b.max.y - kEpsilon && a.max.y > b.min.y + kEpsilon;
}


bool IsHorizontal(int side) { return side == Top || side == Bottom; }


const char* ElementName(int element) {
    static const char* names[Count] = { "Nome", "Distancia", "Barra de vida", "Porcentagem" };
    return element >= 0 && element < Count ? names[element] : "Elemento";
}


const char* SideName(int side) {
    static const char* names[4] = { "Acima", "Abaixo", "Esquerda", "Direita" };
    return side >= Top && side <= Right ? names[side] : "Lado";
}


Model Preset(int index) {
    Model model;
    model.items[Name].side = Top;
    model.items[Distance].side = Bottom;
    model.items[Health].side = Left;
    model.items[Percent].side = Right;
    model.items[Name].order = 0;
    model.items[Percent].order = 1;
    model.items[Distance].order = 2;
    model.preset = index;
    if (index == 2) {
        model.items[Health].side = Right;
        model.items[Percent].side = Left;
    } else if (index == 3 || index == 4) {
        model.items[Health].side = index == 3 ? Top : Bottom;
    } else if (index >= 5 && index <= 8) {
        int side = index == 5 ? Left : index == 6 ? Right : index == 7 ? Top : Bottom;
        for (auto& item : model.items) item.side = side;
    } else if (index == 9 || index == 10) {
        model.items[Name].position = index == 9 ? 0.0f : 1.0f;
        model.items[Distance].position = model.items[Name].position;
    }
    return model;
}


void Normalize(Model& model) {
    Model defaults = Preset(1);
    for (int id = 0; id < Count; ++id) {
        auto& item = model.items[id];
        if (item.side < Top || item.side > Right) item.side = defaults.items[id].side;
        item.position = Clamp(item.position, 0.0f, 1.0f, 0.5f);
        item.extraGap = Clamp(item.extraGap, 0.0f, 12.0f, 0.0f);
        item.order = std::max(0, std::min(3, item.order));
    }
    model.gap = Clamp(model.gap, 2.0f, 12.0f, 4.0f);
    model.spacing = Clamp(model.spacing, 2.0f, 8.0f, 3.0f);
    model.barLength = Clamp(model.barLength, 0.25f, 1.0f, 1.0f);
    model.barThickness = Clamp(model.barThickness, 2.0f, 6.0f, 3.0f);
    model.gridSize = Clamp(model.gridSize, 1.0f, 10.0f, 1.0f);
    if (model.preset < 0 || model.preset > 10) model.preset = 0;
    if (model.boxStyle < 0 || model.boxStyle > 2) model.boxStyle = 0;
}


static float SegmentDistance(ImVec2 p, ImVec2 a, ImVec2 b) {
    float dx = b.x - a.x, dy = b.y - a.y;
    float denominator = dx * dx + dy * dy;
    float t = denominator > 0.0f ? Clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / denominator, 0.0f, 1.0f, 0.0f) : 0.0f;
    return std::hypot(p.x - a.x - t * dx, p.y - a.y - t * dy);
}


int PickSide(ImVec2 p, const Rect& box, int previous, float hysteresis) {
    if (!std::isfinite(p.x) || !std::isfinite(p.y) || box.Contains(p)) return -1;
    float distances[4] = {
        SegmentDistance(p, box.min, ImVec2(box.max.x, box.min.y)),
        SegmentDistance(p, ImVec2(box.min.x, box.max.y), box.max),
        SegmentDistance(p, box.min, ImVec2(box.min.x, box.max.y)),
        SegmentDistance(p, ImVec2(box.max.x, box.min.y), box.max)
    };
    bool outside[4] = { p.y < box.min.y, p.y > box.max.y, p.x < box.min.x, p.x > box.max.x };
    int winner = -1;
    for (int side = Top; side <= Right; ++side) {
        if (!outside[side]) continue;
        if (winner < 0 || distances[side] < distances[winner]) winner = side;
    }
    if (previous >= Top && previous <= Right && outside[previous] && winner >= 0 && distances[previous] <= distances[winner] + hysteresis) return previous;
    return winner;
}


struct Measurement {
    std::array<std::string, Count> text;
    std::array<ImVec2, Count> size;
};


static Measurement Measure(const Content& content, const Style& style, const Rect& viewport) {
    Measurement result;
    float maxWidth = std::max(12.0f, std::min(120.0f, viewport.Width() - 16.0f));
    for (int id = 0; id < Count; ++id) {
        if (id == Health) continue;
        std::string text = content.text[id];
        std::replace(text.begin(), text.end(), '\n', ' ');
        std::replace(text.begin(), text.end(), '\r', ' ');
        auto width = [&](const std::string& value) { return style.font->CalcTextSizeA(style.fontSize, std::numeric_limits<float>::max(), 0.0f, value.c_str()).x; };
        if (width(text) > maxWidth) {
            const char* remaining = text.c_str();
            float allowed = std::max(1.0f, maxWidth - width("..."));
            style.font->CalcTextSizeA(style.fontSize, allowed, 0.0f, text.c_str(), nullptr, &remaining);
            text = text.substr(0, static_cast<size_t>(remaining - text.c_str())) + "...";
        }
        result.text[id] = text;
        result.size[id] = ImVec2(width(text) + kPaddingX * 2.0f, style.fontSize + kPaddingY * 2.0f);
    }
    return result;
}


static Rect Place(const Rect& box, int side, float position, ImVec2 size, float gap) {
    float x = box.min.x, y = box.min.y;
    if (IsHorizontal(side)) {
        x += (box.Width() - size.x) * position;
        y = side == Top ? box.min.y - gap - size.y : box.max.y + gap;
    } else {
        x = side == Left ? box.min.x - gap - size.x : box.max.x + gap;
        y += (box.Height() - size.y) * position;
    }
    return { ImVec2(x, y), ImVec2(x + size.x, y + size.y) };
}


static void MoveOut(Rect& rect, int side, float distance) {
    float dx = side == Left ? -distance : side == Right ? distance : 0.0f;
    float dy = side == Top ? -distance : side == Bottom ? distance : 0.0f;
    rect.min.x += dx; rect.max.x += dx;
    rect.min.y += dy; rect.max.y += dy;
}


static bool AllFit(const Result& result, const Rect& viewport) {
    for (const auto& item : result.items) if (item.visible && !viewport.Contains(item.rect)) return false;
    return true;
}


static Result ResolveMeasured(const Model& model, const Rect& box, const Rect& viewport, const Content& content, const Measurement& measured) {
    Result result;
    result.box = box;
    std::array<bool, Count> placed = {};
    auto enabled = [&](int id) { return model.items[id].enabled && ((id == Health || id == Percent) ? content.healthAvailable : !measured.text[id].empty()); };
    if (enabled(Health)) {
        const auto& placement = model.items[Health];
        bool horizontal = IsHorizontal(placement.side);
        ImVec2 size = horizontal ? ImVec2(box.Width() * model.barLength, model.barThickness) : ImVec2(model.barThickness, box.Height() * model.barLength);
        result.items[Health] = { Place(box, placement.side, placement.position, size, model.gap + placement.extraGap), "", placement.side, true };
        placed[Health] = true;
    }
    std::array<int, 3> order = {{ Name, Percent, Distance }};
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return model.items[a].order < model.items[b].order; });
    for (int first : order) {
        if (placed[first] || !enabled(first)) continue;
        const auto& anchor = model.items[first];
        std::vector<int> group;
        for (int id : order) {
            if (placed[id] || !enabled(id)) continue;
            if (id == first || (model.stack && model.items[id].side == anchor.side && std::fabs(model.items[id].position - anchor.position) < 0.025f)) group.push_back(id);
        }
        bool row = IsHorizontal(anchor.side) && model.horizontalText;
        ImVec2 size(0.0f, 0.0f);
        float extraGap = 0.0f;
        for (int id : group) {
            const auto& extent = measured.size[id];
            if (row) { size.x += extent.x; size.y = std::max(size.y, extent.y); }
            else { size.x = std::max(size.x, extent.x); size.y += extent.y; }
            extraGap = std::max(extraGap, model.items[id].extraGap);
        }
        if (row) size.x += model.spacing * static_cast<float>(group.size() - 1);
        else size.y += model.spacing * static_cast<float>(group.size() - 1);
        Rect bounds = Place(box, anchor.side, anchor.position, size, model.gap + extraGap);
        for (int iteration = 0; iteration < Count * 2; ++iteration) {
            float shift = 0.0f;
            for (int id = 0; id < Count; ++id) {
                if (!placed[id] || !Intersects(bounds, Expand(result.items[id].rect, model.spacing))) continue;
                const auto& obstacle = result.items[id].rect;
                float delta = anchor.side == Top ? bounds.max.y - obstacle.min.y + model.spacing :
                    anchor.side == Bottom ? obstacle.max.y - bounds.min.y + model.spacing :
                    anchor.side == Left ? bounds.max.x - obstacle.min.x + model.spacing : obstacle.max.x - bounds.min.x + model.spacing;
                shift = std::max(shift, delta);
            }
            if (shift <= kEpsilon) break;
            MoveOut(bounds, anchor.side, shift);
        }
        float cursor = 0.0f;
        for (int id : group) {
            ImVec2 extent = measured.size[id];
            float x = bounds.min.x, y = bounds.min.y;
            if (row) x += cursor;
            else {
                y += cursor;
                float t = anchor.side == Left ? 1.0f : anchor.side == Right ? 0.0f : anchor.position;
                x += (size.x - extent.x) * t;
            }
            result.items[id] = { { ImVec2(x, y), ImVec2(x + extent.x, y + extent.y) }, measured.text[id], anchor.side, true };
            placed[id] = true;
            cursor += (row ? extent.x : extent.y) + model.spacing;
        }
    }
    result.valid = CheckGeometry(result, model.gap) && AllFit(result, viewport);
    return result;
}


bool CheckGeometry(const Result& result, float minimumGap) {
    Rect reserved = Expand(result.box, minimumGap);
    for (int first = 0; first < Count; ++first) {
        const auto& item = result.items[first];
        if (!item.visible) continue;
        if (!std::isfinite(item.rect.min.x) || !std::isfinite(item.rect.min.y) || !std::isfinite(item.rect.max.x) || !std::isfinite(item.rect.max.y)) return false;
        if (item.rect.Width() <= 0 || item.rect.Height() <= 0 || Intersects(item.rect, reserved)) return false;
        for (int second = first + 1; second < Count; ++second) if (result.items[second].visible && Intersects(item.rect, result.items[second].rect)) return false;
    }
    return true;
}


Result Resolve(const Model& input, const Rect& box, const Rect& viewport, const Content& content, const Style& style, bool viewportFallback) {
    Result empty;
    empty.box = box;
    if (!style.font || !std::isfinite(style.fontSize) || style.fontSize <= 0.0f ||
        !std::isfinite(box.min.x) || !std::isfinite(box.min.y) || !std::isfinite(box.max.x) || !std::isfinite(box.max.y) ||
        !std::isfinite(viewport.min.x) || !std::isfinite(viewport.min.y) || !std::isfinite(viewport.max.x) || !std::isfinite(viewport.max.y) ||
        box.Width() < 2.0f || box.Height() < 2.0f || viewport.Width() < 4.0f || viewport.Height() < 4.0f) return empty;
    Model model = input;
    Normalize(model);
    Measurement measured = Measure(content, style, viewport);
    Result result = ResolveMeasured(model, box, viewport, content, measured);
    if (!viewportFallback || result.valid) return result;
    auto score = [&](const Result& candidate) {
        const int weights[Count] = { 8, 1, 16, 2 };
        int sum = 0;
        for (int id = 0; id < Count; ++id) if (candidate.items[id].visible && viewport.Contains(candidate.items[id].rect)) sum += weights[id];
        return sum;
    };
    bool relocated = false;
    for (int id : { Health, Name, Percent, Distance }) {
        if (!result.items[id].visible || viewport.Contains(result.items[id].rect)) continue;
        int bestScore = score(result);
        Model bestModel = model;
        Result best = result;
        int opposite = model.items[id].side ^ 1;
        for (int side : std::array<int, 5>{{ opposite, Top, Bottom, Left, Right }}) {
            if (side == model.items[id].side) continue;
            Model trial = model;
            trial.items[id].side = side;
            Result candidate = ResolveMeasured(trial, box, viewport, content, measured);
            if (CheckGeometry(candidate, model.gap) && score(candidate) > bestScore) {
                best = candidate; bestModel = trial; bestScore = score(candidate);
            }
        }
        if (bestModel.items[id].side != model.items[id].side) relocated = true;
        result = best; model = bestModel;
    }
    for (auto& item : result.items) {
        if (item.visible && !viewport.Contains(item.rect)) { item.visible = false; ++result.hidden; }
    }
    result.relocated = relocated;
    result.valid = CheckGeometry(result, model.gap) && AllFit(result, viewport);
    return result;
}


int HitTest(const Result& result, ImVec2 point, int selected) {
    if (selected >= 0 && selected < Count && result.items[selected].visible && result.items[selected].rect.Contains(point)) return selected;
    for (int id = Count - 1; id >= 0; --id) if (result.items[id].visible && result.items[id].rect.Contains(point)) return id;
    int best = -1;
    float bestDistance = std::numeric_limits<float>::max();
    for (int id = 0; id < Count; ++id) {
        const auto& item = result.items[id];
        if (!item.visible || !Expand(item.rect, 4.0f).Contains(point)) continue;
        float distance = std::hypot(point.x - (item.rect.min.x + item.rect.max.x) * 0.5f, point.y - (item.rect.min.y + item.rect.max.y) * 0.5f);
        if (distance < bestDistance) { best = id; bestDistance = distance; }
    }
    return best;
}


void CancelDrag(Editor& editor) {
    if (editor.active >= 0) { editor.draft = editor.origin; editor.dirty = editor.originDirty; }
    editor.active = -1;
    editor.moved = false;
    editor.candidateValid = false;
}


void Reset(Editor& editor, const Model& model) {
    editor = Editor();
    editor.draft = model;
    Normalize(editor.draft);
}


static bool SameRect(const Rect& a, const Rect& b) {
    return std::fabs(a.min.x - b.min.x) < 0.1f && std::fabs(a.min.y - b.min.y) < 0.1f && std::fabs(a.max.x - b.max.x) < 0.1f && std::fabs(a.max.y - b.max.y) < 0.1f;
}


Result Update(Editor& editor, const Rect& box, const Rect& viewport, const Content& content, const Style& style, const Input& input) {
    if (input.cancel || (editor.active >= 0 && (!SameRect(editor.dragBox, box) || !SameRect(editor.dragViewport, viewport)))) CancelDrag(editor);
    Result current = Resolve(editor.draft, box, viewport, content, style);
    if (input.pressed && input.hovered && !input.cancel && editor.active < 0 && current.valid) {
        int id = HitTest(current, input.mouse, editor.selected);
        if (id >= 0) {
            editor.active = id; editor.selected = id;
            editor.origin = editor.draft; editor.candidate = editor.draft;
            editor.originDirty = editor.dirty;
            editor.press = input.mouse; editor.dragBox = box; editor.dragViewport = viewport;
            const auto& item = current.items[id];
            editor.grab = ImVec2(Clamp((input.mouse.x - item.rect.min.x) / item.rect.Width(), 0.0f, 1.0f, 0.5f), Clamp((input.mouse.y - item.rect.min.y) / item.rect.Height(), 0.0f, 1.0f, 0.5f));
            editor.grabAlong = IsHorizontal(item.side) ? editor.grab.x : editor.grab.y;
            editor.candidateSide = item.side; editor.candidateValid = true; editor.moved = false;
        }
    }
    if (editor.active < 0) return current;
    if (!editor.moved && std::hypot(input.mouse.x - editor.press.x, input.mouse.y - editor.press.y) > 2.0f) editor.moved = true;
    if (editor.moved) {
        int side = viewport.Contains(input.mouse) ? PickSide(input.mouse, box, editor.candidateSide) : -1;
        editor.candidateValid = side >= 0;
        if (side >= 0) {
            Model candidate = editor.origin;
            auto& item = candidate.items[editor.active];
            item.side = side;
            Result probe = Resolve(candidate, box, viewport, content, style);
            const auto& measured = probe.items[editor.active].rect;
            bool horizontal = IsHorizontal(side);
            float occupied = horizontal ? measured.Width() : measured.Height();
            float available = (horizontal ? box.Width() : box.Height()) - occupied;
            float grab = editor.active == Health ? editor.grabAlong : horizontal ? editor.grab.x : editor.grab.y;
            float offset = (horizontal ? input.mouse.x - box.min.x : input.mouse.y - box.min.y) - grab * occupied;
            if (candidate.grid) offset = Snap(offset, candidate.gridSize);
            item.position = std::fabs(available) > 0.01f ? Clamp(offset / available, 0.0f, 1.0f, 0.5f) : 0.5f;
            for (float anchor : { 0.0f, 0.5f, 1.0f }) if (std::fabs(item.position - anchor) < 0.065f) item.position = anchor;
            candidate.preset = 0;
            Result resolved = Resolve(candidate, box, viewport, content, style);
            editor.candidateValid = resolved.valid;
            editor.candidate = candidate; editor.candidateSide = side;
            if (resolved.valid) current = resolved;
        }
    }
    if (input.released || !input.down) {
        if (editor.moved && editor.candidateValid) { editor.draft = editor.candidate; editor.dirty = true; editor.active = -1; editor.moved = false; }
        else CancelDrag(editor);
        return Resolve(editor.draft, box, viewport, content, style);
    }
    return current;
}


void Draw(ImDrawList* draw, const Result& result, const Content& content, const Style& style) {
    float health = Clamp(content.health, 0.0f, 1.0f, 0.0f);
    for (int id = 0; id < Count; ++id) {
        const auto& item = result.items[id];
        if (!item.visible) continue;
        if (id == Health) {
            draw->AddRectFilled(item.rect.min, item.rect.max, IM_COL32(38, 47, 43, 255));
            if (health <= 0.0f) continue;
            Rect fill = item.rect;
            if (IsHorizontal(item.side)) fill.max.x = fill.min.x + fill.Width() * health;
            else fill.min.y = fill.max.y - fill.Height() * health;
            draw->AddRectFilled(fill.min, fill.max, style.color[Health]);
        } else {
            if ((style.textBackground >> IM_COL32_A_SHIFT) != 0) draw->AddRectFilled(item.rect.min, item.rect.max, style.textBackground, 1.0f);
            draw->AddText(style.font, style.fontSize, ImVec2(item.rect.min.x + kPaddingX, item.rect.min.y + kPaddingY), style.color[id], item.text.c_str());
        }
    }
}


void DrawPreview(ImDrawList* draw, const Rect& viewport, const Result& result, const Model& model, const Style& style, int selected, bool dragging, bool validDrop) {
    draw->AddRectFilled(viewport.min, viewport.max, IM_COL32(12, 15, 21, 255));
    draw->PushClipRect(viewport.min, viewport.max, true);
    const auto& b = result.box;
    ImVec2 center((b.min.x + b.max.x) * 0.5f, (b.min.y + b.max.y) * 0.5f);
    if (model.guides) {
        draw->AddLine(ImVec2(center.x, viewport.min.y), ImVec2(center.x, viewport.max.y), IM_COL32(32, 39, 52, 255));
        draw->AddLine(ImVec2(viewport.min.x, center.y), ImVec2(viewport.max.x, center.y), IM_COL32(32, 39, 52, 255));
    }
    float w = b.Width(), h = b.Height();
    ImU32 silhouette = IM_COL32(58, 69, 87, 255);
    draw->AddCircleFilled(ImVec2(center.x, b.min.y + h * 0.19f), w * 0.10f, silhouette, 20);
    draw->AddLine(ImVec2(center.x, b.min.y + h * 0.31f), ImVec2(center.x, b.min.y + h * 0.59f), silhouette, w * 0.13f);
    draw->AddLine(ImVec2(b.min.x + w * 0.30f, b.min.y + h * 0.51f), ImVec2(center.x, b.min.y + h * 0.32f), silhouette, w * 0.06f);
    draw->AddLine(ImVec2(center.x, b.min.y + h * 0.32f), ImVec2(b.min.x + w * 0.70f, b.min.y + h * 0.51f), silhouette, w * 0.06f);
    draw->AddLine(ImVec2(b.min.x + w * 0.36f, b.min.y + h * 0.87f), ImVec2(center.x, b.min.y + h * 0.59f), silhouette, w * 0.08f);
    draw->AddLine(ImVec2(center.x, b.min.y + h * 0.59f), ImVec2(b.min.x + w * 0.64f, b.min.y + h * 0.87f), silhouette, w * 0.08f);
    if (model.boxStyle == 2) {
        float length = w * 0.25f;
        for (int x = 0; x < 2; ++x) for (int y = 0; y < 2; ++y) {
            ImVec2 p(x ? b.max.x : b.min.x, y ? b.max.y : b.min.y);
            draw->AddLine(p, ImVec2(p.x + (x ? -length : length), p.y), style.boxColor, 1.5f);
            draw->AddLine(p, ImVec2(p.x, p.y + (y ? -length : length)), style.boxColor, 1.5f);
        }
    } else if (model.boxStyle == 1) {
        float dx = w * 0.16f, dy = h * 0.06f;
        Rect front = { ImVec2(b.min.x, b.min.y + dy), ImVec2(b.max.x - dx, b.max.y) };
        Rect back = { ImVec2(b.min.x + dx, b.min.y), ImVec2(b.max.x, b.max.y - dy) };
        draw->AddRect(front.min, front.max, style.boxColor);
        draw->AddRect(back.min, back.max, style.boxColor);
        draw->AddLine(front.min, back.min, style.boxColor);
        draw->AddLine(front.max, back.max, style.boxColor);
        draw->AddLine(ImVec2(front.max.x, front.min.y), ImVec2(back.max.x, back.min.y), style.boxColor);
        draw->AddLine(ImVec2(front.min.x, front.max.y), ImVec2(back.min.x, back.max.y), style.boxColor);
    } else draw->AddRect(b.min, b.max, style.boxColor, 0.0f, 0, 1.5f);
    // Preview skeleton: mesma topologia do jogo (gui.cpp SEG 19 segmentos).
    // Antes era 1 linha rosa vertical — errado. Agora desenha coluna, pernas
    // e bracos articulados nas proporcoes da auditoria [BONE]/[JOINT] 14/09,
    // na cor do box (igual ao jogo usa colVis/colInv).
    if (model.skeleton) {
        float cx = center.x, top = b.min.y;
        ImVec2 P[20];
        P[0]  = ImVec2(cx, top + h * 0.12f);              // HEAD
        P[1]  = ImVec2(cx, top + h * 0.22f);              // NECK
        P[2]  = ImVec2(cx, top + h * 0.30f);              // SP3 (ombros)
        P[3]  = ImVec2(cx, top + h * 0.38f);              // SP2
        P[4]  = ImVec2(cx, top + h * 0.46f);              // SP1 (cintura)
        P[5]  = ImVec2(cx, top + h * 0.54f);              // HL (pelvis)
        P[6]  = ImVec2(cx - w * 0.10f, top + h * 0.60f);  // L1L quadril L
        P[7]  = ImVec2(cx - w * 0.12f, top + h * 0.74f);  // L2L joelho L
        P[8]  = ImVec2(cx - w * 0.13f, top + h * 0.88f);  // FL pe L
        P[9]  = ImVec2(cx + w * 0.10f, top + h * 0.60f);  // L1R
        P[10] = ImVec2(cx + w * 0.12f, top + h * 0.74f);  // L2R
        P[11] = ImVec2(cx + w * 0.13f, top + h * 0.88f);  // FR
        P[12] = ImVec2(cx - w * 0.18f, top + h * 0.31f);  // SL ombro L
        P[13] = ImVec2(cx - w * 0.26f, top + h * 0.42f);  // A1L cotovelo L
        P[14] = ImVec2(cx - w * 0.24f, top + h * 0.53f);  // A2L pulso L
        P[15] = ImVec2(cx + w * 0.18f, top + h * 0.31f);  // SR
        P[16] = ImVec2(cx + w * 0.26f, top + h * 0.42f);  // A1R
        P[17] = ImVec2(cx + w * 0.24f, top + h * 0.53f);  // A2R
        // Maos estimadas: extensao do antebraco (igual HL2L/HL2R do jogo).
        P[18] = ImVec2(P[14].x + (P[14].x - P[13].x) * 0.4f, P[14].y + (P[14].y - P[13].y) * 0.4f);
        P[19] = ImVec2(P[17].x + (P[17].x - P[16].x) * 0.4f, P[17].y + (P[17].y - P[16].y) * 0.4f);
        static const int SEG[][2] = {
            {0,1},{1,2},{2,3},{3,4},{4,5},{5,6},{6,7},{7,8},{5,9},{9,10},{10,11},
            {2,12},{12,13},{13,14},{2,15},{15,16},{16,17},{14,18},{17,19}
        };
        for (int s = 0; s < 19; ++s)
            draw->AddLine(P[SEG[s][0]], P[SEG[s][1]], style.boxColor, 1.5f);
        draw->AddCircle(P[0], w * 0.10f, style.boxColor, 20, 1.5f); // cranio
    }
    if (model.headDot) {
        float hr = h * 0.03f; if (hr < 2.0f) hr = 2.0f; if (hr > 6.0f) hr = 6.0f;
        draw->AddCircleFilled(ImVec2(center.x, b.min.y + h * 0.12f), hr, style.boxColor);
    }
    // Snapline do preview: base do canvas -> pe do box, na cor do box
    // (antes era azul fixo; no jogo usa colVis/colInv = boxColor do preview).
    if (model.snapline) draw->AddLine(ImVec2(center.x, viewport.max.y), ImVec2(center.x, b.max.y), style.boxColor, 1.0f);
    if (dragging && !validDrop) {
        draw->AddRectFilled(b.min, b.max, IM_COL32(190, 60, 75, 22));
        draw->AddRect(b.min, b.max, IM_COL32(255, 120, 100, 255));
    }
    if (selected >= 0 && selected < Count && result.items[selected].visible) {
        Rect selection = Expand(result.items[selected].rect, 1.0f);
        draw->AddRect(selection.min, selection.max, dragging ? IM_COL32(180, 145, 255, 255) : IM_COL32(92, 112, 143, 160), 1.0f);
    }
    draw->PopClipRect();
}

}
