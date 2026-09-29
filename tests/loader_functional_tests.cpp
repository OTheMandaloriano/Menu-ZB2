#include "../apps/loader/ui.h"
#include "../apps/loader/theme.h"
#include "software_renderer.h"
#include <Windows.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace fs=std::filesystem;
void Require(bool value,const char* text){if(!value)throw std::runtime_error(text);}
std::string Read(const fs::path& path){std::ifstream in(path,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
int wmain(int argc,wchar_t** argv){
    try{
        if(argc>1 && std::wstring(argv[1])==L"device"){std::cout<<License::DeviceId();return 0;}
        if(argc==5 && std::wstring(argv[1])==L"validate"){
            auto token=Read(argv[2]);while(!token.empty()&&(token.back()=='\r'||token.back()=='\n'))token.pop_back();
            const std::wstring wideDevice=argv[3];std::string device;
            for(wchar_t ch:wideDevice){if(ch>127)return 2;device.push_back(static_cast<char>(ch));}
            auto result=License::Validate(token,device,std::stoll(argv[4]),LoaderServices::PublicKey());
            std::cout<<(result.valid?"VALID":result.error);return result.valid?0:2;
        }
        if(argc==3 && std::wstring(argv[1])==L"install"){
            std::cout<<LoaderServices::InstallBundle(fs::path(argv[2])).u8string();return 0;
        }
        if(argc==2 && std::wstring(argv[1])==L"scan"){
            auto game=LoaderServices::FindGame();std::cout<<"pid="<<game.pid<<" loaded="<<game.loaded;return 0;
        }
        if(argc==3 && std::wstring(argv[1])==L"state"){
            fs::path root=argv[2];LoaderServices::Stored state{"test-token",1000};
            LoaderServices::SaveState(root,state);auto read=LoaderServices::ReadState(root);
            Require(read.token==state.token && read.lastSeen==1000,"DPAPI roundtrip");
            LoaderServices::CheckClock(read,1000);bool rejected=false;
            try{LoaderServices::CheckClock(read,879);}catch(...){rejected=true;}Require(rejected,"rollback detection");
            std::ofstream corrupt(root/"license.dat",std::ios::binary|std::ios::trunc);corrupt<<"invalid";corrupt.close();
            rejected=false;try{LoaderServices::ReadState(root);}catch(...){rejected=true;}Require(rejected,"corrupted DPAPI state");
            std::cout<<"DPAPI and rollback checks passed";return 0;
        }
        LoaderServices::VerifyBundle();
        for(float scale:{1.f,1.5f,2.f}){
            ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={LoaderWidth*scale,LoaderHeight*scale};io.DeltaTime=1.f/60;
            ConfigureLoaderTheme(scale);io.Fonts->Build();
            for(int page=0;page<9;++page){
                LoaderUiState state;state.snapshot.version="1.1-local";state.snapshot.device=std::string(64,'a');state.snapshot.expiry="29/10/2026 14:00 UTC";
                state.snapshot.phase=static_cast<LoaderPhase>(page<7?page:2);state.snapshot.licensed=page>=2;state.snapshot.pid=page>=3?1234:0;
                const char* messages[]={u8"Verificando licença e pacote...",u8"Ative seu acesso neste computador.","Abra o Zumbi Blocks 2 para continuar.","Entre no mapa e confirme abaixo.",u8"Carregando o menu. Aguarde a confirmação...","Menu carregado. Pressione INSERT no jogo.",u8"Falha ao carregar. Confira as permissões e tente novamente."};
                state.snapshot.message=messages[page<7?page:2];
                state.about=page>=7;state.legal=page==8;
                for(int frame=0;frame<2;++frame){ImGui::NewFrame();DrawLoader(state);ImGui::Render();}
                Require(RenderPpm("loader-"+std::to_string(page)+"-"+std::to_string(static_cast<int>(scale*100))+".ppm",1),"render");
            }
            LoaderUiState state;state.snapshot.phase=LoaderPhase::Ready;state.snapshot.licensed=true;state.snapshot.pid=77;state.snapshot.version="1.0";
            auto frame=[&](float x,float y,bool down){io.MousePos={x*scale,y*scale};io.MouseDown[0]=down;ImGui::NewFrame();DrawLoader(state);ImGui::Render();};
            frame(200,338,false);frame(200,338,true);frame(200,338,false);Require(!state.load,"map confirmation must gate loading");
            state.inMap=true;frame(200,338,true);frame(200,338,false);Require(state.load,"confirmed load click");
            state.load=false;state.snapshot.pid=78;frame(200,338,false);Require(!state.inMap,"process change resets map confirmation");
            state.snapshot.phase=LoaderPhase::Error;frame(200,338,true);frame(200,338,false);Require(state.retry,"error recovery click");
            state.snapshot.phase=LoaderPhase::Activation;state.snapshot.licensed=false;
            frame(200,284,true);frame(200,284,false);Require(!state.activate,"empty license must not submit");
            strcpy_s(state.license,"ZB2L1.test");frame(200,284,true);frame(200,284,false);Require(state.activate,"activation click");
            frame(100,332,true);frame(100,332,false);Require(state.importLicense,"file import click");
            frame(368,28,true);frame(368,28,false);Require(state.close,"close click");
            ImGui::DestroyContext();
        }
        std::cout<<"27 renders and interaction checks passed; signed package verified.\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
