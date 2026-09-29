#include "graphics.h"
#include "../../imgui/imgui.h"
#include "../../imgui/imgui_impl_dx11.h"
#pragma comment(lib,"d3d11.lib")

bool LoaderGraphics::CreateTarget(){
    Microsoft::WRL::ComPtr<ID3D11Texture2D> back;
    return SUCCEEDED(swap->GetBuffer(0,IID_PPV_ARGS(&back))) &&
        SUCCEEDED(device->CreateRenderTargetView(back.Get(),nullptr,&target));
}
bool LoaderGraphics::Initialize(HWND window){
    Shutdown();
    DXGI_SWAP_CHAIN_DESC desc={};
    desc.BufferCount=2;desc.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.OutputWindow=window;
    desc.SampleDesc.Count=1;desc.Windowed=TRUE;desc.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;
    const D3D_DRIVER_TYPE drivers[]={D3D_DRIVER_TYPE_HARDWARE,D3D_DRIVER_TYPE_WARP};
    for(auto type:drivers){
        if(SUCCEEDED(D3D11CreateDeviceAndSwapChain(nullptr,type,nullptr,0,nullptr,0,D3D11_SDK_VERSION,
            &desc,&swap,&device,nullptr,&context)) && CreateTarget())return true;
        Shutdown();
    }
    return false;
}
bool LoaderGraphics::Resize(UINT width,UINT height){
    if(!context || !swap || !width || !height)return false;
    context->OMSetRenderTargets(0,nullptr,nullptr);target.Reset();
    return SUCCEEDED(swap->ResizeBuffers(0,width,height,DXGI_FORMAT_UNKNOWN,0)) && CreateTarget();
}
bool LoaderGraphics::Present(ImDrawData* data){
    if(!target || !context || !swap)return false;
    const float clear[]={27/255.f,28/255.f,31/255.f,1};auto view=target.Get();
    context->OMSetRenderTargets(1,&view,nullptr);context->ClearRenderTargetView(view,clear);
    ImGui_ImplDX11_RenderDrawData(data);
    return SUCCEEDED(swap->Present(1,0));
}
void LoaderGraphics::Shutdown(){
    if(context)context->ClearState();
    target.Reset();swap.Reset();context.Reset();device.Reset();
}
