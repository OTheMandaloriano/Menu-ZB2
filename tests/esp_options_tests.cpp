#include "../esp_options.h"
#include <cassert>
#include <iostream>
int main() {
    for(int style=0;style<3;++style) for(bool has3d:{false,true})
        assert(!EspOptions::BoxFallback(false,style,has3d));
    assert(EspOptions::BoxFallback(true,1,false));
    assert(!EspOptions::BoxFallback(true,1,true));
    assert(EspOptions::SnapOriginY(0,1080)==1080);
    assert(EspOptions::SnapOriginY(1,1080)==0);
    assert(EspOptions::SnapOriginY(2,1080)==540);
    assert(EspOptions::SnapOriginY(99,1080)==1080);
    std::cout << "12 ESP option checks passed\n";
}
