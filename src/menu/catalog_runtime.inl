struct CatalogSnapshot { CatalogEntry entries[128] = {}; int count=0; };
static LatestSnapshot<CatalogSnapshot> s_catalog;
static LatestSnapshot<IconPixels> s_icon;
static std::atomic<int> s_iconRequest{-1};
static void UpdateCatalog() {
    if(!s_catalogRead || !s_options.bMenuOpen)return;
    static long long nextNames=0,nextIcon=0;
    const auto now=PiNow();
    if(now>=nextNames) {
        nextNames=now+2000000;CatalogSnapshot snapshot;void* buffer=snapshot.entries;int capacity=128;void* args[]={&buffer,&capacity};MonoObject* exception=nullptr;
        auto result=pInvoke(s_catalogRead,nullptr,args,&exception);
        if(result && !exception){auto data=pUnbox(result);if(data)memcpy(&snapshot.count,data,4);}
        if(snapshot.count<0 || snapshot.count>128)snapshot.count=0;s_catalog.Publish(snapshot);
    }
    if(now<nextIcon || !s_catalogIcon)return;
    const int id=s_iconRequest.exchange(-1);if(id<0)return;nextIcon=now+100000;
    IconPixels icon;icon.id=id;void* buffer=icon.rgba;int item=id;void* args[]={&item,&buffer};bool ok=false;
    icon.valid=InvokeAimBool(s_catalogIcon,nullptr,args,ok) && ok;s_icon.Publish(icon);
}
int GetCatalog(CatalogEntry* output,int capacity){const auto& snap=s_catalog.Read();int count=(std::min)(capacity,snap.count);if(count>0)memcpy(output,snap.entries,count*sizeof(CatalogEntry));return count;}
void RequestIcon(int id){int empty=-1;s_iconRequest.compare_exchange_strong(empty,id);}
bool GetIcon(IconPixels& output){const auto& value=s_icon.Read();if(value.id<0)return false;output=value;return true;}
