#include "../modifier_math.h"
#include <cstdio>
#include <cstdlib>
#include <limits>

static int checks = 0;
static void Check(bool value, const char* message) {
    ++checks;
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
int main() {
    float value = 0;
    Check(ModifierMath::Scale(3.5f, 1.5f, 0, 100, value) && value == 5.25f,
          "walk speed uses a stable positive baseline");
    Check(ModifierMath::Scale(6.0f, 5.0f, 0, 100, value) && value == 30.0f,
          "jump speed respects multiplier");
    Check(ModifierMath::Scale(-10.0f, 5.0f, -1000, 0, value) && value == -50.0f,
          "fall threshold stays negative as the game expects");
    Check(ModifierMath::Scale(9.0f, 2.0f, 0, 100, value) && value == 18.0f,
          "roll speed has an independent target");
    Check(!ModifierMath::Scale(3.5f, 0.5f, 0, 100, value), "below-one multiplier rejected");
    Check(!ModifierMath::Scale(3.5f, 11.0f, 0, 100, value), "unsafe multiplier rejected");
    Check(!ModifierMath::Scale(std::numeric_limits<float>::quiet_NaN(), 2, 0, 100, value), "NaN baseline rejected");
    Check(!ModifierMath::Scale(-10.0f, 2.0f, 0, 100, value), "wrong signed range rejected");
    std::printf("PASS: %d modifier math checks\n", checks);
}
