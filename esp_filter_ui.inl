static void FilterCheckbox(const char* label,int& word,int bit) {
    unsigned value=static_cast<unsigned>(word),mask=1u<<bit;
    bool selected=(value&mask)!=0;
    if(ImGui::Checkbox(label,&selected))word=static_cast<int>(selected ? value|mask : value&~mask);
}
static void DrawItemFilters(bool loot=false) {
    const char* title=loot?"Itens para puxar":"Catalogo de itens";
    if(ImGui::Button(loot?"Escolher itens para puxar...":"Escolher itens..."))ImGui::OpenPopup(title);
    ImGui::SetNextWindowSize(ImVec2(760,540),ImGuiCond_FirstUseEver);
    if(!ImGui::BeginPopupModal(title,nullptr,ImGuiWindowFlags_NoCollapse))return;
    int* masks[]={&Config::iItemFilter0,&Config::iItemFilter1,&Config::iItemFilter2,&Config::iItemFilter3};
    if(loot){masks[0]=&Config::iLootFilter0;masks[1]=&Config::iLootFilter1;masks[2]=&Config::iLootFilter2;masks[3]=&Config::iLootFilter3;}
    static char searches[2][128]={};char* search=searches[loot?1:0];ImGui::SetNextItemWidth(-1);ImGui::InputTextWithHint("##item-search","Buscar nome ou ID interno...",search,128);
    if(ImGui::Button("Todos"))for(auto p:masks)*p=-1;
    ImGui::SameLine();if(ImGui::Button("Nenhum"))for(auto p:masks)*p=0;
    ImGui::SameLine();if(ImGui::Button("Limpar busca"))search[0]=0;
    Mono::CatalogEntry catalog[128];int count=Mono::GetCatalog(catalog,128);
    UpdateItemIcon();int shown=0;
    if(count==0)ImGui::TextDisabled("Aguardando catalogo do jogo...");
    ImGui::BeginChild("cards",ImVec2(0,-45),true);
    if(ImGui::BeginTable("items",3,ImGuiTableFlags_SizingStretchSame)) {
        for(int i=0;i<count;++i){const auto& item=catalog[i];if(item.id<=0 || item.id>=128)continue;
            const char* alias="";for(const auto& known:ItemCatalog::entries)if(known.id==item.id){alias=known.name;break;}
            if(!ItemSearch::Matches(item.name,alias,search))continue;
            ++shown;ImGui::TableNextColumn();ImGui::PushID(item.id);
            const auto position=ImGui::GetCursorScreenPos();
            if(s_itemIcons[item.id])ImGui::Image(s_itemIcons[item.id],ImVec2(48,48));
            else {ImGui::Dummy(ImVec2(48,48));ImGui::GetWindowDrawList()->AddRect(position,ImVec2(position.x+48,position.y+48),IM_COL32(90,100,110,180));if(!s_iconDone[item.id])Mono::RequestIcon(item.id);}
            ImGui::SameLine();ImGui::BeginGroup();FilterCheckbox("##enabled",*masks[item.id/32],item.id%32);ImGui::TextWrapped("%s",item.name);ImGui::EndGroup();
            if(ImGui::IsItemHovered())ImGui::SetTooltip("%s | ID %d",alias,item.id);
            ImGui::Separator();ImGui::PopID();
        }
        ImGui::EndTable();
    }
    if(count>0 && shown==0)ImGui::TextDisabled("Nenhum item corresponde a busca.");
    ImGui::EndChild();ImGui::Text("%d itens",shown);ImGui::SameLine();if(ImGui::Button("Fechar"))ImGui::CloseCurrentPopup();ImGui::EndPopup();
}
static void DrawPointFilters() {
    if(!ImGui::TreeNode("Personalizar por ponto"))return;
    static const char* names[]={"Chefao","Equipe (separada)","Local","Sepultura","Municao","Armas","Corpo a corpo",
        "Bancada recarga","Cura","Materiais","Comida","Ponto desconhecido","Casa inicial","Fogueira","Mercador","Bancada melhoria","Bomba","Helicoptero"};
    if(ImGui::Button("Todos##points"))Config::iPoiFilter=-1;
    ImGui::SameLine();if(ImGui::Button("Nenhum##points"))Config::iPoiFilter=0;
    ImGui::BeginChild("PointNames",ImVec2(0,160),true);
    for(int i=0;i<18;++i)if(i!=1 && i!=2)FilterCheckbox(names[i],Config::iPoiFilter,i);
    ImGui::EndChild();ImGui::TreePop();
}
