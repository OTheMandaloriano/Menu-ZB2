#pragma once
#include "controller.h"
#include "navigation.h"
struct LoaderUiState {
    bool close=false,minimize=false,activate=false,load=false,retry=false,importLicense=false,autoChanged=false,autoValue=true;
    bool reducedMotion=false;
    unsigned long lastPid=0;
    char license[8193]={};
    std::string localMessage;
    LoaderNavigation navigation;
    LoaderPage lastRendered=LoaderPage::Library;
    float pageOpacity=1.f;
    LoaderSnapshot snapshot;
};
constexpr int LoaderWidth=760;
constexpr int LoaderHeight=500;
void ConfigureUiTheme(float scale);
void DrawLoader(LoaderUiState& state);
