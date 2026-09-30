#include "backend.h"
#include "../../shared/resources.h"
#include "../../loader/license.h"
#include <Windows.h>
#include <ShlObj.h>
#include <fstream>
#include <stdexcept>
#include <algorithm>
namespace fs=std::filesystem;
namespace Admin {
namespace {
struct Handle {
    HANDLE value=nullptr;~Handle(){if(value&&value!=INVALID_HANDLE_VALUE)CloseHandle(value);}
    Handle()=default;Handle(const Handle&)=delete;Handle&operator=(const Handle&)=delete;
};
void Require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
void SafePath(const fs::path& path){fs::path current;for(const auto& part:fs::absolute(path)){current/=part;DWORD a=GetFileAttributesW(current.c_str());if(a!=INVALID_FILE_ATTRIBUTES&&(a&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("Pasta redirecionada nao suportada para o servico.");}}
std::string FileBytes(const fs::path& path){std::ifstream file(path,std::ios::binary|std::ios::ate);Require(bool(file),"Nao foi possivel ler o servico local.");auto n=file.tellg();Require(n>0&&n<2*1024*1024,"Servico local incorreto.");std::string result(static_cast<size_t>(n),'\0');file.seekg(0);file.read(result.data(),n);Require(bool(file),"Servico local incompleto.");return result;}
void ReadBounded(HANDLE pipe,HANDLE process,void* buffer,DWORD length,ULONGLONG deadline){
    auto* out=static_cast<unsigned char*>(buffer);DWORD done=0;
    while(done<length){DWORD available=0;
        Require(PeekNamedPipe(pipe,nullptr,0,nullptr,&available,nullptr),"O servico encerrou sem confirmar a operacao. Consulte o historico.");
        if(available){DWORD read=0;Require(ReadFile(pipe,out+done,std::min(available,length-done),&read,nullptr)&&read>0,"Resposta incompleta do servico.");done+=read;continue;}
        Require(WaitForSingleObject(process,0)==WAIT_TIMEOUT,"O servico encerrou sem resposta.");
        Require(GetTickCount64()<deadline,"Tempo de resposta excedido. Consulte o historico antes de repetir uma emissao.");Sleep(3);
    }
}
}
fs::path DataRoot(){PWSTR local=nullptr;Require(SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&local)),"Perfil Windows indisponivel.");fs::path root=fs::path(local)/L"ZB2Admin";CoTaskMemFree(local);return root;}
fs::path Backend::Executable(){
    const auto bytes=AppResources::Read(501),expected=AppResources::Read(502);
    Require(License::Sha256(bytes.data(),bytes.size())==expected,"Servico embutido danificado. Obtenha o pacote novamente.");
    fs::path directory=root_/L"runtime"/fs::path(expected.substr(0,16));SafePath(directory);fs::create_directories(directory);
    auto executable=directory/L"ZB2AdminBackend.exe";SafePath(executable);
    if(fs::exists(executable)){auto existing=FileBytes(executable);Require(License::Sha256(existing.data(),existing.size())==expected,"Servico local alterado. Reinstale esta versao do Admin.");return executable;}
    auto temporary=directory/(L"backend-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64())+L".tmp");
    {std::ofstream file(temporary,std::ios::binary|std::ios::trunc);Require(bool(file),"Nao foi possivel instalar o servico.");file.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));file.flush();Require(bool(file),"Falha ao gravar o servico.");}
    if(!MoveFileExW(temporary.c_str(),executable.c_str(),MOVEFILE_WRITE_THROUGH)){DeleteFileW(temporary.c_str());throw std::runtime_error("Nao foi possivel concluir a instalacao do servico.");}return executable;
}
Snapshot Backend::Call(const Request& request){
    auto executable=Executable();std::string input=EncodeRequest(request);
    SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES),nullptr,TRUE};Handle readInput,writeInput,readOutput,writeOutput,errors;
    Require(CreatePipe(&readInput.value,&writeInput.value,&security,131080)&&CreatePipe(&readOutput.value,&writeOutput.value,&security,0),"Nao foi possivel abrir o canal local.");
    Require(SetHandleInformation(writeInput.value,HANDLE_FLAG_INHERIT,0)&&SetHandleInformation(readOutput.value,HANDLE_FLAG_INHERIT,0),"Falha ao proteger o canal local.");
    errors.value=CreateFileW(L"NUL",GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,OPEN_EXISTING,0,nullptr);Require(errors.value!=INVALID_HANDLE_VALUE,"Canal de diagnostico indisponivel.");
    STARTUPINFOEXW startup{};startup.StartupInfo.cb=sizeof(startup);startup.StartupInfo.dwFlags=STARTF_USESTDHANDLES;
    startup.StartupInfo.hStdInput=readInput.value;startup.StartupInfo.hStdOutput=writeOutput.value;startup.StartupInfo.hStdError=errors.value;
    SIZE_T attributeSize=0;InitializeProcThreadAttributeList(nullptr,1,0,&attributeSize);std::vector<unsigned char> attributes(attributeSize);
    startup.lpAttributeList=reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributes.data());
    Require(InitializeProcThreadAttributeList(startup.lpAttributeList,1,0,&attributeSize),"Falha ao preparar o servico.");
    HANDLE inherited[]={readInput.value,writeOutput.value,errors.value};
    BOOL updated=UpdateProcThreadAttribute(startup.lpAttributeList,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,inherited,sizeof(inherited),nullptr,nullptr);
    std::wstring command=L"\""+executable.wstring()+L"\" --data \""+root_.wstring()+L"\"";PROCESS_INFORMATION process{};
    BOOL launched=updated&&CreateProcessW(executable.c_str(),command.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW|EXTENDED_STARTUPINFO_PRESENT,nullptr,executable.parent_path().c_str(),&startup.StartupInfo,&process);
    DeleteProcThreadAttributeList(startup.lpAttributeList);Require(launched,"Nao foi possivel iniciar o servico do Admin.");
    Handle child,thread;child.value=process.hProcess;thread.value=process.hThread;
    CloseHandle(readInput.value);readInput.value=nullptr;CloseHandle(writeOutput.value);writeOutput.value=nullptr;
    try{
        DWORD length=static_cast<DWORD>(input.size()),written=0;
        Require(WriteFile(writeInput.value,&length,sizeof(length),&written,nullptr)&&written==sizeof(length),"Falha ao enviar o pedido.");
        Require(WriteFile(writeInput.value,input.data(),length,&written,nullptr)&&written==length,"Falha ao enviar os dados.");
        CloseHandle(writeInput.value);writeInput.value=nullptr;
        auto deadline=GetTickCount64()+30000;DWORD size=0;ReadBounded(readOutput.value,child.value,&size,sizeof(size),deadline);
        Require(size>0&&size<=4*1024*1024,"Resposta fora do limite.");std::string reply(size,'\0');ReadBounded(readOutput.value,child.value,reply.data(),size,deadline);
        Require(WaitForSingleObject(child.value,3000)==WAIT_OBJECT_0,"Servico nao finalizou a operacao.");
        return DecodeResponse(reply);
    }catch(...){if(WaitForSingleObject(child.value,0)==WAIT_TIMEOUT){TerminateProcess(child.value,1);WaitForSingleObject(child.value,1000);}throw;}
}
}
