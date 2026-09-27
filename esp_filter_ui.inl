static void FilterCheckbox(const char* label,int& word,int bit) {
    unsigned value=static_cast<unsigned>(word),mask=1u<<bit;
    bool selected=(value&mask)!=0;
    if(ImGui::Checkbox(label,&selected))word=static_cast<int>(selected ? value|mask : value&~mask);
}
static void DrawItemFilters() {
    if(!ImGui::TreeNode("Personalizar por item"))return;
    int* masks[]={&Config::iItemFilter0,&Config::iItemFilter1,&Config::iItemFilter2,&Config::iItemFilter3};
    if(ImGui::Button("Todos##items"))for(auto p:masks)*p=-1;
    ImGui::SameLine();if(ImGui::Button("Nenhum##items"))for(auto p:masks)*p=0;
    static ImGuiTextFilter search;search.Draw("Buscar item");
    ImGui::BeginChild("ItemNames",ImVec2(0,180),true);
    for(const auto& item:ItemCatalog::entries)if(item.id>0 && search.PassFilter(item.name))
        FilterCheckbox(item.name,*masks[item.id/32],item.id%32);
    ImGui::EndChild();ImGui::TreePop();
}
static void DrawPointFilters() {
    if(!ImGui::TreeNode("Personalizar por ponto"))return;
    static const char* names[]={"Chefao","Outro jogador","Posicao local (ignorada)","Sepultura","Municao","Armas","Corpo a corpo",
        "Bancada recarga","Cura","Materiais","Comida","Ponto desconhecido","Casa inicial","Fogueira","Mercador","Bancada melhoria","Bomba","Helicoptero"};
    if(ImGui::Button("Todos##points"))Config::iPoiFilter=-1;
    ImGui::SameLine();if(ImGui::Button("Nenhum##points"))Config::iPoiFilter=0;
    ImGui::BeginChild("PointNames",ImVec2(0,160),true);
    for(int i=0;i<18;++i)if(i!=2)FilterCheckbox(names[i],Config::iPoiFilter,i);
    ImGui::EndChild();ImGui::TreePop();
}
