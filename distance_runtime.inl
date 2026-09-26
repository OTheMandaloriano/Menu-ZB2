struct DistantSnapshot { DistantMarker entries[256] = {}; int count = 0; ULONGLONG time = 0; };
static LatestSnapshot<DistantSnapshot> s_distantSnapshot;
static void ClearDistantEsp() { s_distantSnapshot.Publish(DistantSnapshot{}); }
static void ConfigureAimRange(bool enabled) {
    if (!s_rangeConfigure) return;
    int active=enabled ? 1 : 0, circle=Config::b360Mode ? 1 : 0;
    float distance=AimPolicy().distance, radius=AimPolicy().radius;
    void* args[]={&active,&distance,&circle,&radius,&s_vpW,&s_vpH};
    MonoObject* exception=nullptr;
    pInvoke(s_rangeConfigure,nullptr,args,&exception);
    static long long nextStats=0;
    if(enabled && s_rangeStats && PiNow()>=nextStats) {
        nextStats=PiNow()+5000000;
        auto result=pInvoke(s_rangeStats,nullptr,nullptr,&exception);
        if(result && !exception){char* text=pStrUtf8(result);if(text){Log::Infof("[RANGE] %s",text);pFree(text);}}
    }
}
static void BuildDistantEsp() {
    if(!Config::bZombieEsp || !s_rangeCollect) {ClearDistantEsp();return;}
    DistantSnapshot snapshot;
    void* buffer=snapshot.entries; int capacity=256;
    float distance=Config::fEspDistance;
    void* args[]={&buffer,&capacity,&distance};
    MonoObject* exception=nullptr;
    auto result=pInvoke(s_rangeCollect,nullptr,args,&exception);
    if(result && !exception) {
        auto value=pUnbox(result);
        if(value)memcpy(&snapshot.count,value,sizeof(int));
    }
    if(snapshot.count<0 || snapshot.count>capacity)snapshot.count=0;
    snapshot.time=GetTickCount64();s_distantSnapshot.Publish(snapshot);
    static long long nextLog=0;
    if(PiNow()>=nextLog) {
        nextLog=PiNow()+5000000;
        Log::Infof("[DISTANCE] ESP=%.0fm AIM=%.0fm registros-distantes-visiveis=%d",
            Config::fEspDistance,Config::fAimDistance,snapshot.count);
        MonoObject* error=nullptr;
        auto message=s_rangeError ? pInvoke(s_rangeError,nullptr,nullptr,&error) : nullptr;
        if(message && !error) {char* text=pStrUtf8(message);if(text){if(*text)Log::Warnf("[RANGE] %s",text);pFree(text);}}
    }
}
int GetDistantEsp(DistantMarker* output,int capacity) {
    const auto& snapshot=s_distantSnapshot.Read();
    if(!output || capacity<=0 || GetTickCount64()-snapshot.time>500)return 0;
    int count=(std::min)(capacity,snapshot.count);
    memcpy(output,snapshot.entries,count*sizeof(DistantMarker));return count;
}
