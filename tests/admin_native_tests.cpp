#include "../apps/admin/native/backend.h"
#include "../apps/admin/native/ui.h"
#include "../apps/shared/widgets.h"
#include "../apps/admin/native/settings.h"
#include "software_renderer.h"
#include <iostream>
#include <fstream>
#include <Windows.h>
void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::string Utf8(const wchar_t* value){int n=WideCharToMultiByte(CP_UTF8,0,value,-1,nullptr,0,nullptr,nullptr);std::string output(static_cast<size_t>(n),'\0');WideCharToMultiByte(CP_UTF8,0,value,-1,output.data(),n,nullptr,nullptr);output.pop_back();return output;}
int wmain(int argc,wchar_t** argv){
    try{
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
            ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={Admin::Width*scale,Admin::Height*scale};io.DeltaTime=1.f/60;ConfigureUiTheme(scale);io.Fonts->Build();Check(Ui::TabularWidth("11/11/2011 11:11")==Ui::TabularWidth("28/08/2088 08:08"),"date digits must be tabular");
            for(int page=0;page<9;++page){
                Admin::UiState state;state.data.initialized=true;state.data.busy=false;state.data.hasKey=true;state.data.revision=1;state.data.total=1;state.data.maxDays=30;state.data.role="owner";state.data.name="Proprietario";state.data.user="Windows";state.data.stationId=std::string(32,'a');state.data.device=std::string(64,'8');
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
                for(int frame=0;frame<12;++frame){ImGui::NewFrame();Admin::Draw(state);ImGui::Render();}
                Check(Ui::DeviceFits(state.data.device,304),"full device ID exceeds client card");
                Check(RenderPpm("admin-native-"+std::to_string(page)+"-"+std::to_string(static_cast<int>(scale*100))+".ppm",1),"render failed");++rendered;
            }
            Admin::UiState state;state.data.initialized=true;state.data.busy=false;state.data.hasKey=true;state.data.revision=1;state.data.maxDays=3650;state.data.role="owner";state.data.device=std::string(64,'a');
            auto frame=[&](float x,float y,bool down){io.MousePos=Ui::P(x,y);io.MouseDown[0]=down;ImGui::NewFrame();Admin::Draw(state);ImGui::Render();};
            state.data.licenses.push_back({"test","Cliente",std::string(64,'1'),"Equipe","copy-exact-token","29/10/2026 12:00","30",u8"Válida"});
            strcpy_s(state.customer,"Cliente UI");strcpy_s(state.device,std::string(64,'1').c_str());
            frame(188,460,false);frame(188,460,true);frame(188,460,false);Check(state.send&&state.action.command=="issue_package","primary must generate ready ZIP");Check(state.action.args[2]=="30","UI must submit exactly 30 days");state.send=false;
            frame(188,496,true);frame(188,496,false);Check(state.send&&state.action.command=="issue","manual key path must remain available");Check(state.action.args[2]=="30","key must submit exactly 30 days");state.send=false;
            frame(739,292,false);frame(739,292,true);frame(739,292,false);Check(state.copy&&state.copyText=="copy-exact-token","inline row copy failed");state.copy=false;
            strcpy_s(state.search,"not-found");frame(500,200,false);Check(state.visibleLicenseCount==0,"live search did not filter");
            strcpy_s(state.search,"cli");frame(500,200,false);Check(state.visibleLicenseCount==1,"live search did not restore matching row");
            state.message="Preferência salva.";state.exportText="old-token";
            frame(740,96,false);frame(740,96,true);frame(740,96,false);Check(state.page==Admin::Page::Help,"in-app help navigation failed");
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
