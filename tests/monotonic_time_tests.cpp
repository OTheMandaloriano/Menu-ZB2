#include "../src/menu/monotonic_time.h"
#include <cassert>
#include <iostream>
int main() {
    using MonotonicTime::Microseconds;
    constexpr long long frequency=10000000;
    assert(Microseconds(9267519994267LL,frequency)==926751999426LL);
    assert(Microseconds(9223372036854LL,frequency)<Microseconds(9223372036864LL,frequency));
    assert(Microseconds(30LL*86400*frequency,frequency)==30LL*86400*1000000);
    assert(Microseconds(365LL*86400*frequency,frequency)==365LL*86400*1000000);
    assert(Microseconds(0,frequency)==0);
    assert(Microseconds(12345678,frequency)==1234567);
    assert(Microseconds(123,0)==0);
    assert(Microseconds(-1,frequency)==0);
    std::cout<<"PASS: 8 monotonic clock checks, including overflow boundary and 365 days\n";
}
