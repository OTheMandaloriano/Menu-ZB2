#include "cover.h"
#include "resources.h"
#include <Windows.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <vector>
namespace Cover {
using Microsoft::WRL::ComPtr;
namespace {int rectangle=-1;bool ready=false;constexpr UINT Width=600,Height=400;}
void Prepare(){
    ready=false;rectangle=-1;
    if(!FindResourceW(nullptr,MAKEINTRESOURCEW(208),RT_RCDATA))return;
    const HRESULT com=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    struct ComScope {bool release;~ComScope(){if(release)CoUninitialize();}} scope{SUCCEEDED(com)};
    try{
        auto data=AppResources::Read(208);ComPtr<IWICImagingFactory> factory;ComPtr<IWICStream> stream;ComPtr<IWICBitmapDecoder> decoder;ComPtr<IWICBitmapFrameDecode> frame;ComPtr<IWICBitmapScaler> scaler;ComPtr<IWICFormatConverter> converter;
        HRESULT result=CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory));
        if(SUCCEEDED(result))result=factory->CreateStream(&stream);
        if(SUCCEEDED(result))result=stream->InitializeFromMemory(reinterpret_cast<BYTE*>(data.data()),static_cast<DWORD>(data.size()));
        if(SUCCEEDED(result))result=factory->CreateDecoderFromStream(stream.Get(),nullptr,WICDecodeMetadataCacheOnLoad,&decoder);
        if(SUCCEEDED(result))result=decoder->GetFrame(0,&frame);
        if(SUCCEEDED(result))result=factory->CreateBitmapScaler(&scaler);
        if(SUCCEEDED(result))result=scaler->Initialize(frame.Get(),Width,Height,WICBitmapInterpolationModeFant);
        if(SUCCEEDED(result))result=factory->CreateFormatConverter(&converter);
        if(SUCCEEDED(result))result=converter->Initialize(scaler.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom);
        std::vector<unsigned char> pixels(Width*Height*4);
        if(SUCCEEDED(result))result=converter->CopyPixels(nullptr,Width*4,static_cast<UINT>(pixels.size()),pixels.data());
        if(SUCCEEDED(result)){
            auto& atlas=*ImGui::GetIO().Fonts;atlas.TexDesiredWidth=1024;rectangle=atlas.AddCustomRectRegular(Width,Height);
            if(!atlas.Build())return;
            unsigned char* target=nullptr;int w=0,h=0;atlas.GetTexDataAsRGBA32(&target,&w,&h);auto* rect=atlas.GetCustomRectByIndex(rectangle);
            if(!target||!rect->IsPacked()||rect->X+Width>static_cast<UINT>(w)||rect->Y+Height>static_cast<UINT>(h))return;
            for(UINT row=0;row<Height;++row)memcpy(target+((static_cast<size_t>(rect->Y)+row)*w+rect->X)*4,pixels.data()+row*Width*4,Width*4);
            ready=true;
        }
    }catch(...){ready=false;}
}
bool Available(){return ready;}
void Draw(float x,float y,float width,float height){
    if(!ready)return;
    auto& atlas=*ImGui::GetIO().Fonts;ImVec2 uv0,uv1;atlas.CalcCustomRectUV(atlas.GetCustomRectByIndex(rectangle),&uv0,&uv1);
    float ratio=width/height,original=static_cast<float>(Width)/Height;
    if(ratio>original){float crop=(1-original/ratio)*.5f*(uv1.y-uv0.y);uv0.y+=crop;uv1.y-=crop;}
    else{float crop=(1-ratio/original)*.5f*(uv1.x-uv0.x);uv0.x+=crop;uv1.x-=crop;}
    auto scale=UiTheme::uiScale;auto alpha=static_cast<int>(ImGui::GetStyle().Alpha*255);ImGui::GetWindowDrawList()->AddImageRounded(atlas.TexID,{x*scale,y*scale},{(x+width)*scale,(y+height)*scale},uv0,uv1,IM_COL32(255,255,255,alpha),8*scale);
}
}
