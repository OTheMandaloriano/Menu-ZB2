static EspLayout::Model ReadTeamLayout() {
    auto model=EspLayout::Preset(Config::iAllyLayout);
    if(Config::szAllyLayout[0]){
        std::istringstream input(Config::szAllyLayout);input.imbue(std::locale::classic());int version=0;
        auto candidate=model;input>>version;
        for(auto& p:candidate.items)input>>p.side>>p.position>>p.extraGap>>p.order;
        input>>candidate.gap>>candidate.spacing>>candidate.barLength>>candidate.barThickness;
        if(input && version==1)model=candidate;
    }
    model.items[EspLayout::Name].enabled=Config::bAllyName;model.items[EspLayout::Distance].enabled=Config::bAllyDist;
    model.items[EspLayout::Health].enabled=Config::bAllyHp;model.items[EspLayout::Percent].enabled=Config::bAllyPct;
    model.skeleton=Config::bAllySkeleton;model.headDot=Config::bAllyHeadDot;model.snapline=Config::bAllySnap;model.boxStyle=Config::iAllyBox;
    EspLayout::Normalize(model);return model;
}
static void SaveTeamLayout(const EspLayout::Model& model) {
    std::ostringstream output;output.imbue(std::locale::classic());output<<1;
    for(const auto& p:model.items)output<<' '<<p.side<<' '<<p.position<<' '<<p.extraGap<<' '<<p.order;
    output<<' '<<model.gap<<' '<<model.spacing<<' '<<model.barLength<<' '<<model.barThickness;
    strncpy_s(Config::szAllyLayout,output.str().c_str(),_TRUNCATE);
    Config::iAllyLayout=0;
}
static void DrawTeamLayoutEditor() {
    static EspLayout::Editor editor;
    if(ImGui::Button("Posicionar elementos da equipe...")){EspLayout::Reset(editor,ReadTeamLayout());ImGui::OpenPopup("Layout da equipe");}
    ImGui::SetNextWindowSize(ImVec2(580,440),ImGuiCond_FirstUseEver);
    if(!ImGui::BeginPopupModal("Layout da equipe",nullptr,ImGuiWindowFlags_NoCollapse))return;
    ImGui::TextWrapped("Arraste nome, distancia, barra e percentual para as bordas do personagem. As configuracoes de zumbis nao sao alteradas.");
    ImVec2 origin=ImGui::GetCursorScreenPos(),size=ImVec2(ImGui::GetContentRegionAvail().x,270);
    ImGui::InvisibleButton("##team-canvas",size);
    EspLayout::Rect viewport={origin,ImVec2(origin.x+size.x,origin.y+size.y)};
    ImVec2 center(origin.x+size.x*.5f,origin.y+size.y*.5f);
    EspLayout::Rect box={ImVec2(center.x-34,center.y-70),ImVec2(center.x+34,center.y+70)};
    auto content=LayoutContent("Aliado",24,87,100);auto style=LayoutStyle(content.health);
    auto color=[](const float* c){return ImGui::GetColorU32(ImVec4(c[0],c[1],c[2],c[3]));};
    style.color[EspLayout::Name]=color(Config::colAllyName);style.color[EspLayout::Distance]=color(Config::colAllyDist);style.color[EspLayout::Percent]=color(Config::colAllyHp);
    style.boxColor=color(Config::colAllyVis);style.skelColor=color(Config::colAllySkel);style.snapColor=color(Config::colAllyLine);style.dotColor=color(Config::colAllyDot);
    EspLayout::Input input;input.mouse=ImGui::GetIO().MousePos;input.pressed=ImGui::IsMouseClicked(0);input.down=ImGui::IsMouseDown(0);input.released=ImGui::IsMouseReleased(0);input.hovered=ImGui::IsItemHovered();input.cancel=ImGui::IsKeyPressed(ImGuiKey_Escape);
    auto result=EspLayout::Update(editor,box,viewport,content,style,input);
    auto model=editor.active>=0 && editor.moved && editor.candidateValid?editor.candidate:editor.draft;
    EspLayout::DrawPreview(ImGui::GetWindowDrawList(),viewport,result,model,style,editor.selected,editor.active>=0,editor.candidateValid);
    EspLayout::Draw(ImGui::GetWindowDrawList(),result,content,style);
    if(editor.dirty && editor.active<0){SaveTeamLayout(editor.draft);editor.dirty=false;}
    if(ImGui::Button("Restaurar layout")){Config::szAllyLayout[0]=0;EspLayout::Reset(editor,ReadTeamLayout());}
    ImGui::SameLine();if(ImGui::Button("Fechar"))ImGui::CloseCurrentPopup();ImGui::EndPopup();
}
