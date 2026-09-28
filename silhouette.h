#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include "imgui/imgui.h"
#include <algorithm>
#include <cstring>
#pragma comment(lib,"d3dcompiler.lib")

namespace Silhouette {
using Microsoft::WRL::ComPtr;
inline SRWLOCK maskLock=SRWLOCK_INIT;
inline ComPtr<ID3D11ShaderResourceView> published, frameMask;
inline ComPtr<ID3D11Device> shaderDevice;
inline ComPtr<ID3D11PixelShader> shader;
inline ComPtr<ID3D11Buffer> constants, previousConstants;
inline ID3D11DeviceContext* frameContext=nullptr;
struct Parameters { float front[4],behind[4],width,padding[3]; };
inline const char* source=R"(
Texture2D mask : register(t0);
cbuffer Options : register(b0) { float4 front; float4 behind; float width; float3 padding; };
float4 main(float4 position:SV_POSITION,float4 color:COLOR0,float2 uv:TEXCOORD0):SV_Target {
    uint w,h; mask.GetDimensions(w,h);
    int2 p=clamp(int2(uv*float2(w,h)),int2(0,0),int2(w-1,h-1));
    float2 center=mask.Load(int3(p,0)).rg;
    if(max(center.r,center.g)>0.01) return 0;
    float2 edge=0;
    int radius=clamp((int)width,1,6);
    for(int y=-radius;y<=radius;++y) for(int x=-radius;x<=radius;++x) {
        if(x*x+y*y>radius*radius) continue;
        int2 q=p+int2(x,y);
        if(any(q<0)||q.x>=w||q.y>=h) continue;
        edge=max(edge,mask.Load(int3(q,0)).rg);
    }
    return edge.r>0.01 ? front : (edge.g>0.01 ? behind : float4(0,0,0,0));
})";

inline bool EnsureShader(ID3D11Device* device) {
    if(shaderDevice.Get()!=device) { shader.Reset(); constants.Reset(); shaderDevice=device; }
    if(shader && constants) return true;
    ComPtr<ID3DBlob> code, errors;
    if(FAILED(D3DCompile(source,std::strlen(source),"silhouette",nullptr,nullptr,"main","ps_4_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&errors))) return false;
    if(FAILED(device->CreatePixelShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&shader))) return false;
    D3D11_BUFFER_DESC desc={}; desc.ByteWidth=sizeof(Parameters); desc.Usage=D3D11_USAGE_DEFAULT; desc.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
    return SUCCEEDED(device->CreateBuffer(&desc,nullptr,&constants));
}
inline void Begin(const ImDrawList*,const ImDrawCmd*) {
    frameContext->PSGetConstantBuffers(0,1,previousConstants.ReleaseAndGetAddressOf());
    auto* buffer=constants.Get(); frameContext->PSSetConstantBuffers(0,1,&buffer);
    frameContext->PSSetShader(shader.Get(),nullptr,0);
}
inline void End(const ImDrawList*,const ImDrawCmd*) {
    auto* buffer=previousConstants.Get(); frameContext->PSSetConstantBuffers(0,1,&buffer); previousConstants.Reset();
}
inline void Draw(ID3D11Device* device,ID3D11DeviceContext* context,bool enabled,const float* front,const float* behind,float width) {
    frameMask.Reset();
    if(!enabled || !TryAcquireSRWLockShared(&maskLock)) return;
    frameMask=published; ReleaseSRWLockShared(&maskLock);
    if(!frameMask || !EnsureShader(device)) return;
    ComPtr<ID3D11Device> owner; frameMask->GetDevice(&owner);
    if(owner.Get()!=device) return;
    Parameters values={}; std::copy(front,front+4,values.front); std::copy(behind,behind+4,values.behind);
    values.width=width>=1 && width<=6 ? width : 2;
    context->UpdateSubresource(constants.Get(),0,nullptr,&values,0,0); frameContext=context;
    auto* list=ImGui::GetBackgroundDrawList();
    list->AddCallback(Begin,nullptr);
    list->AddImage((ImTextureID)frameMask.Get(),ImVec2(0,0),ImGui::GetIO().DisplaySize);
    list->AddCallback(End,nullptr);
    list->AddCallback(ImDrawCallback_ResetRenderState,nullptr);
}
}

extern "C" __declspec(dllexport) int __cdecl Zb2PublishOutlineMask(ID3D11Resource* resource) {
    Silhouette::ComPtr<ID3D11ShaderResourceView> view;
    if(resource) {
        Silhouette::ComPtr<ID3D11Device> device; resource->GetDevice(&device);
        if(FAILED(device->CreateShaderResourceView(resource,nullptr,&view))) return 0;
    }
    AcquireSRWLockExclusive(&Silhouette::maskLock);
    Silhouette::published=std::move(view);
    ReleaseSRWLockExclusive(&Silhouette::maskLock);
    return 1;
}
