#include "../apps/loader/auto_inject.h"
#include "../apps/loader/readiness.h"
#include "../apps/loader/readiness_protocol.h"
#include <iostream>
#include <stdexcept>
int main(){try{
    int checks=0;auto check=[&](bool value){++checks;if(!value)throw std::runtime_error("auto-inject check failed: "+std::to_string(checks));};
    AutoInjectGate gate;gate.Observe(100);
    check(!gate.CanProbe(false,true,true,false));check(!gate.CanProbe(true,false,true,false));check(!gate.CanProbe(true,true,false,false));
    check(gate.CanProbe(true,true,true,false));gate.probeAttempt=100;check(!gate.CanProbe(true,true,true,false));
    check(!gate.CanInject(true,true,false,false));check(gate.CanInject(true,true,true,false));check(!gate.CanInject(true,false,true,false));
    gate.manual=true;check(gate.CanInject(true,false,true,false));gate.menuAttempt=100;check(!gate.CanInject(true,true,true,false));
    gate.Observe(101);check(!gate.manual&&gate.menuAttempt==0&&gate.probeAttempt==0);gate.failed=true;check(!gate.CanInject(true,true,true,false));
    gate.Observe(0);check(!gate.CanInject(true,true,true,false));gate.Observe(102);check(!gate.CanInject(true,true,true,true));
    LoaderServices::LogState log;log.expected="d:/game/zumbiblocks2_data/managed";
    log.Line("- Loaded All Assemblies, in 1 seconds");check(!log.assemblies);
    log.Line("Mono path[0] = 'D:/game/ZumbiBlocks2_Data/Managed'");log.Line("- Loaded All Assemblies, in 1 seconds");check(log.assemblies&&!log.domain);
    log.Line("- Finished resetting the current domain, in 0 seconds");check(log.domain&&!log.world);
    log.Line("Client > Received entry confirmation. My LobbyID = 2");check(!log.world);
    log.Line("Client > Received hash info");check(log.world);log.Line("Unknown > >>> GAME CLEANUP");check(!log.world);
    log.Line("Match > Started match");check(log.world);log.Line("Begin MonoManager ReloadAssembly");check(!log.world&&!log.domain);
    ReadyProtocol::State state{};state.magic=ReadyProtocol::Magic;state.version=ReadyProtocol::Version;state.pid=22;state.processCreated=123;state.sampledAt=2000;state.readySince=1000;state.assemblies=state.mapLoaded=state.sceneObjects=1;
    check(ReadyProtocol::Ready(state,22,123,2000));check(!ReadyProtocol::Ready(state,23,123,2000));check(!ReadyProtocol::Ready(state,22,124,2000));
    check(!ReadyProtocol::Ready(state,22,123,3001));state.readySince=1800;check(!ReadyProtocol::Ready(state,22,123,2000));
    state.readySince=1000;state.mapLoaded=0;check(!ReadyProtocol::Ready(state,22,123,2000));state.mapLoaded=1;state.unsupported=1;check(!ReadyProtocol::Ready(state,22,123,2000));
    std::cout<<checks<<" auto-inject/readiness checks passed.\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what();return 1;}}
