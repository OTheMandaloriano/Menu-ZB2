#pragma once
namespace Hotkeys {
struct Toggle {
    int key=-1;
    bool previous=false;
    bool Pressed(int next,bool down,bool allowed) {
        if(next!=key){key=next;previous=down;return false;}
        const bool edge=down && !previous;previous=down;
        return allowed && next>0 && next<256 && edge;
    }
};
}
