#pragma once
#include <Windows.h>
#include <array>
#include <mutex>
namespace UiInput {
struct Event {HWND window;UINT message;WPARAM wparam;LPARAM lparam;};
class Queue {
    std::mutex gate;
    std::array<Event,512> entries{};
    unsigned head=0,count=0;
public:
    bool Push(Event value){std::lock_guard<std::mutex> lock(gate);if(count==entries.size())return false;entries[(head+count)%entries.size()]=value;++count;return true;}
    bool Pop(Event& value){std::lock_guard<std::mutex> lock(gate);if(!count)return false;value=entries[head];head=(head+1)%entries.size();--count;return true;}
};
}
