#pragma once
#include "services.h"
namespace LoaderServices {
struct LogState {
    bool session=false,assemblies=false,domain=false,world=false;
    std::string expected;
    void Line(const std::string& line);
};
class Readiness {
public:
    bool Poll(const Process& process);
    bool World() const{return caughtUp_&&state_.world;}
private:
    uint64_t identity_=0,fileId_=0,offset_=0;
    LogState state_;
    std::string pending_;
    bool caughtUp_=false;
};
}
