#pragma once
#include "controller.h"
struct LoaderUiState {
    bool close=false,minimize=false,activate=false,load=false,retry=false,inMap=false,about=false,legal=false;
    unsigned long lastPid=0;
    char license[2048]={};
    LoaderSnapshot snapshot;
};
void ConfigureLoaderTheme(float scale);
void DrawLoader(LoaderUiState& state);
