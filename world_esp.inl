// Producer: Unity Update. Consumer: Present. No managed references cross threads.
struct WorldSnapshot { WorldMarker entries[256] = {}; int count = 0; ULONGLONG time = 0; };
static LatestSnapshot<WorldSnapshot> s_worldSnapshot;
static void ClearWorldEsp() { s_worldSnapshot.Publish(WorldSnapshot{}); }
static void BridgeError(MonoMethod* method, const char* tag) {
    static long long nextModifier = 0, nextWorld = 0;
    auto& next = method == s_modifierError ? nextModifier : nextWorld;
    if (!method || PiNow() < next) return;
    next = PiNow() + 5000000;
    MonoObject* exception = nullptr;
    auto result = pInvoke(method, nullptr, nullptr, &exception);
    if (!result || exception) return;
    char* message = pStrUtf8(result);
    if (message) { if (*message) Log::Warnf("[%s] %s", tag, message); pFree(message); }
}
static void BuildWorldEsp() {
    int mask=0;
    const bool enabled[] = {s_options.bItemEsp && s_options.bItemWeapons, s_options.bItemEsp && s_options.bItemRare,
        s_options.bItemEsp && s_options.bItemAmmo, s_options.bItemEsp && s_options.bItemSupply,
        s_options.bPoiEsp && s_options.bPoiHeli, s_options.bPoiEsp && s_options.bPoiBoss,
        s_options.bPoiEsp && s_options.bPoiMission, false,
        s_options.bPoiEsp && s_options.bPoiLootFix, s_options.bPoiEsp && s_options.bPoiBench,
        s_options.bPoiEsp && s_options.bPoiFire, s_options.bPoiEsp && s_options.bPoiShop,
        s_options.bPoiEsp && s_options.bPoiRespawn};
    for(int i=0;i<13;++i) if(enabled[i]) mask |= 1<<i;
    if (!mask || !s_worldCollect) { ClearWorldEsp(); return; }
    WorldSnapshot snapshot;
    void* buffer=snapshot.entries;
    int capacity=256;
    void* args[]={&buffer,&capacity,&mask,&s_options.fItemRadius,&s_options.fPoiRadius};
    MonoObject* exception=nullptr;
    auto result=pInvoke(s_worldCollect,nullptr,args,&exception);
    if(result && !exception) {
        auto value=pUnbox(result);
        if(value) memcpy(&snapshot.count,value,sizeof(int));
    }
    if(snapshot.count<0 || snapshot.count>capacity) snapshot.count=0;
    snapshot.time=GetTickCount64();
    s_worldSnapshot.Publish(snapshot);
    BridgeError(s_worldError,"WORLD-ESP");
}
int GetWorldEsp(WorldMarker* output, int capacity) {
    const auto& snapshot=s_worldSnapshot.Read();
    if(!output || capacity<=0 || GetTickCount64()-snapshot.time>500) return 0;
    int count=(std::min)(snapshot.count,capacity);
    memcpy(output,snapshot.entries,count*sizeof(WorldMarker));
    return count;
}
