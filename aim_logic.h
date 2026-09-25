#pragma once

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>

// Deterministic targeting rules. No game pointers are dereferenced here.
namespace Aim {
constexpr int MaxTargets = 128;
constexpr int VisibilityBudget = 3;
constexpr float NearPlane = 0.05f;
struct Point { float x, y, z; };
struct Target {
    std::uintptr_t entity = 0;
    Point position = {};
    float pixels = 0, distance = 0, hp = 0;
};
struct Policy { float distance = 120, radius = 360; int priority = 0; };
enum class Visibility { Unknown, Clear, TargetHit, Blocked };
inline bool CanAim(Visibility value) {
    return value == Visibility::Clear || value == Visibility::TargetHit;
}
inline bool Valid(const Target& t, const Policy& policy) {
    return t.entity && std::isfinite(t.pixels) && t.pixels >= 0 &&
        std::isfinite(t.distance) && t.distance >= 0 &&
        std::isfinite(t.hp) && t.hp > 0 &&
        std::isfinite(t.position.x) && std::isfinite(t.position.y) && std::isfinite(t.position.z) &&
        t.distance <= policy.distance && (policy.radius <= 0 || t.pixels <= policy.radius);
}
inline float Score(const Target& t, int priority) {
    return priority == 1 ? t.hp : priority == 2 ? t.distance : t.pixels;
}
struct Lock {
    std::uintptr_t entity = 0;
    std::int64_t since = 0;
    void Clear() { entity = 0; since = 0; }
    bool Sticky(std::int64_t now) const { return entity && now >= since && now - since < 500000; }
    void Select(std::uintptr_t next, std::int64_t now) {
        if (entity != next) { entity = next; since = now; }
    }
};
inline std::array<int, VisibilityBudget> Rank(const Target* targets, int count,
    const Policy& policy, const Lock& lock, std::int64_t now) {
    std::array<int, VisibilityBudget> result = { -1, -1, -1 };
    auto better = [&](int left, int right) {
        if (right < 0) return true;
        const auto& a = targets[left]; const auto& b = targets[right];
        if (lock.Sticky(now) && (a.entity == lock.entity) != (b.entity == lock.entity))
            return a.entity == lock.entity;
        const float sa = Score(a, policy.priority), sb = Score(b, policy.priority);
        return sa < sb || (sa == sb && (a.pixels < b.pixels || (a.pixels == b.pixels && a.entity < b.entity)));
    };
    for (int i = 0; i < std::min(count, MaxTargets); ++i) {
        if (!Valid(targets[i], policy)) continue;
        for (int slot = 0; slot < VisibilityBudget; ++slot) {
            if (!better(i, result[slot])) continue;
            for (int j = VisibilityBudget - 1; j > slot; --j) result[j] = result[j - 1];
            result[slot] = i;
            break;
        }
    }
    return result;
}
struct Activation {
    int key = -1, mode = -1;
    bool latched = false, previous = false;
    void Reset() { *this = Activation(); }
    bool Update(bool enabled, bool automatic, int nextKey, int nextMode, bool down) {
        if (!enabled) { Reset(); return false; }
        if (key != nextKey || mode != nextMode) { Reset(); key = nextKey; mode = nextMode; }
        down = nextKey > 0 && nextKey <= 255 && down;
        if (mode == 1 && down && !previous) latched = !latched;
        previous = down;
        return automatic || (mode == 1 ? latched : down);
    }
};
inline bool Ray(Point from, Point to, Point& direction, float& distance) {
    direction = {to.x - from.x, to.y - from.y, to.z - from.z};
    distance = std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
    if (!std::isfinite(distance) || distance <= 0.01f || distance > 10000) return false;
    direction.x /= distance; direction.y /= distance; direction.z /= distance;
    return true;
}
inline bool IntersectsViewport(float hx, float hy, float fx, float fy, float width, float height) {
    if (!std::isfinite(hx) || !std::isfinite(hy) || !std::isfinite(fx) || !std::isfinite(fy)) return false;
    const float halfWidth = std::fabs(hy - fy) * 0.45f;
    return std::max(hx, fx) + halfWidth >= 0 && std::min(hx, fx) - halfWidth <= width &&
        std::max(hy, fy) >= 0 && std::min(hy, fy) <= height;
}
}
