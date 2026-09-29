#pragma once
struct LoaderPreviewState {
    int page=0;
    bool close=false,minimize=false;
    char license[2048]={};
    bool attempted=false;
};
void ConfigureLoaderPreview(float scale);
void DrawLoaderPreview(LoaderPreviewState& state);
