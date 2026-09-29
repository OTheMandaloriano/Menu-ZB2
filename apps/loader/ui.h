#pragma once
#include "controller.h"
struct LoaderUiState {
    bool close=false,minimize=false,activate=false,load=false,retry=false,inMap=false,about=false,legal=false,importLicense=false;
    unsigned long lastPid=0;
    char license[8193]={};
    std::string localMessage;
    LoaderSnapshot snapshot;
};
constexpr int LoaderWidth=400;
constexpr int LoaderHeight=384;
void ConfigureLoaderTheme(float scale);
void DrawLoader(LoaderUiState& state);
