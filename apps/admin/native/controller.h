#pragma once
#include "backend.h"
#include <thread>
#include <mutex>
#include <condition_variable>
namespace Admin {
class Controller {
public:
    explicit Controller(std::filesystem::path root);
    ~Controller();
    Snapshot Get();
    bool Submit(Request request);
    void Stop();
private:
    Backend backend_;std::thread thread_;std::mutex mutex_;std::condition_variable wake_;
    bool stop_=false,pending_=true;Request request_{"snapshot",{}};Snapshot state_;
    void Run();
};
}
