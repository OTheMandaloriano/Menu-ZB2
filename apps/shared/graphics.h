#pragma once
#include <d3d11.h>
#include <wrl/client.h>
struct ImDrawData;

class AppGraphics {
public:
    ~AppGraphics(){Shutdown();}
    bool Initialize(HWND window);
    bool Resize(UINT width,UINT height);
    bool Present(ImDrawData* data);
    void Shutdown();
    ID3D11Device* Device() const{return device.Get();}
    ID3D11DeviceContext* Context() const{return context.Get();}
private:
    bool CreateTarget();
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swap;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> target;
};
