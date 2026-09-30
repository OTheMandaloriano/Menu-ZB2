#include "../apps/admin/native/backend.h"
#include "../apps/admin/native/ui.h"
#include "../apps/shared/widgets.h"
#include "../apps/admin/native/settings.h"
#include "../apps/loader/license.h"
#include "software_renderer.h"
#include <iostream>
#include <fstream>
#include <Windows.h>
void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::string Utf8(const wchar_t* value){int n=WideCharToMultiByte(CP_UTF8,0,value,-1,nullptr,0,nullptr,nullptr);std::string output(static_cast<size_t>(n),'\0');WideCharToMultiByte(CP_UTF8,0,value,-1,output.data(),n,nullptr,nullptr);output.pop_back();return output;}
int wmain(int argc,wchar_t** argv){
    try{
        if(argc==3&&std::wstring(argv[1])==L"cache-test"){
            const std::filesystem::path root=argv[2];std::filesystem::create_directories(root/L"licenses");
            auto put=[](const std::filesystem::path& path,const std::string& bytes){std::filesystem::create_directories(path.parent_path());std::ofstream f(path,std::ios::binary);f<<bytes;};
            auto payload=[](char c){return std::string("MZ")+std::string(2048,c);};
            auto service=[&](char c){auto bytes=payload(c);auto hash=License::Sha256(bytes.data(),bytes.size());auto file=root/L"runtime"/hash.substr(0,16)/L"ZB2AdminBackend.exe";put(file,bytes);return file;};
            auto old=service('a'),changed=service('b'),locked=service('c'),current=service('d'),unknown=service('e');
            put(unknown.parent_path()/L"keep.txt","unknown contents");put(root/L"licenses"/L"keep.json","personal record");put(root/L"station.json","private profile");
            auto currentBytes=payload('d');auto currentHash=License::Sha256(currentBytes.data(),currentBytes.size());
            auto plan=Admin::InspectCache(root,currentHash);Check(plan.entries.size()==3,"only obsolete managed services are candidates");
            put(changed,payload('z'));HANDLE busy=CreateFileW(locked.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);Check(busy!=INVALID_HANDLE_VALUE,"test lock");
            auto outside=root/L"outside"/L"ZB2AdminBackend.exe";put(outside,payload('a'));plan.entries.push_back({outside,License::Sha256(payload('a').data(),payload('a').size()),2050});
            auto result=Admin::CleanCache(root,plan,currentHash);CloseHandle(busy);
            Check(result.removed==1&&result.kept==3,"changed, busy and out-of-scope files must be kept");
            Check(!std::filesystem::exists(old)&&std::filesystem::exists(changed)&&std::filesystem::exists(current)&&std::filesystem::exists(unknown)&&std::filesystem::exists(outside),"cleanup boundaries");
            Check(std::filesystem::exists(root/L"station.json")&&std::filesystem::exists(root/L"licenses"/L"keep.json"),"personal state must survive");
            auto next=Admin::InspectCache(root,currentHash);Check(next.entries.size()==1,"changed hash and foreign contents excluded");Check(Admin::CleanCache(root,next,currentHash).removed==1,"unlocked reviewed service removed");
            std::cout<<"Cache checks passed: current service, personal state, changed files, busy files and path boundaries preserved.";return 0;
        }
        if(argc==3&&std::wstring(argv[1])==L"settings-test"){
            const std::filesystem::path root=argv[2];Check(!Admin::ReadSettings(root).reducedMotion,"default motion preference");
            Admin::SaveSettings(root,{true});Check(Admin::ReadSettings(root).reducedMotion,"saved motion preference");
            Admin::SaveSettings(root,{false});Check(!Admin::ReadSettings(root).reducedMotion,"updated motion preference");
            Admin::WriteDiagnostics(root/L"diagnostic.txt",true,4,2);
            std::ifstream input(root/L"diagnostic.txt");std::string data((std::istreambuf_iterator<char>(input)),{});
            Check(data.find("Licencas locais: 4")!=std::string::npos&&data.find("ZB2L1.")==std::string::npos&&data.find("ProtectedKey")==std::string::npos,"diagnostic metadata");
            std::cout<<"Settings persistence and diagnostic checks passed";return 0;
        }
        if(argc==3&&std::wstring(argv[1])==L"render-profile"){
            Admin::UiState state;state.data=Admin::Backend(std::filesystem::path(argv[2])).Call({"snapshot",{}});state.data.revision=1;
            Check(state.data.role=="owner"&&state.data.maxDays>0,"Owner access is not usable");
            ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={Admin::Width,Admin::Height};io.DeltaTime=1.f/60;
            ConfigureUiTheme(1);io.Fonts->Build();for(int frame=0;frame<3;++frame){ImGui::NewFrame();Admin::Draw(state);ImGui::Render();}
            Check(RenderPpm("owner-profile.ppm",1),"Owner UI render failed");ImGui::DestroyContext();std::cout<<state.data.name<<" owner verified";return 0;
        }
        if(argc>=4&&std::wstring(argv[1])==L"rpc"){
            Admin::Request request;request.command=Utf8(argv[3]);for(int i=4;i<argc;++i)request.args.push_back(Utf8(argv[i]));
            auto model=Admin::Backend(std::filesystem::path(argv[2])).Call(request);
            std::cout<<Admin::EncodeRequest({"RESULT",{model.role,model.name,std::to_string(model.maxDays),model.output,model.filename,model.requestText,std::to_string(model.licenses.size())}});return 0;
        }
        int rendered=0;
        Check(FindResourceW(nullptr,MAKEINTRESOURCEW(1),RT_GROUP_ICON)!=nullptr,"Admin PE icon missing");
        for(float scale:{1.f,1.5f,2.f}){
            ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={Admin::Width*scale,Admin::Height*scale};io.DeltaTime=1.f/60;ConfigureUiTheme(scale);io.Fonts->Build();Check(Ui::TabularWidth("11/11/2011 11:11")==Ui::TabularWidth("28/08/2088 08:08"),"date digits must be tabular");Check(UiTheme::regular->FindGlyph('1')->AdvanceX<UiTheme::regular->FindGlyph('0')->AdvanceX,"body digits must preserve natural font metrics");Check(UiTheme::heading->FontSize==18*scale&&UiTheme::regular->FontSize==14*scale&&UiTheme::caption->FontSize==12*scale,"strict font scale");Check(Ui::TabularWidth("30/09/2027 01:12")<=150*scale,"full expiry must fit a single column");
            for(int page=0;page<12;++page){
                Admin::UiState state;state.data.initialized=true;state.data.busy=false;state.data.hasKey=true;state.data.revision=1;state.data.total=1;state.data.maxDays=30;state.data.role="owner";state.data.name="OperadorTeste";state.data.user="OperadorTeste";state.data.stationId=std::string(32,'a');state.data.device=std::string(64,'8');
                state.page=static_cast<Admin::Page>(page<4?page:2);state.started=true;
                if(page==6)state.page=Admin::Page::Settings;
                if(page==7){state.page=Admin::Page::Help;state.credits=true;}
                if(page==8)state.page=Admin::Page::Licenses;
                if(page==4){state.data.role="operator";state.data.maxDays=0;}
                if(page==5){state.data.role="pending";state.data.hasKey=false;state.data.maxDays=0;}
                state.data.licenses.push_back({"abcd","Cliente de teste",std::string(64,'1'),"Equipe","token","29/10/2026 12:00","30",u8"Válida"});
                if(page==8){state.data.licenses.clear();state.data.total=0;}
                if(page==0){strcpy_s(state.customer,"Cliente de teste");strcpy_s(state.device,std::string(64,'1').c_str());}
                state.data.grants.push_back({"abcd","Integrante teste","29/09/2027 12:00","30","grant"});
                if(page==9){state.page=Admin::Page::Licenses;for(int i=0;i<18;++i){auto row=state.data.licenses[0];row.id=std::to_string(i);row.customer="Cliente "+std::to_string(i);state.data.licenses.push_back(row);}state.data.total=19;}
                if(page==10){state.page=Admin::Page::Team;state.data.grants.clear();}
                if(page==11){state.page=Admin::Page::Licenses;strcpy_s(state.search,"sem resultado");}
                for(int frame=0;frame<12;++frame){ImGui::NewFrame();Admin::Draw(state);ImGui::Render();}
                Check(Ui::DeviceFits(state.data.device,304),"full device ID exceeds client card");
                Check(RenderPpm("admin-native-"+std::to_string(page)+"-"+std::to_string(static_cast<int>(scale*100))+".ppm",1),"render failed");++rendered;
            }
            Admin::UiState state;state.data.initialized=true;state.data.busy=false;state.data.hasKey=true;state.data.revision=1;state.data.maxDays=3650;state.data.role="owner";state.data.device=std::string(64,'a');
            auto frame=[&](float x,float y,bool down){io.MousePos=Ui::P(x,y);io.MouseDown[0]=down;ImGui::NewFrame();Admin::Draw(state);ImGui::Render();};
            state.data.licenses.push_back({"test","Cliente",std::string(64,'1'),"Equipe","copy-exact-token","29/10/2026 12:00","30",u8"Válida"});
            strcpy_s(state.customer,"Cliente UI");strcpy_s(state.device,std::string(64,'1').c_str());
            frame(360,258,false);frame(360,258,true);frame(360,258,false);
            Check(!state.send&&!state.copy,"static header must not trigger actions");
            Check(RenderPpm("admin-header-hover-"+std::to_string(static_cast<int>(scale*100))+".ppm",1),"header hover render failed");
            frame(188,430,false);frame(188,430,true);frame(188,430,false);Check(state.send&&state.action.command=="issue_package","primary must generate ready ZIP");Check(state.action.args[2]=="30","UI must submit exactly 30 days");state.send=false;
            frame(188,478,true);frame(188,478,false);Check(state.send&&state.action.command=="issue","manual key path must remain available");Check(state.action.args[2]=="30","key must submit exactly 30 days");state.send=false;
            frame(743,290,false);frame(743,290,true);frame(743,290,false);Check(state.copy&&state.copyText=="copy-exact-token","inline row copy failed");state.copy=false;
            strcpy_s(state.search,"not-found");frame(500,200,false);Check(state.visibleLicenseCount==0,"live search did not filter");
            strcpy_s(state.search,"cli");frame(500,200,false);Check(state.visibleLicenseCount==1,"live search did not restore matching row");
            state.message="Preferência salva.";state.exportText="old-token";
            frame(740,96,false);frame(740,96,true);frame(740,96,false);Check(state.page==Admin::Page::Help,"in-app help navigation failed");
            state.send=false;frame(200,338,true);frame(200,338,false);Check(state.send&&state.action.command=="client_starter"&&state.action.args.empty(),"initial client ZIP must not require an ID");
            state.send=false;frame(600,338,true);frame(600,338,false);Check(state.send&&state.action.command=="team_starter"&&state.action.args.empty(),"initial team ZIP must not require a request");
            Check(state.message.empty()&&state.exportText.empty(),"navigation must clear footer and export actions");
            state.pendingFeedback=true;state.requestPage=Admin::Page::Licenses;state.data.notice="Late license result";state.data.output="late-token";state.data.issuedId="test";++state.data.revision;
            frame(740,96,false);Check(state.message.empty()&&state.exportText.empty(),"late reply must not leak into Help");
            state.page=Admin::Page::Licenses;state.pendingFeedback=true;state.requestPage=state.page;state.data.notice="30 dias";++state.data.revision;strcpy_s(state.search,"old filter");
            frame(500,200,false);Check(state.selectedLicense=="test"&&state.search[0]==0&&state.message=="30 dias","fresh issue must select exact new record and clear filter");
            state.data.output.clear();state.data.issuedId.clear();
            frame(580,96,true);frame(580,96,false);Check(state.page==Admin::Page::Settings,"settings tab failed");
            frame(330,236,true);frame(330,236,false);Check(state.saveSettings&&state.reducedMotion,"motion setting failed");
            frame(200,476,true);frame(200,476,false);Check(state.exportDiagnostics,"diagnostic action failed");
            state.page=Admin::Page::Station;frame(190,272,false);frame(190,272,true);frame(190,272,false);Check(state.copy&&state.copyText==state.data.device,"copy must preserve all 64 ID characters");
            frame(800,32,true);frame(800,32,false);Check(state.close,"custom close control failed");
            ImGui::DestroyContext();
        }
        std::cout<<rendered<<" native Admin renders and UI actions passed.\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
