static ID3D11Device* s_iconDevice=nullptr;
static ID3D11ShaderResourceView* s_itemIcons[128]={};
static bool s_iconDone[128]={};
static void ClearItemIcons(){for(auto*& icon:s_itemIcons){if(icon)icon->Release();icon=nullptr;}memset(s_iconDone,0,sizeof(s_iconDone));if(s_iconDevice)s_iconDevice->Release();s_iconDevice=nullptr;}
static void UpdateItemIcon() {
    Mono::IconPixels pixels;
    if(!s_iconDevice || !Mono::GetIcon(pixels) || pixels.id<0 || pixels.id>=128 || s_iconDone[pixels.id])return;
    s_iconDone[pixels.id]=true;if(!pixels.valid)return;
    D3D11_TEXTURE2D_DESC desc={};desc.Width=desc.Height=48;desc.MipLevels=desc.ArraySize=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA data={};data.pSysMem=pixels.rgba;data.SysMemPitch=48*4;
    ID3D11Texture2D* texture=nullptr;
    if(SUCCEEDED(s_iconDevice->CreateTexture2D(&desc,&data,&texture))){s_iconDevice->CreateShaderResourceView(texture,nullptr,&s_itemIcons[pixels.id]);texture->Release();}
}
