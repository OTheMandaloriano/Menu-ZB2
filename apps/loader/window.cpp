#include <Windows.h>
#include <cstdio>
#include "graphics.h"
#include <windowsx.h>
#include <dwmapi.h>

#include "../../imgui/imgui.h"
#include "../../imgui/imgui_impl_win32.h"
#include "../../imgui/imgui_impl_dx11.h"
#include "ui.h"
#include "window.h"


namespace {
LoaderGraphics graphics;
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
        if(point.y>=0 && point.y<56*dpi && point.x<352*dpi)return HTCAPTION;
        break;
    }
    case WM_DESTROY:PostQuitMessage(0);return 0;
    }
    return DefWindowProcW(window,message,w,l);
}
int RunLoaderWindow(HINSTANCE instance,int show){
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    WNDCLASSW wc={};wc.lpfnWndProc=WindowProc;wc.hInstance=instance;wc.lpszClassName=L"ZB2Loader";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
    if(!RegisterClassW(&wc))return 1;
    HWND window=CreateWindowW(wc.lpszClassName,L"ZB2 Menu",WS_POPUP|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,460,260,nullptr,nullptr,instance,nullptr);
    if(!window || !graphics.Initialize(window)){if(window)DestroyWindow(window);UnregisterClassW(wc.lpszClassName,instance);return 2;}
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;
    pendingDpi=GetDpiForWindow(window)/96.f;ConfigureLoaderTheme(pendingDpi);
    RECT area={};SystemParametersInfoW(SPI_GETWORKAREA,0,&area,0);
    int width=int(460*pendingDpi),height=int(260*pendingDpi);
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
        if(dpiChanged){ImGui_ImplDX11_InvalidateDeviceObjects();ConfigureLoaderTheme(pendingDpi);dpiChanged=false;}
        state.snapshot=controller.Snapshot();
        ImGui_ImplDX11_NewFrame();ImGui_ImplWin32_NewFrame();ImGui::NewFrame();DrawLoader(state);ImGui::Render();
        if(state.activate){controller.Activate(state.license);state.activate=false;}
        if(state.load){controller.Load();state.load=false;}
        if(state.retry){controller.Retry();state.retry=false;}
        if(state.close){controller.RequestStop();PostMessageW(window,WM_CLOSE,0,0);state.close=false;}
        if(state.minimize){ShowWindow(window,SW_MINIMIZE);state.minimize=false;}
        if(!graphics.Present(ImGui::GetDrawData()))break;
    }
    controller.RequestStop();
    ImGui_ImplDX11_Shutdown();ImGui_ImplWin32_Shutdown();ImGui::DestroyContext();
    graphics.Shutdown();if(IsWindow(window))DestroyWindow(window);UnregisterClassW(wc.lpszClassName,instance);return 0;
}
