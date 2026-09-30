#pragma once
#include "controller.h"
struct LoaderUiState {
    bool close=false,minimize=false,activate=false,load=false,retry=false,about=false,help=false,importLicense=false,autoChanged=false,autoValue=true;
    unsigned long lastPid=0;
    char license[8193]={};
    std::string localMessage;
    LoaderSnapshot snapshot;
};
constexpr int LoaderWidth=440;
constexpr int LoaderHeight=270;
void ConfigureUiTheme(float scale);
void DrawLoader(LoaderUiState& state);
