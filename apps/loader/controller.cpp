#include "controller.h"
#include "readiness.h"
#include "auto_inject.h"
#include <algorithm>
#include <ctime>
#include <stdexcept>
#include <Windows.h>
namespace {
std::string Date(int64_t value){time_t date=static_cast<time_t>(value);tm result{};gmtime_s(&result,&date);char text[40]{};strftime(text,sizeof(text),"%d/%m/%Y %H:%M UTC",&result);return text;}
}
LoaderController::LoaderController(){worker_=std::thread(&LoaderController::Run,this);}
LoaderController::~LoaderController(){RequestStop();if(worker_.joinable())worker_.join();}
void LoaderController::RequestStop(){{std::lock_guard<std::mutex> lock(mutex_);stop_=true;}wake_.notify_one();}
void LoaderController::SetAutoInject(bool value){autoInject_.store(value);{std::lock_guard<std::mutex> lock(mutex_);preferencesDirty_=true;snapshot_.autoInject=value;}wake_.notify_one();}
LoaderSnapshot LoaderController::Snapshot(){std::lock_guard<std::mutex> lock(mutex_);return snapshot_;}
void LoaderController::Publish(LoaderSnapshot value){value.autoInject=autoInject_.load();std::lock_guard<std::mutex> lock(mutex_);snapshot_=std::move(value);}
bool LoaderController::Activate(std::string token){std::lock_guard<std::mutex> lock(mutex_);if(busy_||pending_!=Action::None)return false;token_=std::move(token);pending_=Action::Activate;busy_=true;wake_.notify_one();return true;}
bool LoaderController::Load(){std::lock_guard<std::mutex> lock(mutex_);if(busy_||pending_!=Action::None||!snapshot_.licensed||!snapshot_.pid||snapshot_.phase==LoaderPhase::Loading||snapshot_.phase==LoaderPhase::Success)return false;pending_=Action::Load;busy_=true;wake_.notify_one();return true;}
bool LoaderController::Retry(){std::lock_guard<std::mutex> lock(mutex_);if(busy_||pending_!=Action::None)return false;pending_=Action::Retry;busy_=true;wake_.notify_one();return true;}
void LoaderController::Run(){
    LoaderSnapshot state;LoaderServices::Stored stored;std::filesystem::path root,runtime;
    std::vector<unsigned char> publicKey;bool initialized=false;std::string actionError;
    LoaderServices::Readiness readiness;AutoInjectGate gate;
    try{root=LoaderServices::DataDirectory();state.device=License::DeviceId();publicKey=LoaderServices::PublicKey();state.version=LoaderServices::BundleVersion();LoaderServices::VerifyBundle();stored=LoaderServices::ReadState(root);autoInject_.store(LoaderServices::ReadAuto(root));
        LoaderServices::CheckClock(stored,License::Now());
        wchar_t executable[32768]{};GetModuleFileNameW(nullptr,executable,32768);
        LoaderServices::ImportPackageLicense(executable,root,state.device,License::Now(),publicKey,stored,actionError);
        initialized=true;}
    catch(const std::exception& error){state.phase=LoaderPhase::Error;state.message=error.what();Publish(state);}
    auto stopped=[this](){std::lock_guard<std::mutex> lock(mutex_);return stop_;};
    while(true){
        Action action;std::string token;bool savePreferences;bool attempted=false;
        {std::lock_guard<std::mutex> lock(mutex_);if(stop_)break;action=pending_;pending_=Action::None;token=token_;savePreferences=preferencesDirty_;preferencesDirty_=false;}
        if(initialized)try{
            if(savePreferences)LoaderServices::SaveAuto(root,autoInject_.load());
            if(action!=Action::None)actionError.clear();
            auto now=License::Now();LoaderServices::CheckClock(stored,now);
            if(action==Action::Activate){
                state.phase=LoaderPhase::Checking;state.message=u8"Validando assinatura...";Publish(state);token=License::Normalize(token);
                auto result=License::Validate(token,state.device,now,publicKey);if(!result.valid)throw std::runtime_error(result.error);
                stored={token,std::max(stored.lastSeen,now)};LoaderServices::SaveState(root,stored);
            }
            auto license=License::Validate(stored.token,state.device,now,publicKey);state.licensed=license.valid;state.expiry=license.valid?Date(license.expires):"";
            auto game=LoaderServices::FindGame();state.pid=game.pid;
            if(gate.process!=game.created){gate.Observe(game.created);actionError.clear();}
            if(action==Action::Load&&game.pid)gate.manual=true;
            const bool assemblies=!game.loaded&&readiness.Poll(game);
            if(gate.CanProbe(license.valid,autoInject_.load(),assemblies,game.loaded)&&!game.probeLoaded){
                state.phase=LoaderPhase::Loading;state.message=u8"Preparando monitor de prontidão...";Publish(state);
                if(runtime.empty())runtime=LoaderServices::InstallBundle(root);
                if(stopped())return;
                if(gate.Wanted(autoInject_.load())){auto recheck=License::Validate(stored.token,state.device,License::Now(),publicKey);if(!recheck.valid)throw std::runtime_error(recheck.error);gate.probeAttempt=game.created;attempted=true;LoaderServices::PrepareReadiness(game,runtime);game=LoaderServices::FindGame();}
            }
            state.sceneReady=game.loaded||(assemblies&&readiness.World()&&LoaderServices::SceneReady(game));
            if(gate.CanInject(license.valid,autoInject_.load(),state.sceneReady,game.loaded)){
                state.phase=LoaderPhase::Loading;state.message=u8"Cena pronta. Carregando menu...";Publish(state);
                if(runtime.empty())runtime=LoaderServices::InstallBundle(root);
                if(stopped())return;
                now=License::Now();LoaderServices::CheckClock(stored,now);auto recheck=License::Validate(stored.token,state.device,now,publicKey);if(!recheck.valid)throw std::runtime_error(recheck.error);
                stored.lastSeen=std::max(stored.lastSeen,now);LoaderServices::SaveState(root,stored);
                if(gate.Wanted(autoInject_.load())&&readiness.Poll(game)&&readiness.World()){
                    gate.menuAttempt=game.created;gate.manual=false;attempted=true;LoaderServices::LoadRuntime(game,runtime);game.loaded=true;
                }
            }
            if(!license.valid){state.phase=LoaderPhase::Activation;state.message=stored.token.empty()?u8"Ative seu acesso neste computador.":license.error;}
            else if(game.loaded){state.phase=LoaderPhase::Success;state.message="Menu carregado. Use INSERT no jogo.";}
            else if(!game.pid){state.phase=LoaderPhase::Waiting;state.message="Aguardando o Zumbi Blocks 2.";}
            else if(gate.failed||gate.menuAttempt==game.created||(gate.probeAttempt==game.created&&!game.probeLoaded)){state.phase=LoaderPhase::Error;state.message=u8"Tentativa não confirmada. Reinicie o jogo.";}
            else if(!assemblies){state.phase=LoaderPhase::Waiting;state.message=u8"Aguardando inicialização dos assemblies.";}
            else if(!state.sceneReady){state.phase=LoaderPhase::Waiting;state.message=gate.Wanted(autoInject_.load())?(readiness.World()?u8"Mapa carregando. Aguardando prontidão.":u8"Entre em uma partida para carregar o menu."):u8"Automático desligado. Injeção manual disponível.";}
            else{state.phase=LoaderPhase::Ready;state.message=u8"Cena pronta para injeção.";}
            if(license.valid&&now-stored.lastSeen>=60){stored.lastSeen=now;LoaderServices::SaveState(root,stored);}
            if(!actionError.empty()){state.phase=LoaderPhase::Error;state.message=actionError;}
            Publish(state);
        }catch(const std::exception& error){if(attempted)gate.failed=true;state.phase=LoaderPhase::Error;state.message=error.what();actionError=state.message;Publish(state);}
        std::unique_lock<std::mutex> lock(mutex_);busy_=false;
        wake_.wait_for(lock,std::chrono::milliseconds(250),[this]{return stop_||pending_!=Action::None||preferencesDirty_;});
    }
}
