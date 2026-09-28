#include "../item_search.h"
#include "../ui_input_queue.h"
#include "../ui_capture.h"
#include <cassert>
#include <thread>
#include <atomic>
#include <iostream>
int main(){
    assert(!UiCapture::Consume(false,false,true,false,true,true));
    assert(UiCapture::Consume(false,true,true,false,false,false));
    assert(UiCapture::Consume(true,false,false,true,false,true));
    assert(!UiCapture::Consume(true,false,false,true,false,false));
    assert(!UiCapture::Consume(true,false,true,false,false,true));
    assert(UiCapture::Consume(true,false,true,false,true,false));
    assert(ItemSearch::Matches("Muni\xc3\xa7\xc3\xa3o de pistola","PistolAmmo","municao"));
    assert(ItemSearch::Matches("Metralhadora","GreaseGun","grease gun"));
    assert(ItemSearch::Matches("P\xc3\xa9 de cabra","Crowbar","PE DE CABRA"));
    assert(!ItemSearch::Matches("Madeira","Wood","pistola"));
    UiInput::Queue queue;std::atomic<bool> good{true};
    std::thread producer([&]{for(unsigned i=1;i<=20000;++i){UiInput::Event e={nullptr,WM_CHAR,i,0};while(!queue.Push(e))std::this_thread::yield();}});
    for(unsigned i=1;i<=20000;++i){UiInput::Event e;while(!queue.Pop(e))std::this_thread::yield();if(e.wparam!=i)good=false;}
    producer.join();assert(good);
    std::cout<<"PASS: accented/spaced item search; 20000 queued input events preserved in order\n";
}
