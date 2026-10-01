#include "controller.h"
#include "../../loader/license.h"
#include <Windows.h>
namespace Admin {
namespace {void Wipe(Request& request){for(auto& value:request.args){SecureZeroMemory(value.data(),value.size());value.clear();}request.args.clear();}}
Controller::Controller(std::filesystem::path root):backend_(std::move(root)){thread_=std::thread(&Controller::Run,this);}
Controller::~Controller(){Stop();if(thread_.joinable())thread_.join();}
void Controller::Stop(){{std::lock_guard<std::mutex> lock(mutex_);stop_=true;Wipe(request_);}wake_.notify_one();}
Snapshot Controller::Get(){std::lock_guard<std::mutex> lock(mutex_);return state_;}
bool Controller::Submit(Request request){std::lock_guard<std::mutex> lock(mutex_);if(state_.busy||pending_||stop_){Wipe(request);return false;}request_=std::move(request);pending_=true;state_.busy=true;state_.notice.clear();wake_.notify_one();return true;}
void Controller::Run(){
    while(true){Request request;
        {std::unique_lock<std::mutex> lock(mutex_);wake_.wait(lock,[this]{return stop_||pending_;});if(stop_)return;request=std::move(request_);pending_=false;}
        try{auto result=backend_.Call(request);result.device=License::DeviceId();std::lock_guard<std::mutex> lock(mutex_);result.revision=state_.revision+1;state_=std::move(result);}
        catch(const std::exception& error){
            const std::string message=error.what();
            try{auto refreshed=backend_.Call({"snapshot",{}});refreshed.device=License::DeviceId();refreshed.error=true;refreshed.notice=message;std::lock_guard<std::mutex> lock(mutex_);refreshed.revision=state_.revision+1;state_=std::move(refreshed);}
            catch(...){std::lock_guard<std::mutex> lock(mutex_);state_.busy=false;state_.error=true;state_.notice=message;++state_.revision;}
        }
        Wipe(request);
    }
}
}
