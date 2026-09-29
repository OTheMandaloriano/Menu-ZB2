#include "controller.h"
#include <algorithm>
#include <ctime>
#include <stdexcept>
namespace {
std::string Date(int64_t value){time_t date=static_cast<time_t>(value);tm result{};gmtime_s(&result,&date);char text[40]{};strftime(text,sizeof(text),"%d/%m/%Y %H:%M UTC",&result);return text;}
}
LoaderController::LoaderController(){worker_=std::thread(&LoaderController::Run,this);}
LoaderController::~LoaderController(){RequestStop();if(worker_.joinable())worker_.join();}
void LoaderController::RequestStop(){{std::lock_guard<std::mutex> lock(mutex_);stop_=true;}wake_.notify_one();}
LoaderSnapshot LoaderController::Snapshot(){std::lock_guard<std::mutex> lock(mutex_);return snapshot_;}
void LoaderController::Publish(LoaderSnapshot value){std::lock_guard<std::mutex> lock(mutex_);snapshot_=std::move(value);}
bool LoaderController::Activate(std::string token){
    std::lock_guard<std::mutex> lock(mutex_);if(busy_||pending_!=Action::None)return false;
    token_=std::move(token);pending_=Action::Activate;busy_=true;wake_.notify_one();return true;
}
bool LoaderController::Load(){
    std::lock_guard<std::mutex> lock(mutex_);if(busy_||pending_!=Action::None||snapshot_.phase!=LoaderPhase::Ready)return false;
    pending_=Action::Load;busy_=true;wake_.notify_one();return true;
}
bool LoaderController::Retry(){
    std::lock_guard<std::mutex> lock(mutex_);if(busy_||pending_!=Action::None)return false;
    pending_=Action::Retry;busy_=true;wake_.notify_one();return true;
}
void LoaderController::Run(){
    LoaderSnapshot state;LoaderServices::Stored stored;std::filesystem::path root;
    std::vector<unsigned char> publicKey;bool initialized=false;unsigned long failedPid=0;std::string actionError;
    try{root=LoaderServices::DataDirectory();state.device=License::DeviceId();publicKey=LoaderServices::PublicKey();state.version=LoaderServices::BundleVersion();LoaderServices::VerifyBundle();stored=LoaderServices::ReadState(root);initialized=true;}
    catch(const std::exception& error){state.phase=LoaderPhase::Error;state.message=error.what();Publish(state);}
    while(true){
        Action action=Action::None;std::string token;
        {std::lock_guard<std::mutex> lock(mutex_);if(stop_)break;action=pending_;pending_=Action::None;token=token_;}
        if(initialized)try{
            if(action!=Action::None)actionError.clear();
            const auto now=License::Now();LoaderServices::CheckClock(stored,now);
            if(action==Action::Activate){
                state.phase=LoaderPhase::Checking;state.message=u8"Validando assinatura...";Publish(state);
                token=License::Normalize(token);
                auto result=License::Validate(token,state.device,now,publicKey);
                if(!result.valid)throw std::runtime_error(result.error);
                stored={token,std::max(stored.lastSeen,now)};LoaderServices::SaveState(root,stored);
            }
            auto license=License::Validate(stored.token,state.device,now,publicKey);
            state.licensed=license.valid;state.expiry=license.valid?Date(license.expires):"";
            auto game=LoaderServices::FindGame();state.pid=game.pid;
            if(action==Action::Load){
                if(!license.valid)throw std::runtime_error(license.error);
                if(game.pid==failedPid)throw std::runtime_error("Reinicie o jogo antes de tentar outro carregamento.");
                state.phase=LoaderPhase::Loading;state.message=u8"Verificando e instalando o pacote...";Publish(state);
                auto runtime=LoaderServices::InstallBundle(root);
                {std::lock_guard<std::mutex> lock(mutex_);if(stop_)return;}
                // Record time before starting the helper; a failure must not erase the checkpoint.
                const auto loadTime=License::Now();LoaderServices::CheckClock(stored,loadTime);
                auto recheck=License::Validate(stored.token,state.device,loadTime,publicKey);
                if(!recheck.valid)throw std::runtime_error(recheck.error);
                stored.lastSeen=std::max(stored.lastSeen,loadTime);LoaderServices::SaveState(root,stored);
                failedPid=game.pid;
                state.message=u8"Carregando o menu. Aguarde a confirmação...";Publish(state);
                LoaderServices::LoadRuntime(game,runtime);game.loaded=true;failedPid=0;
            }
            if(!license.valid){state.phase=LoaderPhase::Activation;state.message=stored.token.empty()?u8"Ative seu acesso neste computador.":license.error;}
            else if(game.loaded){state.phase=LoaderPhase::Success;state.message="Menu carregado. Pressione INSERT no jogo.";}
            else if(!game.pid){state.phase=LoaderPhase::Waiting;state.message="Abra o Zumbi Blocks 2 para continuar.";failedPid=0;}
            else if(game.pid==failedPid){state.phase=LoaderPhase::Error;state.message="Carregamento anterior falhou. Reinicie o jogo.";}
            else{state.phase=LoaderPhase::Ready;state.message="Entre no mapa e confirme abaixo.";}
            if(license.valid && now-stored.lastSeen>=60){stored.lastSeen=now;LoaderServices::SaveState(root,stored);}
            if(!actionError.empty()){state.phase=LoaderPhase::Error;state.message=actionError;}
            Publish(state);
        }catch(const std::exception& error){state.phase=LoaderPhase::Error;state.message=error.what();if(action!=Action::None)actionError=state.message;Publish(state);}
        std::unique_lock<std::mutex> lock(mutex_);busy_=false;
        wake_.wait_for(lock,std::chrono::seconds(1),[this]{return stop_||pending_!=Action::None;});
    }
}
