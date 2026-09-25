#pragma once
#include <atomic>

// Single producer/single consumer. The consumer owns its slot until its next
// Read(), so a slow frame never races a producer overwriting the same storage.
template<class T> class LatestSnapshot {
    static constexpr unsigned Dirty = 4;
    T slots[3] = {};
    std::atomic<unsigned> middle{1};
    unsigned writer = 2, reader = 0;
public:
    void Publish(const T& value) {
        slots[writer] = value;
        writer = middle.exchange(writer | Dirty, std::memory_order_acq_rel) & 3;
    }
    const T& Read() {
        if (middle.load(std::memory_order_acquire) & Dirty)
            reader = middle.exchange(reader, std::memory_order_acq_rel) & 3;
        return slots[reader];
    }
};
