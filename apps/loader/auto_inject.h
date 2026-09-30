#pragma once
#include <cstdint>
struct AutoInjectGate {
    uint64_t process=0,probeAttempt=0,menuAttempt=0;
    bool manual=false,failed=false;
    void Observe(uint64_t identity){if(process!=identity){process=identity;probeAttempt=menuAttempt=0;manual=failed=false;}}
    bool Wanted(bool enabled)const{return process&&(enabled||manual);}
    bool CanProbe(bool licensed,bool enabled,bool assemblies,bool loaded)const{return !failed&&licensed&&Wanted(enabled)&&assemblies&&!loaded&&probeAttempt!=process&&menuAttempt!=process;}
    bool CanInject(bool licensed,bool enabled,bool ready,bool loaded)const{return !failed&&licensed&&Wanted(enabled)&&ready&&!loaded&&menuAttempt!=process;}
};
