#include <Windows.h>
#include <cstdio>
#include "../shared/graphics.h"
#include <windowsx.h>
#include <dwmapi.h>
#include <commdlg.h>
#include <filesystem>
#include <fstream>

#include "../../imgui/imgui.h"
#include "../../imgui/imgui_impl_win32.h"
#include "../../imgui/imgui_impl_dx11.h"
#include "ui.h"
#include "window.h"


namespace {
AppGraphics graphics;
UINT resizeWidth=0,resizeHeight=0;
float pendingDpi=1;bool dpiChanged=false;

}
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK WindowProc(HWND window,UINT message,WPARAM w,LPARAM l){
    if(ImGui_ImplWin32_WndProcHandler(window,message,w,l))return 1;
    switch(message){
    case WM_SIZE:if(w!=SIZE_MINIMIZED){resizeWidth=LOWORD(l);resizeHeight=HIWORD(l);}return 0;
    case WM_DPICHANGED:{pendingDpi=HIWORD(w)/96.f;dpiChanged=true;auto rect=reinterpret_cast<RECT*>(l);SetWindowPos(window,nullptr,rect->left,rect->top,rect->right-rect->left,rect->bottom-rect->top,SWP_NOZORDER|SWP_NOACTIVATE);return 0;}
    case WM_NCHITTEST:{
        POINT point={GET_X_LPARAM(l),GET_Y_LPARAM(l)};ScreenToClient(window,&point);
        float dpi=GetDpiForWindow(window)/96.f;
        if(point.y>=0 && point.y<48*dpi && point.x<344*dpi)return HTCAPTION;
        break;
    }
    case WM_DESTROY:PostQuitMessage(0);return 0;
    }
    return DefWindowProcW(window,message,w,l);
}
int RunLoaderWindow(HINSTANCE instance,int show){
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    WNDCLASSW wc={};wc.lpfnWndProc=WindowProc;wc.hInstance=instance;wc.lpszClassName=L"ZB2Loader";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hIcon=LoadIconW(instance,MAKEINTRESOURCEW(1));
    if(!RegisterClassW(&wc))return 1;
    HWND window=CreateWindowW(wc.lpszClassName,L"ZB2 Menu",WS_POPUP|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,LoaderWidth,LoaderHeight,nullptr,nullptr,instance,nullptr);
    if(!window || !graphics.Initialize(window)){if(window)DestroyWindow(window);UnregisterClassW(wc.lpszClassName,instance);return 2;}
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;
    pendingDpi=GetDpiForWindow(window)/96.f;ConfigureUiTheme(pendingDpi);
    RECT area={};SystemParametersInfoW(SPI_GETWORKAREA,0,&area,0);
    int width=int(LoaderWidth*pendingDpi),height=int(LoaderHeight*pendingDpi);
    SetWindowPos(window,nullptr,area.left+(area.right-area.left-width)/2,area.top+(area.bottom-area.top-height)/2,width,height,SWP_NOZORDER|SWP_NOACTIVATE);
    const DWORD corners=2;DwmSetWindowAttribute(window,33,&corners,sizeof(corners));
    LoaderUiState state;
    LoaderController controller;
    bool win32=ImGui_ImplWin32_Init(window);bool dx=win32 && ImGui_ImplDX11_Init(graphics.Device(),graphics.Context());
    if(!dx){if(win32)ImGui_ImplWin32_Shutdown();ImGui::DestroyContext();graphics.Shutdown();DestroyWindow(window);UnregisterClassW(wc.lpszClassName,instance);return 3;}
    ShowWindow(window,show);bool done=false;
    while(!done){
        MSG msg;while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);if(msg.message==WM_QUIT)done=true;}
        if(done)break;if(IsIconic(window)){Sleep(30);continue;}
        if(resizeWidth && resizeHeight){if(!graphics.Resize(resizeWidth,resizeHeight))break;resizeWidth=resizeHeight=0;}
        if(dpiChanged){ImGui_ImplDX11_InvalidateDeviceObjects();ConfigureUiTheme(pendingDpi);dpiChanged=false;}
        state.snapshot=controller.Snapshot();
        ImGui_ImplDX11_NewFrame();ImGui_ImplWin32_NewFrame();ImGui::NewFrame();DrawLoader(state);ImGui::Render();
        if(state.activate){controller.Activate(state.license);state.activate=false;}
        if(state.load){controller.Load();state.load=false;}
        if(state.autoChanged){controller.SetAutoInject(state.autoValue);state.autoChanged=false;}
        if(state.retry){controller.Retry();state.retry=false;}
        if(state.importLicense){
            state.importLicense=false;wchar_t path[32768]{};OPENFILENAMEW dialog{};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=window;
            dialog.lpstrFilter=L"Licenca ZB2 (*.zb2license)\0*.zb2license\0\0";dialog.lpstrFile=path;dialog.nMaxFile=32768;
            dialog.lpstrTitle=L"Abrir licenca recebida";dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
            if(GetOpenFileNameW(&dialog))try{
                std::ifstream file(std::filesystem::path(path),std::ios::binary|std::ios::ate);
                const auto size=file.tellg();if(!file || size<=0 || size>8192)throw std::runtime_error(u8"Arquivo incorreto. Escolha a licença recebida.");
                std::string content(static_cast<size_t>(size),'\0');file.seekg(0);file.read(content.data(),size);
                if(!file)throw std::runtime_error(u8"Não foi possível ler a licença completa.");
                content=License::Normalize(content);
                if(content.size()>4096 || (content.rfind("ZB2L1.",0)!=0 && content.rfind("ZB2L2.",0)!=0))throw std::runtime_error(u8"Isso não é uma licença .zb2license.");
                strcpy_s(state.license,content.c_str());state.localMessage="Arquivo importado. Clique em Ativar.";
            }catch(const std::exception& error){state.localMessage=error.what();}
        }
        if(state.close){controller.RequestStop();PostMessageW(window,WM_CLOSE,0,0);state.close=false;}
        if(state.minimize){ShowWindow(window,SW_MINIMIZE);state.minimize=false;}
        if(!graphics.Present(ImGui::GetDrawData()))break;
    }
    controller.RequestStop();
    ImGui_ImplDX11_Shutdown();ImGui_ImplWin32_Shutdown();ImGui::DestroyContext();
    graphics.Shutdown();if(IsWindow(window))DestroyWindow(window);UnregisterClassW(wc.lpszClassName,instance);return 0;
}
