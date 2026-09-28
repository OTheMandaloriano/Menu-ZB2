#pragma once
namespace UiCapture {
inline bool Consume(bool open,bool toggle,bool keyboard,bool mouse,bool wantsKeyboard,bool wantsMouse){
    return toggle || (open && ((keyboard && wantsKeyboard)||(mouse && wantsMouse)));
}
}
