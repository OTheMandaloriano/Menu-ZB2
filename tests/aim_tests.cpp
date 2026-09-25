#include "../aim_logic.h"
#include <cstdio>
#include <cstdlib>
#include <limits>

static int checks;
static void Check(bool ok, const char* message) {
    ++checks;
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
int main() {
    using namespace Aim;
    Policy policy;
    Lock lock;
    Target targets[] = {{1, {}, 90, 4, 100}, {2, {}, 80, 3, 100}, {3, {}, 70, 2, 100},
                        {4, {}, 20, .9f, 100}, {5, {}, 10, .2f, 100}};
    auto ranked = Rank(targets, 5, policy, lock, 1000000);
    Check(ranked[0] == 4 && ranked[1] == 3 && ranked[2] == 2, "top three scan includes final entities");
    Check(Valid(targets[4], policy), "nearby enemy remains eligible");
    lock.Select(1, 1000000);
    Check(Rank(targets, 5, policy, lock, 1200000)[0] == 0, "sticky preserves identity");
    lock.Select(1, 1200000);
    Check(!lock.Sticky(1500000), "same target does not renew sticky forever");
    policy.radius = 50;
    Check(Rank(targets, 5, policy, lock, 1200000)[0] == 4, "sticky cannot bypass radius");
    policy.radius = 0;
    policy.distance = 1;
    Check(Rank(targets, 5, policy, lock, 1200000)[0] == 4, "sticky cannot bypass range");
    targets[4].hp = 0;
    Check(Rank(targets, 5, policy, lock, 1200000)[0] == 3, "dead enemy discarded immediately");
    targets[3].pixels = std::numeric_limits<float>::quiet_NaN();
    Check(Rank(targets, 5, policy, lock, 1200000)[0] == -1, "NaN cannot become target");
    Check(!CanAim(Visibility::Unknown), "physics failure never authorizes aiming through walls");
    Check(!CanAim(Visibility::Blocked), "obstacle blocks aiming");
    Check(CanAim(Visibility::TargetHit), "enemy collider is not mistaken for a wall");
    Check(CanAim(Visibility::Clear), "clear ray permits aiming");
    Activation input;
    Check(!input.Update(true, false, 0, 0, true), "unassigned key never activates aim");
    Check(input.Update(true, true, 2, 0, false), "auto aim works while aimbot enabled and key released");
    Check(!input.Update(false, true, 2, 0, true), "menu/focus gate overrides automatic mode");
    Check(input.Update(true, false, 2, 1, true), "toggle rising edge activates");
    Check(input.Update(true, false, 2, 1, true), "held key does not toggle repeatedly");
    Check(input.Update(true, false, 2, 1, false), "toggle survives release");
    Check(!input.Update(true, false, 2, 1, true), "second press turns toggle off");
    input.Update(false, false, 2, 1, false);
    Check(!input.Update(true, false, 2, 1, false), "disable clears latched state");
    Point direction; float distance;
    Check(Ray({0,0,0}, {0,0,.2f}, direction, distance) && direction.z == 1 && distance == .2f,
          "ray reaches near target without 30cm origin offset or one-meter minimum");
    Check(!Ray({}, {}, direction, distance), "zero-length ray rejected");
    Check(IntersectsViewport(960, 1800, 960, -500, 1920, 1080), "close body crossing viewport stays visible");
    Check(!IntersectsViewport(960, 1800, 960, 1700, 1920, 1080), "fully offscreen body stays culled");
    std::printf("PASS: %d targeting regression checks\n", checks);
}
