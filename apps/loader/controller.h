#pragma once
#include "services.h"
#include <condition_variable>
#include <mutex>
#include <thread>
#include <atomic>
enum class LoaderPhase {Checking,Activation,Waiting,Ready,Loading,Success,Error};
struct LoaderSnapshot {
    LoaderPhase phase=LoaderPhase::Checking;
    bool licensed=false,autoInject=true,sceneReady=false;
    unsigned long pid=0;
    std::string device,version,expiry,message=u8"Verificando licença e pacote...";
};
class LoaderController {
public:
    LoaderController();
    ~LoaderController();
    LoaderSnapshot Snapshot();
    bool Activate(std::string token);
    bool Load();
    bool Retry();
    void RequestStop();
    void SetAutoInject(bool enabled);
private:
    enum class Action {None,Activate,Load,Retry};
    void Run();
    void Publish(LoaderSnapshot value);
    std::mutex mutex_;
    std::condition_variable wake_;
    std::thread worker_;
    bool stop_=false,busy_=true;
    bool preferencesDirty_=false;
    std::atomic<bool> autoInject_{true};
    Action pending_=Action::None;
    std::string token_;
    LoaderSnapshot snapshot_;
};
