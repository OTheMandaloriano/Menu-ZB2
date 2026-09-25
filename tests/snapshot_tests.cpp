#include "../latest_snapshot.h"
#include <array>
#include <atomic>
#include <cstdio>
#include <thread>

struct Frame { unsigned sequence = 0; std::array<unsigned,128> data = {}; };
int main() {
    LatestSnapshot<Frame> snapshot;
    std::atomic<bool> done{false};
    std::thread producer([&] {
        for (unsigned i = 1; i <= 100000; ++i) {
            Frame frame; frame.sequence = i; frame.data.fill(i);
            snapshot.Publish(frame);
        }
        done.store(true, std::memory_order_release);
    });
    bool ok = true;
    unsigned last = 0;
    do {
        const auto& frame = snapshot.Read();
        std::this_thread::yield(); // producer can publish repeatedly during this read
        for (auto value : frame.data) if (value != frame.sequence) ok = false;
        if (frame.sequence < last) ok = false;
        last = frame.sequence;
    } while (!done.load(std::memory_order_acquire));
    producer.join();
    if (snapshot.Read().sequence != 100000) ok = false;
    snapshot.Publish(Frame{});
    if (snapshot.Read().sequence != 0) ok = false;
    std::printf("%s: snapshot ownership, monotonic frames and scene clearing\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
