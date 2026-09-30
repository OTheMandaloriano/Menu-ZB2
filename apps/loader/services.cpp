#include "services.h"
#include "readiness_protocol.h"
#include <Windows.h>
#include <TlHelp32.h>
#include <ShlObj.h>
#include <wincrypt.h>
#include <compressapi.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <charconv>
namespace fs=std::filesystem;
namespace LoaderServices {
namespace {
struct Handle {
    HANDLE value=INVALID_HANDLE_VALUE;
    explicit Handle(HANDLE h):value(h){}
    ~Handle(){if(value && value!=INVALID_HANDLE_VALUE)CloseHandle(value);}
    Handle(const Handle&)=delete;Handle& operator=(const Handle&)=delete;
};
constexpr const char* names[]={"injector.exe","config.ini","kiero-dx11-base.dll","Zb2.AimBridge.dll","0Harmony.dll","Harmony.LICENSE","ZB2.Readiness.dll"};
struct File { std::string name,hash;size_t size=0; };
struct Bundle {std::string version;std::vector<File> files;};
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
bool SafeWord(const std::string& word){return !word.empty() && word.size()<80 && std::all_of(word.begin(),word.end(),[](char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='-';});}
void CheckPath(const fs::path& path) {
    fs::path current;
    for(const auto& part:fs::absolute(path)){
        current/=part;
        DWORD attributes=GetFileAttributesW(current.c_str());
        if(attributes!=INVALID_FILE_ATTRIBUTES && (attributes&FILE_ATTRIBUTE_REPARSE_POINT))
            throw std::runtime_error(u8"Pasta redirecionada não suportada. Use uma pasta Documentos local.");
    }
}
std::string ReadFile(const fs::path& path,size_t maxSize) {
    CheckPath(path);
    std::ifstream input(path,std::ios::binary|std::ios::ate);
    Require(static_cast<bool>(input),u8"Não foi possível ler o arquivo local.");
    auto size=input.tellg();Require(size>=0 && static_cast<uint64_t>(size)<=maxSize,"Arquivo excede o limite permitido.");
    std::string data(static_cast<size_t>(size),'\0');input.seekg(0);
    if(!data.empty())input.read(data.data(),static_cast<std::streamsize>(data.size()));
    Require(static_cast<bool>(input),"Falha ao ler o arquivo completo.");return data;
}
void WriteNew(const fs::path& path,const std::string& data) {
    CheckPath(path);
    Handle file(CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr));
    Require(file.value!=INVALID_HANDLE_VALUE,u8"Não foi possível criar o arquivo. Confira a permissão da pasta.");
    DWORD written=0;
    Require(WriteFile(file.value,data.data(),static_cast<DWORD>(data.size()),&written,nullptr) && written==data.size() && FlushFileBuffers(file.value),"Falha ao salvar arquivo completo.");
}
Bundle ParseBundle() {
    const auto data=Resource(103),signature=Resource(104);
    Require(License::Verify(data,std::vector<unsigned char>(signature.begin(),signature.end()),PublicKey()),u8"Pacote inválido: assinatura não confere. Obtenha novamente o loader.");
    std::istringstream lines(data);std::string magic,commit;Bundle result;
    std::getline(lines,magic);std::getline(lines,result.version);std::getline(lines,commit);
    Require(magic=="ZB2-BUNDLE-1" && SafeWord(result.version) && commit.size()==40 && License::Unhex(commit).size()==20,"Manifesto do pacote incorreto.");
    for(const auto* name:names){
        std::string row;Require(static_cast<bool>(std::getline(lines,row)),"Manifesto incompleto.");
        auto first=row.find(':');auto second=row.find(':',first==std::string::npos?0:first+1);
        Require(first!=std::string::npos && second!=std::string::npos,"Registro incorreto.");
        File file;file.name=row.substr(0,first);file.hash=row.substr(second+1);
        auto start=row.data()+first+1,end=row.data()+second;auto parsed=std::from_chars(start,end,file.size);
        Require(file.name==name && parsed.ec==std::errc() && parsed.ptr==end && file.size>0 && file.size<=64*1024*1024 && License::Unhex(file.hash).size()==32,"Registro de arquivo incorreto.");
        result.files.push_back(file);
    }
    std::string extra;Require(!std::getline(lines,extra),"Manifesto com registros extras.");return result;
}
std::string Unpack(size_t index,const File& file) {
    auto packed=Resource(301+static_cast<int>(index));
    DECOMPRESSOR_HANDLE decompressor=nullptr;
    Require(CreateDecompressor(COMPRESS_ALGORITHM_MSZIP,nullptr,&decompressor),u8"Descompressão indisponível no Windows.");
    std::string output(file.size,'\0');SIZE_T written=0;
    BOOL ok=Decompress(decompressor,packed.data(),packed.size(),output.data(),output.size(),&written);
    CloseDecompressor(decompressor);
    Require(ok && written==file.size && License::Sha256(output.data(),output.size())==file.hash,"Pacote danificado. Baixe novamente o loader.");return output;
}
std::string Transform(const std::string& value,bool decrypt) {
    DATA_BLOB input{static_cast<DWORD>(value.size()),reinterpret_cast<BYTE*>(const_cast<char*>(value.data()))},output{};
    BOOL ok=decrypt?CryptUnprotectData(&input,nullptr,nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&output):
        CryptProtectData(&input,L"ZB2 Menu local license",nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&output);
    Require(ok,u8"Estado local ilegível. A licença pertence a outro usuário ou o arquivo está danificado.");
    std::string result(reinterpret_cast<char*>(output.pbData),output.cbData);LocalFree(output.pbData);return result;
}
bool HasModule(DWORD pid,const wchar_t* name=L"kiero-dx11-base.dll") {
    Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid));
    Require(snapshot.value!=INVALID_HANDLE_VALUE,u8"Não foi possível verificar os módulos do jogo. Confira as permissões.");
    MODULEENTRY32W entry{};entry.dwSize=sizeof(entry);
    Require(Module32FirstW(snapshot.value,&entry),u8"Jogo encerrou durante a verificação. Abra-o novamente.");
    do{if(_wcsicmp(entry.szModule,name)==0)return true;}while(Module32NextW(snapshot.value,&entry));
    return false;
}
}
std::string Resource(int id) {
    HMODULE module=GetModuleHandleW(nullptr);HRSRC resource=FindResourceW(module,MAKEINTRESOURCEW(id),RT_RCDATA);
    Require(resource!=nullptr,"Recurso interno ausente. Obtenha novamente o loader.");
    DWORD size=SizeofResource(module,resource);const void* data=LockResource(LoadResource(module,resource));
    Require(data && size>0 && size<=64*1024*1024,"Recurso interno incorreto.");
    return std::string(static_cast<const char*>(data),size);
}
std::vector<unsigned char> PublicKey(){auto key=Resource(102);return {key.begin(),key.end()};}
fs::path DataDirectory() {
    PWSTR documents=nullptr;
    Require(SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents,0,nullptr,&documents)),"Pasta Documentos indisponivel.");
    fs::path root=fs::path(documents)/L"ZB2Menu"/L"loader";CoTaskMemFree(documents);CheckPath(root);return root;
}
Stored ReadState(const fs::path& root) {
    CheckPath(root);const auto path=root/L"license.dat";
    if(!fs::exists(path))return {};
    auto plain=Transform(ReadFile(path,8192),true);auto split=plain.find('\n');Stored result;
    Require(split!=std::string::npos,"Estado local incompleto.");
    auto parsed=std::from_chars(plain.data(),plain.data()+split,result.lastSeen);
    Require(parsed.ec==std::errc() && parsed.ptr==plain.data()+split && result.lastSeen>0,"Estado local incorreto.");
    result.token=plain.substr(split+1);Require(result.token.size()<=4096,"Licenca local excessiva.");return result;
}
void SaveState(const fs::path& root,const Stored& value) {
    CheckPath(root);fs::create_directories(root);
    auto path=root/L"license.dat",temporary=root/(L"license-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64())+L".tmp");
    auto bytes=Transform(std::to_string(value.lastSeen)+"\n"+value.token,false);
    WriteNew(temporary,bytes);
    if(!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){
        DeleteFileW(temporary.c_str());throw std::runtime_error("Falha ao salvar ativacao. Confira permissoes em Documentos.");
    }
}
void CheckClock(const Stored& state,int64_t now) {Require(now>=state.lastSeen-120,u8"Relógio retrocedeu. Corrija a data do Windows antes de continuar.");}
bool ReadAuto(const fs::path& root){auto file=root/L"auto-inject.txt";if(!fs::exists(file))return true;return ReadFile(file,8)=="1";}
std::string AdjacentLicense(const fs::path& executable){auto file=executable.parent_path()/L"licenca.zb2license";if(!fs::exists(file))return {};return License::Normalize(ReadFile(file,8192));}
bool ImportPackageLicense(const fs::path& executable,const fs::path& root,const std::string& device,int64_t now,const std::vector<unsigned char>& key,Stored& stored,std::string& warning){
    CheckClock(stored,now);const auto current=License::Validate(stored.token,device,now,key);
    try{
        const auto token=AdjacentLicense(executable);if(token.empty())return false;
        auto candidate=License::Validate(token,device,now,key);
        if(!candidate.valid){if(!current.valid)warning=candidate.error;return false;}
        if(current.valid&&(stored.token==token||candidate.expires<=current.expires))return false;
        Stored next{token,std::max(stored.lastSeen,now)};SaveState(root,next);stored=std::move(next);return true;
    }catch(const std::exception& error){warning=error.what();return false;}
}
void SaveAuto(const fs::path& root,bool enabled){CheckPath(root);fs::create_directories(root);auto file=root/L"auto-inject.txt",temporary=root/(L"auto-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64())+L".tmp");WriteNew(temporary,enabled?"1":"0");if(!MoveFileExW(temporary.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){DeleteFileW(temporary.c_str());throw std::runtime_error("Falha ao salvar AUTO-INJECT.");}}
Process FindGame() {
    Process result;Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0));
    Require(snapshot.value!=INVALID_HANDLE_VALUE,"Falha ao consultar processos.");
    PROCESSENTRY32W entry{};entry.dwSize=sizeof(entry);
    Require(Process32FirstW(snapshot.value,&entry),"Falha ao consultar processos.");
    do{
        if(_wcsicmp(entry.szExeFile,L"ZumbiBlocks2.exe")!=0)continue;
        Require(result.pid==0,u8"Mais de um jogo aberto. Mantenha apenas uma instância.");
        Handle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,entry.th32ProcessID));
        Require(process.value!=nullptr,u8"Jogo sem acesso de leitura. Confira as permissões.");
        wchar_t path[32768]{};DWORD length=32768;
        Require(QueryFullProcessImageNameW(process.value,0,path,&length),"Caminho do jogo indisponivel.");
        FILETIME started{},ended{},kernel{},user{};Require(GetProcessTimes(process.value,&started,&ended,&kernel,&user),"Identidade do processo indisponivel.");
        result.pid=entry.th32ProcessID;result.created=ReadyProtocol::FileTime(started);result.executable=path;result.loaded=HasModule(result.pid);
        result.probeLoaded=HasModule(result.pid,L"ZB2.Readiness.dll");result.monoLoaded=HasModule(result.pid,L"mono-2.0-bdwgc.dll");
    }while(Process32NextW(snapshot.value,&entry));return result;
}
std::string BundleVersion(){return ParseBundle().version;}
void VerifyBundle(){auto bundle=ParseBundle();for(size_t i=0;i<bundle.files.size();++i)Unpack(i,bundle.files[i]);}
fs::path InstallBundle(const fs::path& root) {
    auto bundle=ParseBundle();std::vector<std::string> contents;
    for(size_t i=0;i<bundle.files.size();++i)contents.push_back(Unpack(i,bundle.files[i]));
    auto manifest=Resource(103);auto hash=License::Sha256(manifest.data(),manifest.size());
    auto directory=root/L"runtime"/fs::path(bundle.version+"-"+hash.substr(0,16));CheckPath(directory);
    if(fs::exists(directory)) {
        size_t count=0;
        for(const auto& entry:fs::directory_iterator(directory)){
            const auto name=entry.path().filename().string();
            Require(std::find(std::begin(names),std::end(names),name)!=std::end(names),"Instalacao contem arquivos extras. Use uma copia limpa desta versao.");++count;
        }
        Require(count==bundle.files.size(),"Instalacao incompleta. Remova apenas esta versao e tente novamente.");
        for(const auto& file:bundle.files){auto data=ReadFile(directory/fs::path(file.name),file.size);Require(data.size()==file.size && License::Sha256(data.data(),data.size())==file.hash,"Instalacao local alterada. Remova apenas esta versao e tente novamente.");}
        return directory;
    }
    fs::create_directories(directory.parent_path());
    auto staging=directory;staging+=L".staging-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64());
    Require(fs::create_directory(staging),"Falha ao criar pasta temporaria.");
    try{
        for(size_t i=0;i<bundle.files.size();++i)WriteNew(staging/fs::path(bundle.files[i].name),contents[i]);
        Require(MoveFileExW(staging.c_str(),directory.c_str(),MOVEFILE_WRITE_THROUGH),"Falha ao concluir instalacao.");
    }catch(...){
        // Only remove the exact files created by this transaction, never recursively.
        for(const auto& file:bundle.files)DeleteFileW((staging/fs::path(file.name)).c_str());
        RemoveDirectoryW(staging.c_str());throw;
    }
    return directory;
}
static void LaunchHelper(const Process& game,const fs::path& runtime,bool probe) {
    Require(game.pid!=0 && !game.loaded,"Jogo ausente ou menu ja carregado.");
    Handle target(OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,game.pid));
    Require(target.value!=nullptr && WaitForSingleObject(target.value,0)==WAIT_TIMEOUT,"Jogo encerrou. Abra-o novamente.");
    auto current=FindGame();Require(current.pid==game.pid && current.created==game.created && current.executable==game.executable && !current.loaded,"O processo mudou. Confira o jogo e tente novamente.");
    if(probe)Require(!current.probeLoaded,"Sonda ja carregada neste processo.");
    else Require(SceneReady(current),"A cena ainda nao esta pronta. Aguarde a partida carregar.");
    auto assembly=ReadFile(game.executable.parent_path()/L"ZumbiBlocks2_Data"/L"Managed"/L"Assembly-CSharp.dll",64*1024*1024);
    Require(License::Sha256(assembly.data(),assembly.size())=="c41a298975d35f0dad0a05531bce6e0b6e274d0ddf265217d65ce3ac5cbc84e1",u8"Versão do jogo incompatível. Atualize o menu antes de carregar.");
    auto executable=runtime/L"injector.exe";
    std::wstring command=L"\""+executable.wstring()+L"\" --nowait --pid "+std::to_wstring(game.pid);
    if(probe)command+=L" --readiness-probe";
    STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION process{};
    SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES),nullptr,TRUE};
    auto logPath=runtime.parent_path().parent_path()/L"last-load.log";CheckPath(logPath);
    Handle log(CreateFileW(logPath.c_str(),GENERIC_WRITE,FILE_SHARE_READ,&attributes,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr));
    Handle input(CreateFileW(L"NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&attributes,OPEN_EXISTING,0,nullptr));
    Require(log.value!=INVALID_HANDLE_VALUE && input.value!=INVALID_HANDLE_VALUE,"Falha ao abrir log do carregador.");
    startup.dwFlags=STARTF_USESTDHANDLES;startup.hStdOutput=log.value;startup.hStdError=log.value;startup.hStdInput=input.value;
    Require(CreateProcessW(executable.c_str(),command.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW,nullptr,runtime.c_str(),&startup,&process),"Nao foi possivel iniciar o carregador. Confira o historico do antivirus e as permissoes.");
    Handle helper(process.hProcess),thread(process.hThread);
    DWORD wait=WaitForSingleObject(helper.value,45000),exit=1;
    Require(wait==WAIT_OBJECT_0,"Carregamento sem resposta. Nao tente novamente nesta sessao; reinicie o jogo.");
    Require(GetExitCodeProcess(helper.value,&exit) && exit==0,"Carregamento falhou. Consulte o log do carregador; reinicie o jogo antes de tentar novamente.");
    Require(WaitForSingleObject(target.value,0)==WAIT_TIMEOUT && HasModule(game.pid,probe?L"ZB2.Readiness.dll":L"kiero-dx11-base.dll"),"O modulo nao foi confirmado no jogo. Reinicie o jogo e confira o log.");
}
void LoadRuntime(const Process& game,const fs::path& runtime){LaunchHelper(game,runtime,false);}
void PrepareReadiness(const Process& game,const fs::path& runtime){LaunchHelper(game,runtime,true);}
bool SceneReady(const Process& game){
    wchar_t name[96]{};swprintf_s(name,L"Local\\ZB2.Ready.%lu",game.pid);Handle mapping(OpenFileMappingW(FILE_MAP_READ,FALSE,name));if(!mapping.value)return false;
    auto* shared=static_cast<const ReadyProtocol::State*>(MapViewOfFile(mapping.value,FILE_MAP_READ,0,0,sizeof(ReadyProtocol::State)));if(!shared)return false;
    bool ready=false;
    for(int attempt=0;attempt<3;++attempt){LONG before=shared->sequence;if(before&1)continue;MemoryBarrier();ReadyProtocol::State state{};memcpy(&state,shared,sizeof(state));MemoryBarrier();if(before==shared->sequence){ready=ReadyProtocol::Ready(state,game.pid,game.created,GetTickCount64());break;}}
    UnmapViewOfFile(shared);return ready;
}
}
