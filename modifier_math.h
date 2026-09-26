#pragma once

#include <cmath>

namespace ModifierMath {

inline bool Scale(float base, float multiplier, float low, float high, float& output) {
    if (!std::isfinite(base) || !std::isfinite(multiplier) ||
        !(multiplier >= 1.0f && multiplier <= 10.0f) || !(base > low && base < high))
        return false;
    output = base * multiplier;
    return std::isfinite(output) && output > low && output < high;
}

}
