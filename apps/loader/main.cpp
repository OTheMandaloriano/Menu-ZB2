#include <Windows.h>
#include <cstdio>
#include <d3d11.h>
#include <wrl/client.h>
#include "../../imgui/imgui.h"
#include "../../imgui/imgui_impl_win32.h"
#include "../../imgui/imgui_impl_dx11.h"
#include "preview_ui.h"
#pragma comment(lib,"d3d11.lib")
using Microsoft::WRL::ComPtr;
namespace {
ComPtr<ID3D11Device> device;
ComPtr<ID3D11DeviceContext> context;
ComPtr<IDXGISwapChain> swap;
ComPtr<ID3D11RenderTargetView> target;
UINT resizeWidth=0,resizeHeight=0;
bool CreateTarget(){ComPtr<ID3D11Texture2D> back;return SUCCEEDED(swap->GetBuffer(0,IID_PPV_ARGS(&back))) && SUCCEEDED(device->CreateRenderTargetView(back.Get(),nullptr,&target));}
bool CreateGraphics(HWND window){
    DXGI_SWAP_CHAIN_DESC desc={};desc.BufferCount=2;desc.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.OutputWindow=window;desc.SampleDesc.Count=1;
    desc.Windowed=TRUE;desc.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;
    auto result=D3D11CreateDeviceAndSwapChain(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&desc,&swap,&device,nullptr,&context);
    if(FAILED(result))result=D3D11CreateDeviceAndSwapChain(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&desc,&swap,&device,nullptr,&context);
    return SUCCEEDED(result) && CreateTarget();
}
}
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK WindowProc(HWND window,UINT message,WPARAM w,LPARAM l){
    if(ImGui_ImplWin32_WndProcHandler(window,message,w,l))return 1;
    switch(message){
    case WM_SIZE:if(w!=SIZE_MINIMIZED){resizeWidth=LOWORD(l);resizeHeight=HIWORD(l);}return 0;
    case WM_DPICHANGED:{auto rect=reinterpret_cast<RECT*>(l);SetWindowPos(window,nullptr,rect->left,rect->top,rect->right-rect->left,rect->bottom-rect->top,SWP_NOZORDER|SWP_NOACTIVATE);return 0;}
    case WM_DESTROY:PostQuitMessage(0);return 0;
    }
    return DefWindowProcW(window,message,w,l);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int show){
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    WNDCLASSW wc={};wc.lpfnWndProc=WindowProc;wc.hInstance=instance;wc.lpszClassName=L"ZB2LoaderPreview";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
    if(!RegisterClassW(&wc))return 1;
    HWND window=CreateWindowW(wc.lpszClassName,L"ZB2 Menu | Previa do loader",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,780,570,nullptr,nullptr,instance,nullptr);
    if(!window || !CreateGraphics(window)){if(window)DestroyWindow(window);UnregisterClassW(wc.lpszClassName,instance);return 2;}
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();auto& style=ImGui::GetStyle();style.WindowPadding=ImVec2(22,18);style.FramePadding=ImVec2(10,8);style.ItemSpacing=ImVec2(10,10);style.FrameRounding=5;
    style.Colors[ImGuiCol_WindowBg]=ImVec4(.055f,.065f,.075f,1);style.Colors[ImGuiCol_ChildBg]=ImVec4(.07f,.08f,.09f,1);
    style.Colors[ImGuiCol_Button]=ImVec4(.10f,.30f,.29f,1);style.Colors[ImGuiCol_ButtonHovered]=ImVec4(.14f,.40f,.37f,1);
    wchar_t windows[MAX_PATH]={};GetWindowsDirectoryW(windows,MAX_PATH);
    char fontPath[MAX_PATH*3]={};char base[MAX_PATH*3]={};WideCharToMultiByte(CP_UTF8,0,windows,-1,base,sizeof(base),nullptr,nullptr);
    snprintf(fontPath,sizeof(fontPath),"%s/Fonts/segoeui.ttf",base);
    if(!io.Fonts->AddFontFromFileTTF(fontPath,18.f))io.Fonts->AddFontDefault();
    bool win32=ImGui_ImplWin32_Init(window);bool dx=win32 && ImGui_ImplDX11_Init(device.Get(),context.Get());
    if(!dx){if(win32)ImGui_ImplWin32_Shutdown();ImGui::DestroyContext();DestroyWindow(window);return 3;}
    ShowWindow(window,show);bool done=false;
    while(!done){
        MSG msg;while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);if(msg.message==WM_QUIT)done=true;}
        if(done)break;if(IsIconic(window)){Sleep(30);continue;}
        if(resizeWidth && resizeHeight){context->OMSetRenderTargets(0,nullptr,nullptr);target.Reset();if(FAILED(swap->ResizeBuffers(0,resizeWidth,resizeHeight,DXGI_FORMAT_UNKNOWN,0)) || !CreateTarget())break;resizeWidth=resizeHeight=0;}
        ImGui_ImplDX11_NewFrame();ImGui_ImplWin32_NewFrame();ImGui::NewFrame();DrawLoaderPreview();ImGui::Render();
        const float clear[]={.055f,.065f,.075f,1};auto view=target.Get();context->OMSetRenderTargets(1,&view,nullptr);context->ClearRenderTargetView(view,clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());if(FAILED(swap->Present(1,0)))break;
    }
    ImGui_ImplDX11_Shutdown();ImGui_ImplWin32_Shutdown();ImGui::DestroyContext();context->ClearState();
    target.Reset();swap.Reset();context.Reset();device.Reset();if(IsWindow(window))DestroyWindow(window);UnregisterClassW(wc.lpszClassName,instance);return 0;
}
