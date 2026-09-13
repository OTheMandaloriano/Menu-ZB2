#pragma once
#include "../imgui/imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

inline bool RenderPpm(const std::string& path,int scale=2) {
    ImDrawData* data=ImGui::GetDrawData();
    int width=static_cast<int>(data->DisplaySize.x)*scale,height=static_cast<int>(data->DisplaySize.y)*scale;
    std::vector<unsigned char> pixels(static_cast<size_t>(width)*height*3,12);
    unsigned char* atlas=nullptr;int textureWidth=0,textureHeight=0;
    ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&atlas,&textureWidth,&textureHeight);
    auto edge=[](ImVec2 a,ImVec2 b,ImVec2 p){return (b.x-a.x)*(p.y-a.y)-(b.y-a.y)*(p.x-a.x);};
    auto topLeft=[](ImVec2 a,ImVec2 b){return a.y>b.y || (a.y==b.y && a.x<b.x);};
    for(int listIndex=0;listIndex<data->CmdListsCount;++listIndex) {
        const ImDrawList* list=data->CmdLists[listIndex];
        for(const auto& command : list->CmdBuffer) {
            if(command.UserCallback)continue;
            int clipLeft=std::max(0,static_cast<int>(std::ceil((command.ClipRect.x-data->DisplayPos.x)*scale)));
            int clipTop=std::max(0,static_cast<int>(std::ceil((command.ClipRect.y-data->DisplayPos.y)*scale)));
            int clipRight=std::min(width,static_cast<int>(std::floor((command.ClipRect.z-data->DisplayPos.x)*scale)));
            int clipBottom=std::min(height,static_cast<int>(std::floor((command.ClipRect.w-data->DisplayPos.y)*scale)));
            for(unsigned index=0;index+2<command.ElemCount;index+=3) {
                ImDrawVert vertex[3];
                for(int k=0;k<3;++k){vertex[k]=list->VtxBuffer[list->IdxBuffer[command.IdxOffset+index+k]+command.VtxOffset];vertex[k].pos.x=(vertex[k].pos.x-data->DisplayPos.x)*scale;vertex[k].pos.y=(vertex[k].pos.y-data->DisplayPos.y)*scale;}
                float area=edge(vertex[0].pos,vertex[1].pos,vertex[2].pos);
                if(std::fabs(area)<1e-8f)continue;
                if(area<0){std::swap(vertex[1],vertex[2]);area=-area;}
                int x0=std::max(clipLeft,static_cast<int>(std::floor(std::min({vertex[0].pos.x,vertex[1].pos.x,vertex[2].pos.x}))));
                int x1=std::min(clipRight,static_cast<int>(std::ceil(std::max({vertex[0].pos.x,vertex[1].pos.x,vertex[2].pos.x}))));
                int y0=std::max(clipTop,static_cast<int>(std::floor(std::min({vertex[0].pos.y,vertex[1].pos.y,vertex[2].pos.y}))));
                int y1=std::min(clipBottom,static_cast<int>(std::ceil(std::max({vertex[0].pos.y,vertex[1].pos.y,vertex[2].pos.y}))));
                for(int y=y0;y<y1;++y)for(int x=x0;x<x1;++x) {
                    ImVec2 point(x+0.5f,y+0.5f);
                    float weights[3]={edge(vertex[1].pos,vertex[2].pos,point),edge(vertex[2].pos,vertex[0].pos,point),edge(vertex[0].pos,vertex[1].pos,point)};
                    bool inside=true;
                    for(int k=0;k<3;++k)if(weights[k]<0 || (weights[k]==0 && !topLeft(vertex[(k+1)%3].pos,vertex[(k+2)%3].pos)))inside=false;
                    if(!inside)continue;
                    float u=0,v=0,color[4]={};
                    for(int k=0;k<3;++k){weights[k]/=area;u+=vertex[k].uv.x*weights[k];v+=vertex[k].uv.y*weights[k];for(int channel=0;channel<4;++channel)color[channel]+=((vertex[k].col>>(channel*8))&255)*weights[k];}
                    float tx=u*textureWidth-0.5f,ty=v*textureHeight-0.5f;
                    int ix=static_cast<int>(std::floor(tx)),iy=static_cast<int>(std::floor(ty));
                    float dx=tx-ix,dy=ty-iy,sample[4]={};
                    for(int sy=0;sy<2;++sy)for(int sx=0;sx<2;++sx){int px=std::clamp(ix+sx,0,textureWidth-1),py=std::clamp(iy+sy,0,textureHeight-1);float weight=(sx?dx:1-dx)*(sy?dy:1-dy);for(int channel=0;channel<4;++channel)sample[channel]+=atlas[(py*textureWidth+px)*4+channel]*weight;}
                    float alpha=color[3]*sample[3]/65025.0f;
                    size_t offset=(static_cast<size_t>(y)*width+x)*3;
                    for(int channel=0;channel<3;++channel)pixels[offset+channel]=static_cast<unsigned char>(std::clamp(color[channel]*sample[channel]/255.0f*alpha+pixels[offset+channel]*(1-alpha),0.0f,255.0f));
                }
            }
        }
    }
    FILE* file=std::fopen(path.c_str(),"wb");if(!file)return false;
    std::fprintf(file,"P6\n%d %d\n255\n",width,height);
    bool ok=std::fwrite(pixels.data(),1,pixels.size(),file)==pixels.size();
    return std::fclose(file)==0 && ok;
}
