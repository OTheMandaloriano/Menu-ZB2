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
enum class GameMode { Lobby = 0, SinglePlayer = 1, Client = 2, Host = 3 };
inline bool CanRedirectShot(GameMode mode) { return mode == GameMode::SinglePlayer; }
inline bool ShouldMoveVisible(bool firing, bool aimbot, bool automatic, bool silentRequested, bool silentAllowed) {
    return firing && (aimbot || automatic || (silentRequested && !silentAllowed));
}
struct Point { float x, y, z; };
struct Target {
    std::uintptr_t entity = 0;
    Point position = {};
    float pixels = 0, distance = 0, hp = 0;
    float angle = 0;
    bool projected = true;
    std::uintptr_t bone = 0;
};
struct Policy { float distance = 120, radius = 360; int priority = 0; bool fullCircle = false; };
enum class Visibility { Unknown, Clear, TargetHit, Blocked };
inline bool CanAim(Visibility value) {
    return value == Visibility::Clear || value == Visibility::TargetHit;
}
inline bool Valid(const Target& t, const Policy& policy) {
    return t.entity && std::isfinite(t.angle) && t.angle >= 0 && t.angle <= 180 &&
        std::isfinite(t.distance) && t.distance >= 0 &&
        std::isfinite(t.hp) && t.hp > 0 &&
        std::isfinite(t.position.x) && std::isfinite(t.position.y) && std::isfinite(t.position.z) &&
        t.distance <= policy.distance && (policy.fullCircle ||
        (t.projected && std::isfinite(t.pixels) && t.pixels >= 0 &&
         (policy.radius <= 0 || t.pixels <= policy.radius)));
}
inline float Score(const Target& t, const Policy& policy) {
    return policy.priority == 1 ? t.hp : policy.priority == 2 ? t.distance :
        policy.fullCircle ? t.angle : t.pixels;
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
inline std::array<int, MaxTargets> Rank(const Target* targets, int count,
    const Policy& policy, const Lock& lock, std::int64_t now) {
    std::array<int, MaxTargets> result;
    result.fill(-1);
    auto better = [&](int left, int right) {
        if (right < 0) return true;
        const auto& a = targets[left]; const auto& b = targets[right];
        if (lock.Sticky(now) && (a.entity == lock.entity) != (b.entity == lock.entity))
            return a.entity == lock.entity;
        const float sa = Score(a, policy), sb = Score(b, policy);
        return sa < sb || (sa == sb && (a.angle < b.angle || (a.angle == b.angle && a.entity < b.entity)));
    };
    for (int i = 0; i < std::min(count, MaxTargets); ++i) {
        if (!Valid(targets[i], policy)) continue;
        for (int slot = 0; slot < MaxTargets; ++slot) {
            if (!better(i, result[slot])) continue;
            for (int j = MaxTargets - 1; j > slot; --j) result[j] = result[j - 1];
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
inline float Angle(Point forward, Point direction) {
    const float a = forward.x*forward.x + forward.y*forward.y + forward.z*forward.z;
    const float b = direction.x*direction.x + direction.y*direction.y + direction.z*direction.z;
    if (!std::isfinite(a) || !std::isfinite(b) || a < 1e-8f || b < 1e-8f) return -1;
    const float dot = (forward.x*direction.x + forward.y*direction.y + forward.z*direction.z) / std::sqrt(a*b);
    return std::acos((std::max)(-1.0f, (std::min)(1.0f, dot))) * 57.29577951f;
}
inline void KeepBest(Target* targets, int& count, const Target& candidate, const Policy& policy) {
    if (!Valid(candidate, policy)) return;
    if (count < MaxTargets) { targets[count++] = candidate; return; }
    int worst = 0;
    for (int i = 1; i < count; ++i)
        if (Score(targets[i], policy) > Score(targets[worst], policy)) worst = i;
    if (Score(candidate, policy) < Score(targets[worst], policy)) targets[worst] = candidate;
}
inline bool IntersectsViewport(float hx, float hy, float fx, float fy, float width, float height) {
    if (!std::isfinite(hx) || !std::isfinite(hy) || !std::isfinite(fx) || !std::isfinite(fy)) return false;
    const float halfWidth = std::fabs(hy - fy) * 0.45f;
    return std::max(hx, fx) + halfWidth >= 0 && std::min(hx, fx) - halfWidth <= width &&
        std::max(hy, fy) >= 0 && std::min(hy, fy) <= height;
}
}
