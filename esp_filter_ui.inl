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
    if(loot)ImGui::TextWrapped("1. Escolha os itens. 2. Feche esta janela. 3. Ative Item Magnet uma vez. Os itens serao trazidos ao chao, sem entrar na mochila.");
    if(ImGui::Button(loot?"Todos permitidos":"Todos")){for(auto p:masks)*p=-1;if(loot)Config::iItemMagnetType=3;}
    ImGui::SameLine();if(ImGui::Button("Limpar selecao"))for(auto p:masks)*p=0;
    ImGui::SameLine();if(ImGui::Button("Limpar busca"))search[0]=0;
    Mono::CatalogEntry catalog[128];int count=Mono::GetCatalog(catalog,128);
    UpdateItemIcon();int shown=0,selected=0;
    for(int i=0;i<count;++i)if(catalog[i].id>0 && catalog[i].id<128 && (static_cast<unsigned>(*masks[catalog[i].id/32])&(1u<<(catalog[i].id%32))))++selected;
    ImGui::Text("%d de %d selecionados",selected,count);
    if(count==0)ImGui::TextDisabled("Aguardando catalogo do jogo...");
    ImGui::BeginChild("cards",ImVec2(0,-45),true);
    if(ImGui::BeginTable("items",3,ImGuiTableFlags_SizingStretchSame)) {
        for(int i=0;i<count;++i){const auto& item=catalog[i];if(item.id<=0 || item.id>=128)continue;
            const char* alias="";for(const auto& known:ItemCatalog::entries)if(known.id==item.id){alias=known.name;break;}
            if(!ItemSearch::Matches(item.name,alias,search))continue;
            ++shown;ImGui::TableNextColumn();ImGui::PushID(item.id);
            const auto position=ImGui::GetCursorScreenPos();
            if(s_itemIcons[item.id]){ImGui::Image(s_itemIcons[item.id],ImVec2(48,48));if(ImGui::IsItemClicked()){unsigned mask=1u<<(item.id%32);*masks[item.id/32]=static_cast<int>(static_cast<unsigned>(*masks[item.id/32])^mask);}}
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

static void DrawInventoryGrant() {
    ImGui::Separator();ImGui::TextUnformatted("Adicionar item (solo/host)");
    if(ImGui::Button("Escolher item para adicionar..."))ImGui::OpenPopup("Adicionar ao inventario");
    ImGui::SetNextWindowSize(ImVec2(650,460),ImGuiCond_FirstUseEver);
    if(!ImGui::BeginPopupModal("Adicionar ao inventario",nullptr,ImGuiWindowFlags_NoCollapse))return;
    static char search[128]={};ImGui::InputTextWithHint("##grant-search","Buscar nome ou ID...",search,128);
    Mono::CatalogEntry catalog[128];int count=Mono::GetCatalog(catalog,128);UpdateItemIcon();
    ImGui::BeginChild("grant-items",ImVec2(0,-100),true);
    for(int i=0;i<count;++i){const auto& item=catalog[i];if(item.id<=0 || item.id>=128)continue;
        char id[20];_snprintf_s(id,_TRUNCATE,"%d",item.id);
        if(!ItemSearch::Matches(item.name,id,search))continue;
        ImGui::PushID(item.id);
        if(s_itemIcons[item.id]){ImGui::Image(s_itemIcons[item.id],ImVec2(32,32));ImGui::SameLine();}
        else if(!s_iconDone[item.id])Mono::RequestIcon(item.id);
        if(ImGui::Selectable(item.name,Config::iGrantItem==item.id))Config::iGrantItem=item.id;
        ImGui::PopID();
    }
    ImGui::EndChild();
    ImGui::SliderInt("Quantidade por pilha",&Config::iItemAmount,1,999);
    ImGui::TextWrapped("Respeita o limite da pilha e o espaco do inventario. Armas: uma unidade por clique. Itens bloqueados nao sao permitidos.");
    ImGui::BeginDisabled(Config::iGrantItem<=0 || !Mono::Get().inMap || Mono::Get().coopMode==2);
    if(ImGui::Button("Adicionar selecionado"))Config::iGrantRequest=Config::iGrantRequest>=2147483646?1:Config::iGrantRequest+1;
    ImGui::EndDisabled();ImGui::SameLine();if(ImGui::Button("Fechar"))ImGui::CloseCurrentPopup();
    if(Mono::Get().extrasStatus[0])ImGui::TextWrapped("%s",Mono::Get().extrasStatus);
    ImGui::EndPopup();
}
