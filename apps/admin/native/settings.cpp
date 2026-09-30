#include "settings.h"
#include <Windows.h>
#include <fstream>
#include <stdexcept>
#include <string>
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
void WriteDiagnostics(const std::filesystem::path& path,bool owner,int licenseCount,int teamCount){
    Write(path,"ZB2 Admin 1.6\nFrontend: Dear ImGui / D3D11 / Windows x64\nModo: offline\nPerfil: "+std::string(owner?"proprietario":"integrante ou pendente")+"\nLicencas locais: "+std::to_string(licenseCount)+"\nAutorizacoes locais: "+std::to_string(teamCount)+"\nNao inclui nomes, IDs, chaves, tokens ou conteudo de licencas.\n");
}
}
