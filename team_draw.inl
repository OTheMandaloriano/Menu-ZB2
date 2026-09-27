static void DrawTeamMarker(ImDrawList* draw,const Mono::WorldMarker& marker,const ImGuiIO& io) {
    if(marker.distance>Config::fAllyDistance)return;
    const auto tint=[](const float* c){return ImGui::GetColorU32(ImVec4(c[0],c[1],c[2],c[3]));};
    ImVec2 a(marker.left*io.DisplaySize.x,marker.top*io.DisplaySize.y),b(marker.right*io.DisplaySize.x,marker.bottom*io.DisplaySize.y);
    auto color=tint(Config::colAllyVis);
    if(Config::bAllyBoxShow){
        if(Config::iAllyBox==1){static const int edges[][2]={{0,1},{1,3},{3,2},{2,0},{4,5},{5,7},{7,6},{6,4},{0,4},{1,5},{2,6},{3,7}};
            for(auto& edge:edges){int l=edge[0]*3,r=edge[1]*3;if(marker.corners[l+2] && marker.corners[r+2])draw->AddLine(ImVec2(marker.corners[l]*io.DisplaySize.x,marker.corners[l+1]*io.DisplaySize.y),ImVec2(marker.corners[r]*io.DisplaySize.x,marker.corners[r+1]*io.DisplaySize.y),color,1.5f);}
        }else if(Config::iAllyBox==2){float w=(b.x-a.x)*.25f,h=(b.y-a.y)*.2f;for(int side=0;side<4;++side){bool right=(side&1)!=0,bottom=(side&2)!=0;ImVec2 p(right?b.x:a.x,bottom?b.y:a.y);draw->AddLine(p,ImVec2(p.x+(right?-w:w),p.y),color,1.5f);draw->AddLine(p,ImVec2(p.x,p.y+(bottom?-h:h)),color,1.5f);}}
        else draw->AddRect(a,b,color,0,0,1.5f);
    }
    if(Config::bAllySnap)draw->AddLine(ImVec2(io.DisplaySize.x*.5f,EspOptions::SnapOriginY(Config::iAllySnapFrom,io.DisplaySize.y)),ImVec2((a.x+b.x)*.5f,b.y),tint(Config::colAllyLine));
    if(Config::bAllySkeleton){static const int edges[][2]={{0,1},{1,2},{2,3},{3,4},{2,5},{5,6},{6,7},{2,8},{8,9},{9,10},{4,11},{11,12},{12,13},{4,14},{14,15},{15,16}};
        for(auto& edge:edges){int l=edge[0]*3,r=edge[1]*3;if(marker.bones[l+2] && marker.bones[r+2])draw->AddLine(ImVec2(marker.bones[l]*io.DisplaySize.x,marker.bones[l+1]*io.DisplaySize.y),ImVec2(marker.bones[r]*io.DisplaySize.x,marker.bones[r+1]*io.DisplaySize.y),tint(Config::colAllySkel),1.5f);}
    }
    if(Config::bAllyHeadDot && marker.bones[2])draw->AddCircleFilled(ImVec2(marker.bones[0]*io.DisplaySize.x,marker.bones[1]*io.DisplaySize.y),3,tint(Config::colAllyDot));
    auto model=ReadTeamLayout();
    model.items[EspLayout::Name].enabled=Config::bAllyName;model.items[EspLayout::Distance].enabled=Config::bAllyDist;
    model.items[EspLayout::Health].enabled=Config::bAllyHp;model.items[EspLayout::Percent].enabled=Config::bAllyPct;
    auto content=LayoutContent(marker.name,marker.distance,marker.health,marker.maxHealth);auto style=LayoutStyle(content.health);
    style.color[EspLayout::Name]=tint(Config::colAllyName);style.color[EspLayout::Distance]=tint(Config::colAllyDist);style.color[EspLayout::Percent]=tint(Config::colAllyHp);
    EspLayout::Rect envelope={a,b},viewport={ImVec2(0,0),io.DisplaySize};auto geometry=EspLayout::Resolve(model,envelope,viewport,content,style);EspLayout::Draw(draw,geometry,content,style);
}
