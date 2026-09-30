#include "settings.h"
#include <Windows.h>
#include <fstream>
#include <stdexcept>
#include <string>
#include "../../loader/license.h"
#include <algorithm>
namespace Admin {
namespace {
void Safe(const std::filesystem::path& path){std::filesystem::path current;for(const auto& part:std::filesystem::absolute(path)){current/=part;DWORD attributes=GetFileAttributesW(current.c_str());if(attributes!=INVALID_FILE_ATTRIBUTES&&(attributes&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("Pasta redirecionada nao suportada para configuracoes.");}}
void Write(const std::filesystem::path& path,const std::string& content){
    Safe(path);std::filesystem::create_directories(path.parent_path());
    auto temporary=path;temporary+=L"."+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64())+L".tmp";
    {std::ofstream stream(temporary,std::ios::binary|std::ios::trunc);stream<<content;stream.flush();if(!stream)throw std::runtime_error("Nao foi possivel salvar o arquivo.");}
    if(!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){DeleteFileW(temporary.c_str());throw std::runtime_error("Falha ao concluir o arquivo.");}
}
}
Settings ReadSettings(const std::filesystem::path& root){
    auto path=root/L"preferences.txt";Safe(path);Settings result;if(!std::filesystem::exists(path))return result;
    if(std::filesystem::file_size(path)>128)throw std::runtime_error("Preferencias invalidas.");std::ifstream stream(path);std::string line;std::getline(stream,line);
    if(line=="reduced_motion=1")result.reducedMotion=true;else if(line!="reduced_motion=0")throw std::runtime_error("Preferencias invalidas. Confira preferences.txt.");return result;
}
void SaveSettings(const std::filesystem::path& root,const Settings& settings){Write(root/L"preferences.txt",settings.reducedMotion?"reduced_motion=1\n":"reduced_motion=0\n");}
namespace {
bool HashName(const std::string& value,size_t length){return value.size()==length&&std::all_of(value.begin(),value.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');});}
// Only single-file, content-addressed Admin services are cache candidates.
// Runtime used by the game, licenses, backups and logs are outside this API.
bool Candidate(const std::filesystem::path& root,const std::filesystem::path& path,const std::string& current){
    if(!HashName(current,64))throw std::runtime_error("Identificação do serviço atual inválida.");
    Safe(root);Safe(path);
    const auto base=std::filesystem::absolute(root/L"runtime").lexically_normal();
    if(std::filesystem::absolute(path.parent_path().parent_path()).lexically_normal()!=base||path.filename()!=L"ZB2AdminBackend.exe")return false;
    auto name=path.parent_path().filename().string();if(!HashName(name,16)||name==current.substr(0,16))return false;
    size_t count=0;for(const auto& item:std::filesystem::directory_iterator(path.parent_path())){(void)item;++count;}return count==1;
}
struct CacheFile {HANDLE value=INVALID_HANDLE_VALUE;~CacheFile(){if(value!=INVALID_HANDLE_VALUE)CloseHandle(value);}};
std::string ReadCache(HANDLE file){
    LARGE_INTEGER size{};if(!GetFileSizeEx(file,&size)||size.QuadPart<1024||size.QuadPart>32*1024*1024)return {};
    std::string bytes(static_cast<size_t>(size.QuadPart),'\0');DWORD read=0;
    if(!ReadFile(file,bytes.data(),static_cast<DWORD>(bytes.size()),&read,nullptr)||read!=bytes.size()||bytes[0]!='M'||bytes[1]!='Z')return {};return bytes;
}
}
CachePlan InspectCache(const std::filesystem::path& root,const std::string& currentHash){
    Safe(root);CachePlan plan;auto base=root/L"runtime";Safe(base);if(!std::filesystem::exists(base))return plan;
    for(const auto& folder:std::filesystem::directory_iterator(base)){
        try{auto path=folder.path()/L"ZB2AdminBackend.exe";if(!Candidate(root,path,currentHash))continue;
            CacheFile file;file.value=CreateFileW(path.c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);if(file.value==INVALID_HANDLE_VALUE)continue;
            auto bytes=ReadCache(file.value);if(bytes.empty())continue;auto hash=License::Sha256(bytes.data(),bytes.size());if(hash.substr(0,16)!=folder.path().filename().string())continue;
            plan.entries.push_back({path,hash,bytes.size()});plan.bytes+=bytes.size();
        }catch(const std::filesystem::filesystem_error&){continue;}catch(const std::runtime_error&){continue;}
    }return plan;
}
CacheResult CleanCache(const std::filesystem::path& root,const CachePlan& reviewed,const std::string& currentHash){
    CacheResult result;
    for(const auto& entry:reviewed.entries){bool removed=false;
        try{if(Candidate(root,entry.path,currentHash)){
            // Exclusive access prevents removing a service that another process is using.
            CacheFile file;file.value=CreateFileW(entry.path.c_str(),GENERIC_READ|DELETE,0,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
            if(file.value!=INVALID_HANDLE_VALUE){auto bytes=ReadCache(file.value);
                if(bytes.size()==entry.bytes&&License::Sha256(bytes.data(),bytes.size())==entry.hash&&entry.hash.substr(0,16)==entry.path.parent_path().filename().string()){
                    FILE_DISPOSITION_INFO disposition{TRUE};removed=SetFileInformationByHandle(file.value,FileDispositionInfo,&disposition,sizeof(disposition))!=FALSE;
                }
            }
        }}catch(const std::exception&){removed=false;}
        if(removed){++result.removed;result.bytes+=entry.bytes;RemoveDirectoryW(entry.path.parent_path().c_str());}else ++result.kept;
    }return result;
}
void WriteDiagnostics(const std::filesystem::path& path,bool owner,int licenseCount,int teamCount){
    Write(path,"ZB2 Admin 1.13\nFrontend: Dear ImGui / D3D11 / Windows x64\nModo: offline\nPerfil: "+std::string(owner?"proprietario":"integrante ou pendente")+"\nLicencas locais: "+std::to_string(licenseCount)+"\nAutorizacoes locais: "+std::to_string(teamCount)+"\nNao inclui nomes, IDs, chaves, tokens ou conteudo de licencas.\n");
}
}
