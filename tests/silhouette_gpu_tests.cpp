#include "../silhouette.h"
#include <stdexcept>
#include <iostream>
#include <vector>
#pragma comment(lib,"d3d11.lib")
using Silhouette::ComPtr;
static void Check(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
int main() {
    ComPtr<ID3D11Device> device; ComPtr<ID3D11DeviceContext> context;
    Check(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)),"WARP device");
    Check(Silhouette::EnsureShader(device.Get()),"compile real outline pixel shader");
    constexpr unsigned size=16;
    std::vector<unsigned> pixels(size*size);
    for(unsigned y=5;y<10;++y) for(unsigned x=3;x<6;++x) pixels[y*size+x]=0xff0000ff;
    for(unsigned y=5;y<10;++y) for(unsigned x=10;x<13;++x) pixels[y*size+x]=0xff00ff00;
    D3D11_TEXTURE2D_DESC desc={}; desc.Width=desc.Height=size; desc.MipLevels=desc.ArraySize=1;
    desc.Format=DXGI_FORMAT_R8G8B8A8_TYPELESS; desc.SampleDesc.Count=1; desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA initial={}; initial.pSysMem=pixels.data(); initial.SysMemPitch=size*4;
    ComPtr<ID3D11Texture2D> mask,target,readback;
    Check(SUCCEEDED(device->CreateTexture2D(&desc,&initial,&mask)),"create input mask");
    Check(Zb2PublishOutlineMask(mask.Get())==1,"publish mask");
    Check(Silhouette::published.Get()!=nullptr,"retained GPU view");
    desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BindFlags=D3D11_BIND_RENDER_TARGET;
    Check(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&target)),"create output");
    desc.BindFlags=0; desc.Usage=D3D11_USAGE_STAGING; desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    Check(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&readback)),"create readback");
    ComPtr<ID3D11RenderTargetView> rtv;
    Check(SUCCEEDED(device->CreateRenderTargetView(target.Get(),nullptr,&rtv)),"output view");
    const char* vsSource=R"(
struct O { float4 p:SV_POSITION; float4 c:COLOR0; float2 uv:TEXCOORD0; };
O main(uint id:SV_VertexID) { O o; o.uv=float2((id<<1)&2,id&2); o.p=float4(o.uv*float2(2,-2)+float2(-1,1),0,1); o.c=1; return o; }
)";
    ComPtr<ID3DBlob> bytecode,error; ComPtr<ID3D11VertexShader> vs;
    Check(SUCCEEDED(D3DCompile(vsSource,std::strlen(vsSource),"test",nullptr,nullptr,"main","vs_4_0",0,0,&bytecode,&error)),"test vertex shader");
    Check(SUCCEEDED(device->CreateVertexShader(bytecode->GetBufferPointer(),bytecode->GetBufferSize(),nullptr,&vs)),"create vertex shader");
    auto* output=rtv.Get(); context->OMSetRenderTargets(1,&output,nullptr);
    D3D11_VIEWPORT viewport={0,0,float(size),float(size),0,1}; context->RSSetViewports(1,&viewport);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vs.Get(),nullptr,0); context->PSSetShader(Silhouette::shader.Get(),nullptr,0);
    auto* srv=Silhouette::published.Get(); context->PSSetShaderResources(0,1,&srv);
    Silhouette::Parameters parameters={{1,0,0,1},{0,1,0,1},1,{0,0,0}};
    context->UpdateSubresource(Silhouette::constants.Get(),0,nullptr,&parameters,0,0);
    auto* cb=Silhouette::constants.Get(); context->PSSetConstantBuffers(0,1,&cb);
    context->Draw(3,0); context->CopyResource(readback.Get(),target.Get());
    D3D11_MAPPED_SUBRESOURCE mapped={};
    Check(SUCCEEDED(context->Map(readback.Get(),0,D3D11_MAP_READ,0,&mapped)),"read GPU result");
    auto pixel=[&](int x,int y) { return reinterpret_cast<unsigned*>(static_cast<unsigned char*>(mapped.pData)+y*mapped.RowPitch)[x]; };
    Check(pixel(3,6)==0 && pixel(11,6)==0,"body interior stays transparent");
    Check(pixel(2,6)==0xff0000ff,"visible contour has front color");
    Check(pixel(9,6)==0xff00ff00,"occluded contour has hidden color");
    Check(pixel(0,0)==0 && pixel(7,6)==0,"outside contour transparent");
    context->Unmap(readback.Get(),0);
    Check(Zb2PublishOutlineMask(nullptr)==1 && !Silhouette::published,"disable releases published view");
    std::cout << "Silhouette WARP GPU checks passed: real shader, visible/hidden edges, transparent interior, resource release\n";
}
