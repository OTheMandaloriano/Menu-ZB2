#pragma once
namespace EspOptions {
    inline float SnapOriginY(int origin, float height) {
        return origin == 1 ? 0.0f : origin == 2 ? height * 0.5f : height;
    }
    inline bool BoxFallback(bool enabled, int style, bool has3d) {
        return enabled && style == 1 && !has3d;
    }
}
