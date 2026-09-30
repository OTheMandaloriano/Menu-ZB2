#include "../src/menu/runtime_gate.h"
#include <thread>
#include <cstdio>

int main() {
    bool contenderSkipped = false;
    RuntimeGate::TryScope frame;
    if (!frame) return 1;
    std::thread game([&] { RuntimeGate::TryScope update; contenderSkipped = !update; });
    game.join(); // would deadlock if TryScope accidentally waited for frame
    if (!contenderSkipped) return 2;
    { RuntimeGate::TryScope reentrant; if (reentrant) return 3; }
    frame.Release();
    { RuntimeGate::TryScope next; if (!next) return 4; }
    { RuntimeGate::TryScope afterScope; if (!afterScope) return 5; }
    std::puts("PASS: render/game contention skips without waiting; scope releases ownership");
}
