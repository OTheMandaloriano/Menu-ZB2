#include "readiness.h"
#include "readiness_protocol.h"
#include <Windows.h>
#include <ShlObj.h>
#include <algorithm>
namespace LoaderServices {
namespace {
std::string NormalizePath(std::string value){for(auto& ch:value){if(ch=='\\')ch='/';else if(ch>='A'&&ch<='Z')ch=static_cast<char>(ch-'A'+'a');}return value;}
}
void LogState::Line(const std::string& line){
    if(line.rfind("Mono path[0] = '",0)==0){session=NormalizePath(line).find(expected)!=std::string::npos;assemblies=domain=world=false;}
    if(!session)return;
    if(line=="Begin MonoManager ReloadAssembly"){assemblies=domain=world=false;}
    if(line.find("- Loaded All Assemblies, in ")==0)assemblies=true;
    if(line.find("- Finished resetting the current domain, in ")==0)domain=assemblies;
    if(line.find(">>> GAME CLEANUP")!=std::string::npos || line.find("SERVER SHUTDOWN")!=std::string::npos || line.find("Disconnected from server!")!=std::string::npos)world=false;
    if(line=="Match > Started match" || line=="Client > Received hash info")world=true;
}
bool Readiness::Poll(const Process& process){
    if(!process.pid)return false;
    if(identity_!=process.created){identity_=process.created;fileId_=offset_=0;pending_.clear();state_={};state_.expected=NormalizePath((process.executable.parent_path()/L"ZumbiBlocks2_Data"/L"Managed").u8string());}
    PWSTR folder=nullptr;if(FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppDataLow,0,nullptr,&folder)))return false;
    auto path=std::filesystem::path(folder)/L"Adrianks47"/L"ZumbiBlocks2"/L"Player.log";CoTaskMemFree(folder);
    HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){caughtUp_=false;return false;}
    BY_HANDLE_FILE_INFORMATION info{};if(!GetFileInformationByHandle(file,&info)){CloseHandle(file);caughtUp_=false;return false;}
    const auto id=(uint64_t(info.nFileIndexHigh)<<32)|info.nFileIndexLow;const auto size=(uint64_t(info.nFileSizeHigh)<<32)|info.nFileSizeLow;
    if(ReadyProtocol::FileTime(info.ftLastWriteTime)<process.created){CloseHandle(file);caughtUp_=false;return false;}
    if(id!=fileId_||size<offset_){fileId_=id;offset_=0;pending_.clear();auto expected=state_.expected;state_={};state_.expected=expected;}
    LARGE_INTEGER start{};start.QuadPart=static_cast<LONGLONG>(offset_);if(!SetFilePointerEx(file,start,nullptr,FILE_BEGIN)){CloseHandle(file);return false;}
    char buffer[65536];uint64_t budget=4*1024*1024;
    while(offset_<size&&budget){DWORD read=0;DWORD ask=static_cast<DWORD>(std::min<uint64_t>(sizeof(buffer),std::min(size-offset_,budget)));
        if(!ReadFile(file,buffer,ask,&read,nullptr)||!read)break;offset_+=read;budget-=read;pending_.append(buffer,read);
        size_t consumed=0;
        for(size_t end=pending_.find('\n');end!=std::string::npos;end=pending_.find('\n',consumed)){auto line=pending_.substr(consumed,end-consumed);if(!line.empty()&&line.back()=='\r')line.pop_back();state_.Line(line);consumed=end+1;}
        pending_.erase(0,consumed);if(pending_.size()>65536){pending_.clear();state_.domain=false;}
    }
    CloseHandle(file);caughtUp_=offset_==size;
    return caughtUp_&&state_.session&&state_.assemblies&&state_.domain&&process.monoLoaded;
}
}
