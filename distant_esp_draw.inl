static void DrawDistantEsp(ImDrawList* draw, const ImGuiIO& io) {
    if(!Config::bZombieEsp || !(Config::bZombieBoxShow || Config::bZombieName || Config::bZombieDist || Config::bZombieSnap)) return;
    static Mono::DistantMarker markers[256];
    int count=Mono::GetDistantEsp(markers,256);
    auto color=[](const float* c){return ImGui::GetColorU32(ImVec4(c[0],c[1],c[2],c[3]));};
    for(int i=0;i<count;++i) {
        const auto& item=markers[i];
        if(!std::isfinite(item.distance) || item.distance<0 || item.distance>Config::fEspDistance)continue;
        if(!(item.x>=0 && item.x<=1 && item.y>=0 && item.y<=1))continue;
        ImVec2 point(item.x*io.DisplaySize.x,item.y*io.DisplaySize.y);
        ImU32 boxColor=color(item.type>=6 && Config::bBossColor ? Config::colBossBox : Config::colZombieBox);
        // A position marker deliberately makes no claim about unloaded body bounds.
        if(Config::bZombieBoxShow) {
            const ImVec2 points[]={{point.x,point.y-5},{point.x+5,point.y},{point.x,point.y+5},{point.x-5,point.y}};
            draw->AddPolyline(points,4,boxColor,ImDrawFlags_Closed,1.5f);
        }
        if(Config::bZombieSnap)
            draw->AddLine(ImVec2(io.DisplaySize.x*.5f,EspOptions::SnapOriginY(Config::iSnapFrom,io.DisplaySize.y)),point,color(Config::colZombieSnap));
        if(Config::bZombieName) {
            const char* name=item.type==6 ? "Assalto (distante)" : item.type==7 ? "Rainha (distante)" :
                item.type==8 ? "Ceifador (distante)" : "Zumbi (distante)";
            draw->AddText(ImVec2(point.x+7,point.y-14),color(Config::colZombieName),name);
        }
        if(Config::bZombieDist) {
            char text[32];_snprintf_s(text,_TRUNCATE,"%.0fm",item.distance);
            draw->AddText(ImVec2(point.x+7,point.y),color(Config::colZombieDist),text);
        }
    }
}
