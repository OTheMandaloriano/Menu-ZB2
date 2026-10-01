#pragma once
#include "model.h"
#include "settings.h"
namespace Admin {
constexpr int Width=840,Height=600;
enum class Page {Licenses,Team,Station,Help,Settings,Recovery};
enum class Picker {None,Request,Authorization,Owner};
struct UiState {
    Snapshot data;Page page=Page::Licenses;
    bool close=false,minimize=false,send=false,save=false,copy=false,saveAfterReply=false;
    Page requestPage=Page::Licenses;bool pendingFeedback=false;
    bool started=false,credits=false;unsigned revision=0;float fade=1;
    bool reducedMotion=false,saveSettings=false,exportDiagnostics=false;
    int openFolder=0;
    bool exportRecovery=false,restoreRecovery=false,recoveryAcknowledged=false;
    char recoveryPassword[256]{},recoveryConfirm[256]{};
    bool inspectCache=false,cleanCache=false,cacheReviewed=false;CachePlan cachePlan;std::string cacheSummary;
    int visibleLicenseCount=0;
    std::string dataPath,applicationPath;
    Picker pick=Picker::None;Request action;
    char customer[256]{},device[128]{},days[8]="30",search[256]{},name[256]{},grantDays[8]="365",maxDays[8]="30";
    std::string requestText,requestName,requestId,exportText,exportName,copyText,selectedLicense,selectedGrant,message;
};
void Draw(UiState& state);
}
