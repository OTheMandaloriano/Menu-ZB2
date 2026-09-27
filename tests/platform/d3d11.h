#pragma once
#include "Windows.h"
struct ID3D11Texture2D {void Release(){}};
struct ID3D11ShaderResourceView {void Release(){}};
struct D3D11_TEXTURE2D_DESC {unsigned Width=0,Height=0,MipLevels=0,ArraySize=0,Format=0,Usage=0,BindFlags=0;struct {unsigned Count=0,Quality=0;} SampleDesc;};
struct D3D11_SUBRESOURCE_DATA {const void* pSysMem=nullptr;unsigned SysMemPitch=0,SysMemSlicePitch=0;};
constexpr unsigned DXGI_FORMAT_R8G8B8A8_UNORM=28,D3D11_USAGE_IMMUTABLE=1,D3D11_BIND_SHADER_RESOURCE=8;
#ifndef SUCCEEDED
#define SUCCEEDED(value) ((value)>=0)
#endif
struct ID3D11Device {void AddRef(){}void Release(){}int CreateTexture2D(const D3D11_TEXTURE2D_DESC*,const D3D11_SUBRESOURCE_DATA*,ID3D11Texture2D**){return -1;}int CreateShaderResourceView(ID3D11Texture2D*,void*,ID3D11ShaderResourceView**){return -1;}};
struct ID3D11DeviceContext {};
