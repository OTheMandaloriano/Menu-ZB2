#include "ui.h"
#include "../../shared/widgets.h"
#include "../../loader/assets/IconsFontAwesome6.h"
#include "../../loader/license.h"
#include <algorithm>
#include <Windows.h>
#include <charconv>
#include <cctype>
namespace Admin {
namespace {
using namespace UiTheme;
void TableHeader(std::initializer_list<const char*> names){
    ImGui::TableNextRow(ImGuiTableRowFlags_Headers,32*uiScale);
    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,IM_COL32(22,25,34,255));
    int column=0;for(auto name:names){ImGui::TableNextColumn();auto at=ImGui::GetCursorScreenPos();
        if(column++==0)at.x+=14*uiScale;
        ImGui::GetWindowDrawList()->AddText(caption,caption->FontSize,at,muted,name);
    }
}
bool Matches(const LicenseRow& row,const char* filter){
    std::string haystack=row.customer+" "+row.device+" "+row.issuer,query=filter;
    for(auto& c:haystack)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    for(auto& c:query)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return haystack.find(query)!=std::string::npos;
}
int Number(const char* input){int value=0;auto n=strlen(input);auto result=std::from_chars(input,input+n,value);return result.ec==std::errc()&&result.ptr==input+n?value:0;}
void RequestAction(UiState& state,const char* command,std::vector<std::string> args={},bool save=false){state.requestPage=state.page;state.pendingFeedback=true;state.exportText.clear();state.exportName.clear();state.action={command,std::move(args)};state.send=true;state.saveAfterReply=save;state.message.clear();}
void Label(float x,float y,const char* value){Ui::Text(x,y,330,20,value,muted,regular);}
void Switch(UiState& state,Page page){if(state.page!=page){state.page=page;SecureZeroMemory(state.recoveryPassword,sizeof(state.recoveryPassword));SecureZeroMemory(state.recoveryConfirm,sizeof(state.recoveryConfirm));state.recoveryAcknowledged=false;state.message.clear();state.exportText.clear();state.exportName.clear();state.credits=false;state.fade=Ui::ReducedMotion()?1.f:0.f;}}
void Copy(UiState& state,const std::string& value){state.copy=true;state.copyText=value;state.message="Copiado.";}
bool ValidDevice(const char* input){return strlen(input)==64&&std::all_of(input,input+64,[](unsigned char c){return std::isxdigit(c)!=0;});}
void LicensePage(UiState& state){
    if(state.data.maxDays<=0){
        Ui::Text(24,152,792,36,u8"Autorize esta estação para emitir",text,heading);
        Ui::Wrapped(24,212,680,u8"A emissão fica disponível depois que o proprietário autorizar este PC. Se você só vai usar o menu, utilize o pacote CLIENTE.");
        if(Ui::Button(24,300,300,40,u8"Abrir minha estação",false,true))Switch(state,Page::Station);return;
    }
    const auto matches=std::count_if(state.data.licenses.begin(),state.data.licenses.end(),[&](const LicenseRow& r){return Matches(r,state.search);});
    const float rowsHeight=matches?40.f*static_cast<float>(std::min<decltype(matches)>(matches,5)):104.f;
    const float historyHeight=168.f+rowsHeight;
    Ui::Panel(24,136,264,376);Ui::Panel(304,136,512,historyHeight);
    Ui::Text(40,148,232,28,"Cliente já enviou o ID",text,regular);
    Ui::Text(320,148,480,28,u8"Histórico de licenças",text,regular);
    Label(40,192,"Cliente");Ui::Input(40,216,232,36,"##customer","Nome ou identificador",state.customer,sizeof(state.customer),0,ICON_FA_USER);
    Label(40,260,"ID do computador");Ui::Input(40,284,232,36,"##device","Cole os 64 caracteres",state.device,sizeof(state.device),0,ICON_FA_MICROCHIP);
    Label(40,328,"Prazo de uso (dias)");
    Ui::Input(40,352,100,40,"##days","Quantidade de dias",state.days,sizeof(state.days),ImGuiInputTextFlags_CharsDecimal,ICON_FA_CALENDAR);
    const int days=Number(state.days);
    auto limit=days>0&&days<=state.data.maxDays?std::to_string(days)+u8" dias a partir da emissão":"Informe de 1 a "+std::to_string(state.data.maxDays)+" dias";
    Ui::Wrapped(152,356,120,limit.c_str(),muted);
    ImGui::BeginDisabled(!ValidDevice(state.device)||strlen(state.customer)<2||days<1||days>state.data.maxDays);
    if(Ui::Button(40,412,232,36,u8"Gerar ZIP do cliente",false,true,ICON_FA_WAND_MAGIC_SPARKLES))RequestAction(state,"issue_package",{state.customer,state.device,std::to_string(days)});
    if(Ui::Button(40,460,216,36,"Gerar somente a chave",false,false,ICON_FA_KEY))RequestAction(state,"issue",{state.customer,state.device,std::to_string(days)});ImGui::EndDisabled();
    Ui::Input(320,188,480,40,"##search","Buscar cliente ou ID",state.search,sizeof(state.search),0,ICON_FA_MAGNIFYING_GLASS);
    ImGui::SetCursorPos(Ui::P(316,244));ImGui::BeginChild("license-list",Ui::P(480,32+rowsHeight),false);
    state.visibleLicenseCount=0;
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,Ui::P(7,6));
    if(ImGui::BeginTable("licenses",4,ImGuiTableFlags_RowBg|ImGuiTableFlags_BordersInnerH|ImGuiTableFlags_SizingFixedFit)){
        ImGui::TableSetupColumn("CLIENTE",ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("VALIDADE",ImGuiTableColumnFlags_WidthFixed,150*uiScale);
        ImGui::TableSetupColumn("STATUS",ImGuiTableColumnFlags_WidthFixed,100*uiScale);
        ImGui::TableSetupColumn(u8"AÇÕES",ImGuiTableColumnFlags_WidthFixed,68*uiScale);
        TableHeader({"CLIENTE","VALIDADE","STATUS",u8"AÇÕES"});
        for(const auto& row:state.data.licenses){
            if(!Matches(row,state.search))continue;++state.visibleLicenseCount;
            ImGui::PushID(row.id.c_str());ImGui::TableNextRow(ImGuiTableRowFlags_None,40*uiScale);ImGui::TableNextColumn();
            auto at=ImGui::GetCursorScreenPos();if(ImGui::Selectable("##row",state.selectedLicense==row.id,ImGuiSelectableFlags_SpanAllColumns|ImGuiSelectableFlags_AllowItemOverlap,ImVec2(0,28*uiScale)))state.selectedLicense=row.id;
            if(ImGui::BeginPopupContextItem("license-actions")){if(ImGui::MenuItem("Salvar arquivo da licença")){state.exportText=row.token;state.exportName="Licenca-"+row.id.substr(0,8)+".zb2license";state.save=true;}ImGui::EndPopup();}
            Ui::Hint(row.customer.c_str());
            std::string name=row.customer;float available=ImGui::GetContentRegionAvail().x-14*uiScale;
            while(!name.empty()&&regular->CalcTextSizeA(regular->FontSize,10000,0,(name+"...").c_str()).x>available){size_t n=name.size()-1;while(n&&(static_cast<unsigned char>(name[n])&0xc0)==0x80)--n;name.resize(n);}if(name!=row.customer)name+="...";
            ImGui::GetWindowDrawList()->AddText(ImVec2(at.x+14*uiScale,at.y+7*uiScale),ImGui::GetColorU32(ImGuiCol_Text),name.c_str());
            ImGui::TableNextColumn();ImGui::SetCursorPosY(ImGui::GetCursorPosY()+7*uiScale);Ui::Tabular(row.expiry.c_str());
            ImGui::TableNextColumn();const bool active=row.expires?row.expires>License::Now():row.status==u8"Válida"||row.status=="Ativa";
            auto label=active?std::string("Ativo")+u8" · "+row.days+"d":std::string("Expirado");Ui::Badge(label.c_str(),active);
            ImGui::TableNextColumn();
            if(Ui::RowAction(ICON_FA_COPY))Copy(state,row.token);Ui::Hint("Copiar chave completa");
            ImGui::SameLine(0,8*uiScale);if(Ui::RowAction(ICON_FA_FILE_ZIPPER))RequestAction(state,"client_package",{row.id});Ui::Hint("Salvar ZIP deste cliente");
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::PopStyleVar();
    if(state.visibleLicenseCount==0){
        const char* title=state.search[0]?"Nenhum resultado":u8"Seu histórico começa aqui";
        const char* detail=state.search[0]?"Tente outro nome ou ID.":"As novas licenças aparecem neste espaço.";
        auto* draw=ImGui::GetWindowDrawList();auto origin=ImGui::GetWindowPos();
        float width=regular->CalcTextSizeA(regular->FontSize,10000,0,title).x;
        draw->AddText(regular,regular->FontSize,ImVec2(origin.x+(480*uiScale-width)*.5f,origin.y+56*uiScale),ImGui::GetColorU32(ImGuiCol_Text),title);
        width=caption->CalcTextSizeA(caption->FontSize,10000,0,detail).x;draw->AddText(caption,caption->FontSize,ImVec2(origin.x+(480*uiScale-width)*.5f,origin.y+84*uiScale),ImGui::GetColorU32(ImGuiCol_TextDisabled),detail);
    }ImGui::EndChild();
    auto count=std::to_string(state.visibleLicenseCount)+" de "+std::to_string(state.data.total)+u8" licenças nesta estação";
    Ui::Text(320,136+historyHeight-28,480,20,count.c_str(),muted,caption);
}

void TeamPage(UiState& state){
    if(state.data.role!="owner"||state.data.maxDays==0){Ui::Text(40,152,792,36,u8"Sua autorização vem do proprietário",text,heading);Ui::Wrapped(40,212,680,u8"Em Minha estação, salve sua solicitação e envie ao proprietário. Depois importe a autorização recebida. A chave principal não é compartilhada com integrantes.");if(Ui::Button(40,304,300,40,u8"Abrir minha estação",false,true))Switch(state,Page::Station);return;}
    const float teamRows=state.data.grants.empty()?40.f:40.f*static_cast<float>(std::min<size_t>(state.data.grants.size(),6));
    Ui::Panel(24,136,348,360);Ui::Panel(388,136,428,104+teamRows);
    Ui::Text(40,148,316,28,"Autorizar integrante",text,regular);Ui::Text(404,148,396,28,u8"Estações autorizadas",text,regular);
    if(Ui::Button(40,192,Ui::ButtonWidth(u8"Abrir solicitação",ICON_FA_FOLDER_OPEN),40,u8"Abrir solicitação",false,false,ICON_FA_FOLDER_OPEN))state.pick=Picker::Request;
    if(state.requestText.empty())Ui::Wrapped(40,240,316,u8"Selecione o arquivo .zb2station enviado pelo integrante.");
    else{Ui::Wrapped(40,236,316,state.requestName.c_str(),text);Ui::Text(40,276,316,24,state.requestId.c_str(),muted,caption);}
    Label(40,320,u8"Autorização (dias)");Label(208,320,u8"Máximo por licença");
    Ui::Input(40,344,100,36,"##grantDays","365",state.grantDays,sizeof(state.grantDays),ImGuiInputTextFlags_CharsDecimal,ICON_FA_CALENDAR);
    Ui::Input(208,344,100,36,"##maxDays","30",state.maxDays,sizeof(state.maxDays),ImGuiInputTextFlags_CharsDecimal,ICON_FA_CALENDAR);
    Ui::Wrapped(40,396,316,u8"Exemplo: emitir por 365 dias, com licenças de até 30 dias.");
    const int period=Number(state.grantDays),maximum=Number(state.maxDays);
    ImGui::BeginDisabled(state.requestText.empty()||period<1||period>3650||maximum<1||maximum>period);
    if(Ui::Button(40,440,316,40,u8"Autorizar e gerar ZIP",false,true,ICON_FA_WAND_MAGIC_SPARKLES))RequestAction(state,"authorize_package",{state.requestText,state.grantDays,state.maxDays});ImGui::EndDisabled();
    ImGui::SetCursorPos(Ui::P(404,192));ImGui::BeginChild("team-list",Ui::P(396,32+teamRows),false);
    if(ImGui::BeginTable("team",3,ImGuiTableFlags_RowBg|ImGuiTableFlags_BordersInnerH|ImGuiTableFlags_SizingFixedFit)){
        ImGui::TableSetupColumn("Integrante",ImGuiTableColumnFlags_WidthStretch);ImGui::TableSetupColumn("Validade",ImGuiTableColumnFlags_WidthFixed,150*uiScale);ImGui::TableSetupColumn("Limite",ImGuiTableColumnFlags_WidthFixed,52*uiScale);TableHeader({"INTEGRANTE","VALIDADE","LIMITE"});
        for(const auto& row:state.data.grants){ImGui::PushID(row.id.c_str());ImGui::TableNextRow(ImGuiTableRowFlags_None,40*uiScale);ImGui::TableNextColumn();auto at=ImGui::GetCursorScreenPos();if(ImGui::Selectable("##grant",state.selectedGrant==row.id,ImGuiSelectableFlags_SpanAllColumns,ImVec2(0,26*uiScale)))state.selectedGrant=row.id;if(ImGui::BeginPopupContextItem("grant-actions")){if(ImGui::MenuItem("Salvar ZIP desta estação"))RequestAction(state,"team_package",{row.id});ImGui::EndPopup();}Ui::Hint("Botão direito: ações desta estação");ImGui::GetWindowDrawList()->AddText(ImVec2(at.x+14*uiScale,at.y),ImGui::GetColorU32(ImGuiCol_Text),row.name.c_str());ImGui::TableNextColumn();Ui::Tabular(row.expiry.c_str());ImGui::TableNextColumn();ImGui::TextUnformatted(row.days.c_str());ImGui::PopID();}ImGui::EndTable();
    }
    if(state.data.grants.empty())ImGui::TextDisabled(u8"Nenhuma estação autorizada.");ImGui::EndChild();

}
void StationPage(UiState& state){
    Ui::Panel(24,136,372,384);
    const bool owner=state.data.role=="owner"&&state.data.maxDays>0;
    Ui::Panel(412,136,404,owner?208.f:384.f);
    if(!state.data.hasKey&&Ui::Button(428,468,372,36,"Restaurar backup de proprietário",false,false,ICON_FA_SHIELD_HALVED))Switch(state,Page::Recovery);
    if(owner){Ui::Panel(412,360,404,160);Ui::Text(428,376,372,24,"Histórico local",text,regular);auto summary=std::to_string(state.data.total)+" licenças · "+std::to_string(state.data.grants.size())+" estações autorizadas";Ui::Text(428,416,372,24,summary.c_str(),text,regular);Ui::Wrapped(428,460,372,"Os registros ficam neste PC. Envie o ZIP gerado para cada cliente ou integrante.");}
    Ui::Text(40,148,340,28,u8"Minha estação",text,regular);Label(40,180,"ID completo deste computador");Ui::Device(40,208,340,state.data.device);
    if(Ui::Button(40,256,200,36,"Copiar ID completo",false,false,ICON_FA_COPY))Copy(state,state.data.device);
    Label(40,308,u8"Estação emissora");Ui::Text(40,332,340,24,state.data.stationId.empty()?u8"Ainda não configurada":state.data.stationId.c_str(),text,caption);
    Label(40,376,u8"Usuário Windows");Ui::Text(40,400,340,24,state.data.user.c_str(),text,regular);
    Label(40,444,u8"Responsável pela estação");Ui::Wrapped(40,468,340,state.data.name.empty()?u8"Não configurado":state.data.name.c_str(),text);
    Ui::Text(428,148,372,28,state.data.hasKey?u8"Seu acesso de emissão":u8"Autorizar este PC para emitir",text,regular);
    if(!state.data.hasKey){
        Label(428,180,"Seu nome neste PC (não cadastra outra pessoa)");Ui::Input(428,208,372,36,"##memberName","Seu nome",state.name,sizeof(state.name),0,ICON_FA_USER);
        ImGui::BeginDisabled(strlen(state.name)<2);if(Ui::Button(428,264,372,40,u8"Criar solicitação",false,true,ICON_FA_KEY))RequestAction(state,"create_station",{state.name},true);ImGui::EndDisabled();
        Ui::Wrapped(428,324,372,u8"Este botão prepara o SEU computador como integrante. Para cadastrar outro integrante, use a aba Equipe na conta do proprietário.");
        ImGui::BeginDisabled(strlen(state.name)<2);if(Ui::Button(428,420,372,36,u8"Configurar proprietário neste PC",true))state.pick=Picker::Owner;ImGui::EndDisabled();
    }else if(state.data.role=="owner"){
        if(state.data.maxDays>0){
            Ui::Text(428,192,372,28,u8"Proprietário",IM_COL32(146,186,117,255));Ui::Wrapped(428,240,372,u8"Você pode gerar licenças e autorizar integrantes. As chaves da equipe são independentes da sua.");

        }else{Ui::Wrapped(428,192,372,state.data.problem.c_str());if(Ui::Button(428,340,372,40,"Verificar novamente"))RequestAction(state,"snapshot");}
    }else if(state.data.maxDays<=0){
        Ui::Wrapped(428,192,372,u8"Este PC foi configurado como integrante emissor. O nome abaixo identifica quem usa este PC; não é um membro cadastrado à distância.");
        if(Ui::Button(428,264,372,40,u8"Salvar solicitação",false,false,ICON_FA_DOWNLOAD))RequestAction(state,"export_request",{},true);
        if(Ui::Button(428,320,372,40,u8"Importar autorização",false,true,ICON_FA_FOLDER_OPEN))state.pick=Picker::Authorization;
        Ui::Wrapped(428,392,372,u8"Recebeu um ZIP pronto? Extraia tudo e abra o Admin da pasta extraída. A autorização será reconhecida automaticamente.");
        if(Ui::Button(428,480,372,32,u8"Sou o proprietário: recuperar acesso",true))state.pick=Picker::Owner;
    }else{
        Ui::Text(428,192,372,28,u8"Emissão autorizada",IM_COL32(146,186,117,255));Label(428,244,u8"Autorização válida até");Ui::Text(428,272,372,24,state.data.grantExpiry.c_str());
        auto maximum="Até "+std::to_string(state.data.maxDays)+" dias por licença.";Ui::Wrapped(428,324,372,maximum.c_str());
        if(Ui::Button(428,404,372,40,u8"Emitir licença",false,true))Switch(state,Page::Licenses);
        if(Ui::Button(428,460,372,28,u8"Importar nova autorização",true))state.pick=Picker::Authorization;
    }
}
void SettingsPage(UiState& state){
    Ui::Text(24,136,792,32,u8"Configurações",text,heading);
    if(Ui::Button(592,132,224,36,"Recuperação do acesso",false,false,ICON_FA_SHIELD_HALVED))Switch(state,Page::Recovery);
    Ui::Panel(24,176,368,352);Ui::Panel(408,176,408,352);
    Ui::Text(40,192,328,24,"Aparência",text,regular);
    if(Ui::Toggle(40,220,328,"Reduzir movimento",state.reducedMotion)){state.saveSettings=true;Ui::SetReducedMotion(state.reducedMotion);}
    Ui::Wrapped(40,272,328,u8"Mantém os controles responsivos e desativa transições. A preferência fica salva neste perfil.");
    Ui::Text(40,352,328,24,"Diagnóstico",text,regular);
    Ui::Wrapped(40,376,328,u8"Exporte versão e contagens para suporte, sem nomes, IDs, chaves ou licenças.");
    if(Ui::Button(40,456,232,40,"Exportar diagnóstico",false,true,ICON_FA_DOWNLOAD))state.exportDiagnostics=true;
    Ui::Text(424,192,376,24,"Pastas",text,regular);
    if(Ui::Button(424,220,188,36,"Meus dados",false,false,ICON_FA_FOLDER_OPEN))state.openFolder=1;
    if(Ui::Button(624,220,176,36,"Pacotes",false,false,ICON_FA_FOLDER_OPEN))state.openFolder=2;
    if(Ui::Button(424,268,188,36,"Aplicativo",false,false,ICON_FA_DESKTOP))state.openFolder=3;
    if(Ui::Button(624,268,176,36,"Logs do menu",false,false,ICON_FA_FILE_LINES))state.openFolder=4;
    Ui::Text(424,328,376,24,"Limpeza segura",text,regular);
    Ui::Wrapped(424,364,376,"Verifica serviços antigos do Admin. Preserva o serviço atual, licenças, configurações e arquivos do jogo.");
    ImGui::BeginDisabled(state.data.busy);
    if(Ui::Button(424,436,176,36,"Analisar resíduos",false,false,ICON_FA_MAGNIFYING_GLASS))state.inspectCache=true;
    ImGui::BeginDisabled(!state.cacheReviewed||state.cachePlan.entries.empty());
    if(Ui::Button(612,436,188,36,"Limpar analisados",false,false,ICON_FA_TRASH_CAN))state.cleanCache=true;
    ImGui::EndDisabled();ImGui::EndDisabled();
    Ui::Wrapped(424,488,376,state.cacheSummary.empty()?"Analise primeiro. Nada é apagado automaticamente.":state.cacheSummary.c_str());

}
void RecoveryPage(UiState& state){
    const bool owner=state.data.role=="owner"&&state.data.maxDays>0;
    Ui::Text(24,136,580,32,"Recuperação do proprietário",text,heading);
    if(Ui::Button(688,136,128,32,"Voltar",false,false,ICON_FA_ARROW_LEFT))Switch(state,Page::Settings);
    Ui::Panel(24,188,360,336);Ui::Panel(404,188,412,336);
    Ui::Text(40,204,328,24,owner?"Antes de trocar de PC":"Já possui um backup?",text,regular);
    Ui::Wrapped(40,252,328,owner?"Crie um arquivo protegido por senha. No PC novo, abra Meu acesso e restaure esse arquivo. Não apague o PC antigo antes de testar a recuperação.":"Selecione o arquivo .dbrecovery e informe a senha definida ao criá-lo. O programa valida a chave e protege o acesso para este usuário Windows.");
    Ui::Wrapped(40,376,328,"Este backup recupera o acesso de proprietário. Não inclui histórico de clientes, autorizações ou configurações. Guarde esses dados separadamente.");
    Ui::Text(420,204,380,24,owner?"Criar backup protegido":"Restaurar acesso",text,regular);
    Ui::Text(420,248,380,20,"Senha (12 a 128 caracteres)",muted,caption);
    Ui::Input(420,272,380,40,"##recovery-password","Digite a senha",state.recoveryPassword,sizeof(state.recoveryPassword),ImGuiInputTextFlags_Password,ICON_FA_LOCK);
    if(owner){Ui::Text(420,324,380,20,"Repita a senha",muted,caption);Ui::Input(420,348,380,40,"##recovery-confirm","Confirme a senha",state.recoveryConfirm,sizeof(state.recoveryConfirm),ImGuiInputTextFlags_Password,ICON_FA_LOCK);}
    else Ui::Wrapped(420,336,380,state.data.hasKey?"Este perfil já tem uma estação. A recuperação não substitui uma chave existente.":"Você precisa do arquivo e da senha. Não existe recuperação pelo GitHub ou pelo nome de usuário.");
    ImGui::SetCursorPos(Ui::P(420,412));ImGui::Checkbox("Guardei a senha em local seguro",&state.recoveryAcknowledged);
    const bool password=strlen(state.recoveryPassword)>=12&&strlen(state.recoveryPassword)<256;
    ImGui::BeginDisabled(state.data.busy||!password||!state.recoveryAcknowledged||(owner?strcmp(state.recoveryPassword,state.recoveryConfirm)!=0:state.data.hasKey));
    if(Ui::Button(420,464,380,40,owner?"Salvar recuperação protegida":"Escolher backup e restaurar",false,true,owner?ICON_FA_DOWNLOAD:ICON_FA_FOLDER_OPEN)){state.exportRecovery=owner;state.restoreRecovery=!owner;}
    ImGui::EndDisabled();
}
void HelpPage(UiState& state){
    Ui::Text(24,136,792,32,"Primeiro envie o programa. Depois ative o acesso.",text,heading);
    Ui::Panel(24,184,380,344);Ui::Panel(424,184,392,344);
    Ui::Text(40,196,348,24,"CLIENTE | quem vai jogar",text,regular);
    Ui::Wrapped(40,236,348,u8"1. Salve o programa inicial no botão abaixo.\n2. Envie esse ZIP ao cliente. Não precisa de ID.\n3. Ele extrai e abre ZB2Menu.exe. Em Meu acesso, clica em Copiar ID e envia para você.");
    ImGui::BeginDisabled(state.data.busy);
    if(Ui::Button(40,320,348,36,"Salvar programa inicial",false,true,ICON_FA_DOWNLOAD))RequestAction(state,"client_starter");
    ImGui::EndDisabled();
    Ui::Wrapped(40,376,348,u8"4. Em Clientes, preencha nome, ID e dias.\n5. Clique em Gerar ZIP do cliente e envie o novo ZIP.\n\nO cliente extrai o novo pacote e abre ZB2Menu.exe. A licença já está incluída.");
    Ui::Text(440,196,360,24,"EQUIPE | quem vai emitir licenças",text,regular);
    Ui::Wrapped(440,236,360,u8"1. Salve e envie o programa inicial da equipe.\n2. O integrante extrai e abre ZB2Admin.exe.\n3. Em Meu acesso, informa o nome e cria uma solicitação. Ele envia o arquivo para você.");
    ImGui::BeginDisabled(state.data.busy);ImGui::PushID("team-starter");
    if(Ui::Button(440,320,360,36,"Salvar programa inicial",false,true,ICON_FA_DOWNLOAD))RequestAction(state,"team_starter");
    ImGui::PopID();ImGui::EndDisabled();
    Ui::Wrapped(440,376,360,u8"4. Você abre Minha equipe → Abrir solicitação.\n5. Define os limites e clica em Autorizar e gerar ZIP. Envie esse novo ZIP ao integrante.\n\nEle extrai e abre o Admin no PC autorizado. Sua chave de proprietário não vai no pacote.");

}
}
void Draw(UiState& state){
    if(state.revision!=state.data.revision){state.revision=state.data.revision;
        const bool relevant=!state.started||(state.pendingFeedback&&state.requestPage==state.page);
        if(relevant)state.message=state.data.notice;
        if(!state.data.issuedId.empty()){state.selectedLicense=state.data.issuedId;if(state.page==Page::Licenses)state.search[0]=0;}
        if(!state.started&&state.data.initialized){state.started=true;if(state.name[0]==0)strcpy_s(state.name,state.data.user.c_str());if(!state.data.hasKey||state.data.maxDays==0)state.page=Page::Station;}
        if(!state.data.requestText.empty()){state.requestText=state.data.requestText;state.requestName=state.data.requestName;state.requestId=state.data.requestId;}
        if(relevant&&!state.data.output.empty()){state.exportText=state.data.output;state.exportName=state.data.filename;if(state.saveAfterReply)state.save=true;}
        state.saveAfterReply=false;state.pendingFeedback=false;
        if(state.data.maxDays>0 && Number(state.days)>state.data.maxDays)snprintf(state.days,sizeof(state.days),"%d",state.data.maxDays);
    }
    ImGui::SetNextWindowPos(ImVec2(0,0));ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("ZB2 Admin",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar);
    Ui::Brand(24,16,"ADMIN");const std::string identity=state.data.name+(state.data.role=="owner"&&state.data.maxDays>0?u8" · Proprietário":state.data.maxDays>0?" · Integrante":" · Sem autorização");Ui::Text(424,20,304,28,identity.c_str(),muted,caption,2);
    if(Ui::Button(748,20,28,28,ICON_FA_MINUS,true))state.minimize=true;
    if(Ui::Button(788,20,28,28,ICON_FA_XMARK,true))state.close=true;
    const char* tabs[]={"Clientes","Minha equipe","Meu acesso",u8"Configurações","Como enviar"};
    const Page tabPages[]={Page::Licenses,Page::Team,Page::Station,Page::Settings,Page::Help};
    const char* tabIcons[]={ICON_FA_USER,ICON_FA_USERS,ICON_FA_DESKTOP,ICON_FA_GEAR,ICON_FA_CIRCLE_QUESTION};
    for(int i=0;i<5;++i)if(Ui::Button(24+160.f*static_cast<float>(i),80,152,36,tabs[i],true,state.page==tabPages[i],tabIcons[i]))Switch(state,tabPages[i]);
    state.fade=std::min(1.f,state.fade+ImGui::GetIO().DeltaTime/.12f);ImGui::PushStyleVar(ImGuiStyleVar_Alpha,state.fade);
    if(state.page==Page::Help){
        if(state.credits){
            Ui::Panel(156,148,528,356);
            Ui::Text(180,184,480,40,"ZB2 Pro Menu",text,heading,1);
            Ui::Text(180,232,480,24,"DirectX 11 Hook & Modding Engine",muted,regular,1);
            Ui::Text(180,280,480,24,"OTheMandaloriano & Equipe",text,regular,1);
            Ui::Text(180,316,480,24,u8"v1.16-local · Alpha Build",muted,caption,1);
            Ui::Text(180,360,480,24,state.data.maxDays>0?u8"Emissão Autorizada":u8"Aguardando autorização",state.data.maxDays>0?IM_COL32(110,231,183,255):muted,regular,1);
            if(Ui::Button(292,432,256,36,u8"Voltar à Ajuda",false,false,ICON_FA_ARROW_LEFT)){state.credits=false;state.message.clear();state.exportText.clear();}

        }
        else{HelpPage(state);if(Ui::Button(704,136,112,32,"Créditos",true)){state.credits=true;state.message.clear();state.exportText.clear();}}
    }
    else if(state.page==Page::Settings)SettingsPage(state);
    else if(state.page==Page::Recovery)RecoveryPage(state);
    else if(!state.data.initialized){Ui::Text(24,200,792,40,state.data.busy?u8"Verificando estação...":u8"Não foi possível abrir a estação",text,heading,1);Ui::Wrapped(100,276,640,state.message.c_str());if(!state.data.busy&&Ui::Button(270,388,300,40,"Verificar novamente"))RequestAction(state,"snapshot");}
    else{ImGui::BeginDisabled(state.data.busy);switch(state.page){case Page::Licenses:LicensePage(state);break;case Page::Team:TeamPage(state);break;case Page::Station:StationPage(state);break;default:break;}ImGui::EndDisabled();}
    ImGui::PopStyleVar();ImGui::GetWindowDrawList()->AddLine(Ui::P(24,540),Ui::P(816,540),IM_COL32(49,49,55,255));
    const std::string message=state.data.busy&&state.requestPage==state.page?"Processando...":state.message.empty()?"ZB2 Admin 1.16":state.message;
    ImGui::GetWindowDrawList()->PushClipRect(Ui::P(24,548),Ui::P(816,588),true);Ui::Text(24,548,792,28,message.c_str(),state.data.error?IM_COL32(231,153,151,255):muted,caption);ImGui::GetWindowDrawList()->PopClipRect();
    ImGui::SetCursorPos(Ui::P(24,548));ImGui::InvisibleButton("##notice",Ui::P(792,32));Ui::Hint(message.c_str());
    ImGui::End();
}
}
