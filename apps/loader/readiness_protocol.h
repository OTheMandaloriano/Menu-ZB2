#pragma once
#include <Windows.h>
#include <cstdint>
namespace ReadyProtocol {
constexpr uint32_t Magic=0x5a425252,Version=1;
struct alignas(8) State {
    volatile LONG sequence;
    uint32_t magic,version,pid;
    uint64_t processCreated,sampledAt,readySince;
    uint32_t assemblies,mapLoaded,sceneObjects,unsupported;
};
inline uint64_t FileTime(FILETIME time){return (static_cast<uint64_t>(time.dwHighDateTime)<<32)|time.dwLowDateTime;}
inline bool Ready(const State& state,uint32_t pid,uint64_t created,uint64_t now){
    return state.magic==Magic&&state.version==Version&&state.pid==pid&&state.processCreated==created&&
        state.assemblies&&state.mapLoaded&&state.sceneObjects&&!state.unsupported&&state.readySince&&
        now>=state.sampledAt&&now-state.sampledAt<=1000&&now>=state.readySince&&now-state.readySince>=750;
}
}
