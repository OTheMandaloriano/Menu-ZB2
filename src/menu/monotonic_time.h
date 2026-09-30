#pragma once
#include <cstdint>
namespace MonotonicTime {
    inline std::int64_t Microseconds(std::int64_t counter, std::int64_t frequency) {
        if(counter < 0 || frequency <= 0) return 0;
        return (counter / frequency) * 1000000LL +
            static_cast<std::int64_t>((counter % frequency) * (1000000.0L / frequency));
    }
}
