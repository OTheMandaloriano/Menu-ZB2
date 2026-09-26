#include "../runtime_settings.h"
#include "../runtime_gate.h"
#include <atomic>
#include <thread>
#include <cstdio>

int main() {
    RuntimeSettings::Channel channel;
    RuntimeSettings::Values initial;
    initial.fAimDistance=120;initial.fEspDistance=200;initial.bZombieEsp=true;
    channel.Publish(initial);
    std::atomic<bool> busy{false},release{false};
    bool stable=false;
    std::thread game([&]{
        const auto local=channel.Read();
        busy=true;
        while(!release.load())std::this_thread::yield();
        stable=local.fAimDistance==120 && local.fEspDistance==200;
        const auto next=channel.Read();
        stable=stable && next.fAimDistance==40 && next.fEspDistance==60 && !next.bZombieEsp;
    });
    while(!busy.load())std::this_thread::yield();
    int rendered=0;
    for(int frame=0;frame<10000;++frame) {
        RuntimeGate::TryScope ui;
        if(!ui){release=true;game.join();return 1;}
        auto next=initial;next.fAimDistance=40;next.fEspDistance=60;next.bZombieEsp=false;
        channel.Publish(next);++rendered;
    }
    release=true;game.join();
    if(!stable || rendered!=10000)return 2;
    std::puts("PASS: 10000 render frames while Unity is busy; immutable settings and latest slider/toggle publication");
}
