// Loaded only after Unity reports its domain reset complete. No game hooks or invokes.
#include "readiness_protocol.h"
#include <atomic>
#include <cwchar>
namespace {
std::atomic<bool> stopping{false};
using NoArgs=void*(__cdecl*)();using Attach=void*(__cdecl*)(void*);using Detach=void(__cdecl*)(void*);
using Image=void*(__cdecl*)(const char*);using Class=void*(__cdecl*)(void*,const char*,const char*);
using Field=void*(__cdecl*)(void*,const char*);using VTable=void*(__cdecl*)(void*,void*);
using StaticGet=void(__cdecl*)(void*,void*,void*);using GetValue=void(__cdecl*)(void*,void*,void*);
struct Api {NoArgs root{};Attach attach{};Detach detach{};Image image{};Class klass{};Field field{};VTable vtable{};StaticGet stat{};GetValue value{};};
template<class T> bool Bind(HMODULE module,const char* name,T& result){result=reinterpret_cast<T>(GetProcAddress(module,name));return result!=nullptr;}
bool Sample(const Api& api,void* domain,void* image,bool& map,bool& objects){
    __try {
        auto singleton=[&](const char* name,const char* field)->void*{void* c=api.klass(image,"",name);if(!c)return nullptr;void* f=api.field(c,field);if(!f)return nullptr;void* vt=api.vtable(domain,c);if(!vt)return nullptr;void* object=nullptr;api.stat(vt,f,&object);return object;};
        void* main=singleton("ZBMain","instance");void* c=api.klass(image,"","ZBMain");void* field=c?api.field(c,"mapIsLoaded"):nullptr;
        if(!field)return false;unsigned char loaded=0;if(main)api.value(main,field,&loaded);map=main&&loaded!=0;
        objects=singleton("MainCamera","instance")&&singleton("ZombieLoader","Instance");return true;
    }__except(EXCEPTION_EXECUTE_HANDLER){map=objects=false;return true;}
}
DWORD WINAPI Observe(void*){
    wchar_t name[96]{};swprintf_s(name,L"Local\\ZB2.Ready.%lu",GetCurrentProcessId());
    HANDLE mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(ReadyProtocol::State),name);if(!mapping)return 1;
    auto* state=static_cast<ReadyProtocol::State*>(MapViewOfFile(mapping,FILE_MAP_WRITE,0,0,sizeof(ReadyProtocol::State)));if(!state){CloseHandle(mapping);return 1;}
    FILETIME created{},exit{},kernel{},user{};GetProcessTimes(GetCurrentProcess(),&created,&exit,&kernel,&user);
    Api api;void* domain=nullptr;void* thread=nullptr;bool bound=false;uint64_t readySince=0;
    while(!stopping.load()){
        if(GetModuleHandleW(L"Deadblock.Menu.dll")||GetModuleHandleW(L"kiero-dx11-base.dll"))break;
        bool assemblies=false,map=false,objects=false,unsupported=false;
        if(!bound){HMODULE module=GetModuleHandleW(L"mono-2.0-bdwgc.dll");if(module){
            bound=Bind(module,"mono_get_root_domain",api.root)&&Bind(module,"mono_thread_attach",api.attach)&&Bind(module,"mono_thread_detach",api.detach)&&
                Bind(module,"mono_image_loaded",api.image)&&Bind(module,"mono_class_from_name",api.klass)&&Bind(module,"mono_class_get_field_from_name",api.field)&&
                Bind(module,"mono_class_vtable",api.vtable)&&Bind(module,"mono_field_static_get_value",api.stat)&&Bind(module,"mono_field_get_value",api.value);
            unsupported=!bound;
        }}
        if(bound){domain=api.root();if(domain){if(!thread)thread=api.attach(domain);void* image=thread?api.image("Assembly-CSharp"):nullptr;
            assemblies=image!=nullptr;if(image)unsupported=!Sample(api,domain,image,map,objects);
        }}
        const auto now=GetTickCount64();if(assemblies&&map&&objects&&!unsupported){if(!readySince)readySince=now;}else readySince=0;
        InterlockedIncrement(&state->sequence);state->magic=ReadyProtocol::Magic;state->version=ReadyProtocol::Version;state->pid=GetCurrentProcessId();
        state->processCreated=ReadyProtocol::FileTime(created);state->sampledAt=now;state->readySince=readySince;state->assemblies=assemblies;state->mapLoaded=map;state->sceneObjects=objects;state->unsupported=unsupported;
        InterlockedIncrement(&state->sequence);
        Sleep(100);
    }
    if(thread&&api.detach)api.detach(thread);UnmapViewOfFile(state);CloseHandle(mapping);return 0;
}
}
BOOL WINAPI DllMain(HMODULE module,DWORD reason,LPVOID){
    if(reason==DLL_PROCESS_ATTACH){DisableThreadLibraryCalls(module);HANDLE thread=CreateThread(nullptr,0,Observe,nullptr,0,nullptr);if(thread)CloseHandle(thread);}
    else if(reason==DLL_PROCESS_DETACH)stopping.store(true);
    return TRUE;
}
