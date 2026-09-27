#include "../hotkey_toggle.h"
#include <cassert>
#include <iostream>
int main(){
    Hotkeys::Toggle key;
    assert(!key.Pressed(117,false,true));
    assert(key.Pressed(117,true,true));
    for(int i=0;i<100;++i)assert(!key.Pressed(117,true,true));
    assert(!key.Pressed(117,false,true));assert(key.Pressed(117,true,true));
    assert(!key.Pressed(72,true,true));assert(!key.Pressed(72,false,true));
    assert(!key.Pressed(72,true,false));assert(!key.Pressed(72,true,true));
    assert(!key.Pressed(72,false,true));assert(key.Pressed(72,true,true));
    assert(!key.Pressed(0,true,true));assert(!key.Pressed(256,true,true));
    std::cout<<"PASS: hotkey toggles once per press, suppresses held/rebound/focus/menu keys\n";
}
