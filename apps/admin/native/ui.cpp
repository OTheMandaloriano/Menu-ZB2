#include "ui.h"
#include "../../shared/widgets.h"
#include "../../loader/assets/IconsFontAwesome6.h"
#include "../../loader/license.h"
#include <algorithm>
#include <charconv>
#include <cctype>
namespace Admin {
namespace {
using namespace UiTheme;
int Number(const char* input){int value=0;auto n=strlen(input);auto result=std::from_chars(input,input+n,value);return result.ec==std::errc()&&result.ptr==input+n?value:0;}
void RequestAction(UiState& state,const char* command,std::vector<std::string> args={},bool save=false){state.action={command,std::move(args)};state.send=true;state.saveAfterReply=save;state.message.clear();}
void Label(float x,float y,const char* value){Ui::Text(x,y,330,20,value,muted,caption);}
void Switch(UiState& state,Page page){if(state.page!=page){state.page=page;state.fade=Ui::ReducedMotion()?1.f:0.f;}}
void Copy(UiState& state,const std::string& value){state.copy=true;state.copyText=value;state.message="Copiado.";}
void Export(UiState& state,const std::string& value,const std::string& name){state.exportText=value;state.exportName=name;state.save=true;}
bool ValidDevice(const char* input){return strlen(input)==64&&std::all_of(input,input+64,[](unsigned char c){return std::isxdigit(c)!=0;});}
void LicensePage(UiState& state){
    if(state.data.maxDays<=0){
        Ui::Text(24,152,792,36,u8"Autorize esta estação para emitir",text,heading);
        Ui::Wrapped(24,212,680,u8"A emissão fica disponível depois que o proprietário autorizar este PC. Se você só vai usar o menu, utilize o pacote CLIENTE.");
        if(Ui::Button(24,300,300,40,u8"Abrir minha estação",false,true))Switch(state,Page::Station);return;
    }
    Ui::Text(24,136,328,28,u8"Emitir licença",text,regular);Ui::Text(400,136,416,28,u8"Histórico desta estação",text,regular);
    Label(24,180,"Cliente");Ui::Input(24,204,328,36,"##customer","Nome ou identificador",state.customer,sizeof(state.customer),0,ICON_FA_USER);
    Label(24,256,"ID do computador do cliente");Ui::Input(24,280,328,36,"##device","Cole os 64 caracteres",state.device,sizeof(state.device),0,ICON_FA_MICROCHIP);
    Label(24,332,"Prazo de uso");
    std::string chosen=std::to_string(std::max(1,Number(state.days)))+" dias";
    if(Ui::Button(24,356,192,36,chosen.c_str()))ImGui::OpenPopup("Prazo");
    if(ImGui::BeginPopup("Prazo")){
        for(int amount:{7,15,30,90,180,365}){ImGui::BeginDisabled(amount>state.data.maxDays);std::string label=std::to_string(amount)+" dias";if(ImGui::Selectable(label.c_str()))snprintf(state.days,sizeof(state.days),"%d",amount);ImGui::EndDisabled();}
        ImGui::EndPopup();
    }
    Ui::Input(228,356,124,36,"##days","Dias",state.days,sizeof(state.days),ImGuiInputTextFlags_CharsDecimal,ICON_FA_CALENDAR);
    auto limit="Limite: "+std::to_string(state.data.maxDays)+" dias por licença.";Label(24,408,limit.c_str());
    ImGui::BeginDisabled(!ValidDevice(state.device)||strlen(state.customer)<2||Number(state.days)<1||Number(state.days)>state.data.maxDays);
    if(Ui::Button(24,452,328,44,u8"Gerar licença",false,true,ICON_FA_WAND_MAGIC_SPARKLES))RequestAction(state,"issue",{state.customer,state.device,state.days});ImGui::EndDisabled();
    Ui::Input(400,180,312,36,"##search","Buscar cliente ou ID",state.search,sizeof(state.search),0,ICON_FA_MAGNIFYING_GLASS);
    if(Ui::Button(724,180,92,36,"Buscar",false,false,ICON_FA_MAGNIFYING_GLASS))RequestAction(state,"snapshot",{state.search});
    ImGui::SetCursorPos(Ui::P(400,232));ImGui::BeginChild("license-list",Ui::P(416,228),false);
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,Ui::P(4,6));
    if(ImGui::BeginTable("licenses",4,ImGuiTableFlags_RowBg|ImGuiTableFlags_SizingFixedFit)){
        ImGui::TableSetupColumn("Cliente",ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Validade",ImGuiTableColumnFlags_WidthFixed,108*uiScale);
        ImGui::TableSetupColumn("Status",ImGuiTableColumnFlags_WidthFixed,120*uiScale);
        ImGui::TableSetupColumn("",ImGuiTableColumnFlags_WidthFixed,62*uiScale);ImGui::TableHeadersRow();
        for(const auto& row:state.data.licenses){
            ImGui::PushID(row.id.c_str());ImGui::TableNextRow(ImGuiTableRowFlags_None,36*uiScale);ImGui::TableNextColumn();
            auto at=ImGui::GetCursorScreenPos();if(ImGui::Selectable("##row",state.selectedLicense==row.id,ImGuiSelectableFlags_SpanAllColumns|ImGuiSelectableFlags_AllowItemOverlap,ImVec2(0,24*uiScale)))state.selectedLicense=row.id;
            ImGui::GetWindowDrawList()->AddText(at,ImGui::GetColorU32(ImGuiCol_Text),row.customer.c_str());
            ImGui::TableNextColumn();Ui::Tabular(row.expiry.c_str());
            ImGui::TableNextColumn();const bool active=row.expires?row.expires>License::Now():row.status==u8"Válida"||row.status=="Ativa";
            auto label=std::string(active?"Ativa":"Expirada")+u8" · "+row.days+"d";Ui::Badge(label.c_str(),active);
            ImGui::TableNextColumn();if(Ui::RowCopy(ICON_FA_COPY))Copy(state,row.token);ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::PopStyleVar();
    if(state.data.licenses.empty())ImGui::TextDisabled("Nenhuma emissao encontrada.");ImGui::EndChild();
    const auto found=std::find_if(state.data.licenses.begin(),state.data.licenses.end(),[&](const LicenseRow& row){return row.id==state.selectedLicense;});
    ImGui::BeginDisabled(found==state.data.licenses.end());
    if(Ui::Button(400,476,202,36,"Copiar selecionada",false,false,ICON_FA_COPY)&&found!=state.data.licenses.end())Copy(state,found->token);
    if(Ui::Button(614,476,202,36,"Salvar selecionada",false,false,ICON_FA_DOWNLOAD)&&found!=state.data.licenses.end())Export(state,found->token,"Licenca-"+found->id.substr(0,8)+".zb2license");ImGui::EndDisabled();
}
void TeamPage(UiState& state){
    if(state.data.role!="owner"||state.data.maxDays==0){Ui::Text(24,152,792,36,u8"Sua autorização vem do proprietário",text,heading);Ui::Wrapped(24,212,680,u8"Em Minha estação, salve sua solicitação e envie ao proprietário. Depois importe a autorização recebida. A chave principal não é compartilhada com integrantes.");if(Ui::Button(24,304,300,40,u8"Abrir minha estação",false,true))Switch(state,Page::Station);return;}
    Ui::Text(24,136,328,28,"Autorizar integrante",text,regular);Ui::Text(400,136,416,28,u8"Estações autorizadas",text,regular);
    if(Ui::Button(24,180,328,40,u8"Abrir solicitação",false,false,ICON_FA_FOLDER_OPEN))state.pick=Picker::Request;
    if(state.requestText.empty())Ui::Wrapped(24,240,328,u8"Selecione o arquivo .zb2station enviado pelo integrante.");
    else{Ui::Wrapped(24,236,328,state.requestName.c_str(),text);Ui::Text(24,276,328,24,state.requestId.c_str(),muted,caption);}
    Label(24,320,u8"Autorização (dias)");Label(200,320,u8"Máximo por licença");
    Ui::Input(24,344,152,36,"##grantDays","365",state.grantDays,sizeof(state.grantDays),ImGuiInputTextFlags_CharsDecimal,ICON_FA_CALENDAR);
    Ui::Input(200,344,152,36,"##maxDays","30",state.maxDays,sizeof(state.maxDays),ImGuiInputTextFlags_CharsDecimal,ICON_FA_CALENDAR);
    Ui::Wrapped(24,396,328,u8"Exemplo: emitir por 365 dias, com licenças de até 30 dias.");
    const int period=Number(state.grantDays),maximum=Number(state.maxDays);
    ImGui::BeginDisabled(state.requestText.empty()||period<1||period>3650||maximum<1||maximum>period);
    if(Ui::Button(24,452,328,44,u8"Gerar autorização",false,true,ICON_FA_WAND_MAGIC_SPARKLES))RequestAction(state,"authorize",{state.requestText,state.grantDays,state.maxDays},true);ImGui::EndDisabled();
    ImGui::SetCursorPos(Ui::P(400,180));ImGui::BeginChild("team-list",Ui::P(416,280),false);
    if(ImGui::BeginTable("team",3,ImGuiTableFlags_RowBg|ImGuiTableFlags_SizingStretchProp)){
        ImGui::TableSetupColumn("Integrante");ImGui::TableSetupColumn("Validade");ImGui::TableSetupColumn("Limite");ImGui::TableHeadersRow();
        for(const auto& row:state.data.grants){ImGui::PushID(row.id.c_str());ImGui::TableNextRow(ImGuiTableRowFlags_None,32*uiScale);ImGui::TableNextColumn();auto at=ImGui::GetCursorScreenPos();if(ImGui::Selectable("##grant",state.selectedGrant==row.id,ImGuiSelectableFlags_SpanAllColumns,ImVec2(0,26*uiScale)))state.selectedGrant=row.id;ImGui::GetWindowDrawList()->AddText(at,ImGui::GetColorU32(ImGuiCol_Text),row.name.c_str());ImGui::TableNextColumn();ImGui::TextUnformatted(row.expiry.c_str());ImGui::TableNextColumn();ImGui::TextUnformatted(row.days.c_str());ImGui::PopID();}ImGui::EndTable();
    }
    if(state.data.grants.empty())ImGui::TextDisabled("Nenhuma estacao autorizada.");ImGui::EndChild();
    const auto found=std::find_if(state.data.grants.begin(),state.data.grants.end(),[&](const GrantRow& row){return row.id==state.selectedGrant;});
    ImGui::BeginDisabled(found==state.data.grants.end());if(Ui::Button(400,476,416,36,u8"Salvar autorização selecionada",false,false,ICON_FA_DOWNLOAD)&&found!=state.data.grants.end())Export(state,found->token,"Autorizacao-"+found->id.substr(0,8)+".zb2issuer");ImGui::EndDisabled();
}
void StationPage(UiState& state){
    Ui::Text(24,136,370,28,u8"Minha estação",text,regular);Label(24,180,"ID completo deste computador");Ui::Device(24,208,352,state.data.device);
    if(Ui::Button(24,272,352,28,"Copiar ID completo",false,false,ICON_FA_COPY))Copy(state,state.data.device);
    Label(24,328,u8"Estação emissora");Ui::Text(24,352,352,24,state.data.stationId.empty()?u8"Ainda não configurada":state.data.stationId.c_str(),text,caption);
    Label(24,396,u8"Usuário Windows");Ui::Text(24,420,352,24,state.data.user.c_str(),text,regular);
    Label(24,464,u8"Responsável pela estação");Ui::Wrapped(24,488,352,state.data.name.empty()?u8"Não configurado":state.data.name.c_str(),text);
    Ui::Text(424,136,392,28,state.data.hasKey?u8"Seu acesso de emissão":u8"Preparar este PC",text,regular);
    if(!state.data.hasKey){
        Label(424,180,"Nome do integrante");Ui::Input(424,208,392,36,"##memberName","Seu nome",state.name,sizeof(state.name),0,ICON_FA_USER);
        ImGui::BeginDisabled(strlen(state.name)<2);if(Ui::Button(424,264,392,40,u8"Criar solicitação",false,true,ICON_FA_KEY))RequestAction(state,"create_station",{state.name},true);ImGui::EndDisabled();
        Ui::Wrapped(424,324,392,u8"Envie a solicitação ao proprietário. Ele devolverá uma autorização para este PC.");
        ImGui::BeginDisabled(strlen(state.name)<2);if(Ui::Button(424,420,392,36,u8"Configurar proprietário neste PC",true))state.pick=Picker::Owner;ImGui::EndDisabled();
    }else if(state.data.role=="owner"){
        if(state.data.maxDays>0){
            Ui::Text(424,192,392,28,u8"Proprietário",IM_COL32(146,186,117,255));Ui::Wrapped(424,240,392,u8"Você pode gerar licenças e autorizar integrantes. As chaves da equipe são independentes da sua.");
            if(Ui::Button(424,352,392,40,"Abrir equipe",false,true))Switch(state,Page::Team);
        }else{Ui::Wrapped(424,192,392,state.data.problem.c_str());if(Ui::Button(424,352,392,40,"Verificar novamente"))RequestAction(state,"snapshot");}
    }else if(state.data.maxDays<=0){
        Ui::Wrapped(424,192,392,u8"Aguardando uma autorização válida do proprietário.");
        if(Ui::Button(424,264,392,40,u8"Salvar solicitação",false,false,ICON_FA_DOWNLOAD))RequestAction(state,"export_request",{},true);
        if(Ui::Button(424,320,392,40,u8"Importar autorização",false,true,ICON_FA_FOLDER_OPEN))state.pick=Picker::Authorization;
        Ui::Wrapped(424,392,392,u8"Importe o arquivo .zb2issuer recebido. Não use uma licença de cliente nesta etapa.");
    }else{
        Ui::Text(424,192,392,28,u8"Emissão autorizada",IM_COL32(146,186,117,255));Label(424,244,u8"Autorização válida até");Ui::Text(424,272,392,24,state.data.grantExpiry.c_str());
        auto maximum="Até "+std::to_string(state.data.maxDays)+" dias por licença.";Ui::Wrapped(424,324,392,maximum.c_str());
        if(Ui::Button(424,404,392,40,u8"Emitir licença",false,true))Switch(state,Page::Licenses);
        if(Ui::Button(424,460,392,28,u8"Importar nova autorização",true))state.pick=Picker::Authorization;
    }
}
void HelpPage(){
    Ui::Text(24,136,792,32,"O que enviar",text,heading);
    Ui::Panel(24,200,380,272);Ui::Panel(424,200,392,272);
    Ui::Text(44,216,340,28,"Para o cliente",text,regular);
    Ui::Wrapped(44,264,340,u8"1. Pacote CLIENTE.zip\n2. Arquivo .zb2license gerado para o PC dele.\n\nO cliente abre ZB2Menu.exe, importa o arquivo e ativa o acesso.");
    Ui::Text(444,216,352,28,"Para integrante que vai emitir",text,regular);
    Ui::Wrapped(444,264,352,u8"1. Pacote EQUIPE.zip\n2. Ele cria uma solicitação .zb2station.\n3. Você autoriza e devolve o .zb2issuer.\n4. Ele importa e pode emitir nos limites concedidos.");
    Ui::Wrapped(24,496,792,u8"Se o integrante só vai usar o menu, envie o pacote CLIENTE. Não envie a chave principal nem os dados privados de uma estação.");
}
}
void Draw(UiState& state){
    if(state.revision!=state.data.revision){state.revision=state.data.revision;state.message=state.data.notice;
        if(!state.started&&state.data.initialized){state.started=true;if(!state.data.hasKey||state.data.maxDays==0)state.page=Page::Station;}
        if(!state.data.requestText.empty()){state.requestText=state.data.requestText;state.requestName=state.data.requestName;state.requestId=state.data.requestId;}
        if(!state.data.output.empty()){state.exportText=state.data.output;state.exportName=state.data.filename;if(state.saveAfterReply)state.save=true;}
        state.saveAfterReply=false;
        if(state.data.maxDays>0 && Number(state.days)>state.data.maxDays)snprintf(state.days,sizeof(state.days),"%d",state.data.maxDays);
    }
    ImGui::SetNextWindowPos(ImVec2(0,0));ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("ZB2 Admin",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar);
    Ui::Text(24,16,320,36,"ZB2 Admin",text,heading);Ui::Text(448,20,280,28,state.data.role=="owner"&&state.data.maxDays>0?u8"Proprietário":state.data.maxDays>0?"Integrante autorizado":"Aguardando acesso",muted,caption,2);
    if(Ui::Button(748,20,28,28,ICON_FA_MINUS,true))state.minimize=true;
    if(Ui::Button(788,20,28,28,ICON_FA_XMARK,true))state.close=true;
    const char* tabs[]={u8"Licenças","Equipe",u8"Minha estação","Ajuda"};
    const char* tabIcons[]={ICON_FA_KEY,ICON_FA_USERS,ICON_FA_DESKTOP,ICON_FA_CIRCLE_QUESTION};
    for(int i=0;i<4;++i)if(Ui::Button(24+198.f*static_cast<float>(i),80,190,36,tabs[i],true,state.page==static_cast<Page>(i),tabIcons[i]))Switch(state,static_cast<Page>(i));
    state.fade=std::min(1.f,state.fade+ImGui::GetIO().DeltaTime/.12f);ImGui::PushStyleVar(ImGuiStyleVar_Alpha,state.fade);
    if(state.page==Page::Help)HelpPage();
    else if(!state.data.initialized){Ui::Text(24,200,792,40,state.data.busy?u8"Verificando estação...":u8"Não foi possível abrir a estação",text,heading,1);Ui::Wrapped(100,276,640,state.message.c_str());if(!state.data.busy&&Ui::Button(270,388,300,40,"Verificar novamente"))RequestAction(state,"snapshot");}
    else{ImGui::BeginDisabled(state.data.busy);switch(state.page){case Page::Licenses:LicensePage(state);break;case Page::Team:TeamPage(state);break;case Page::Station:StationPage(state);break;default:break;}ImGui::EndDisabled();}
    ImGui::PopStyleVar();ImGui::GetWindowDrawList()->AddLine(Ui::P(24,540),Ui::P(816,540),IM_COL32(49,49,55,255));
    const std::string message=state.data.busy?"Processando...":state.message.empty()?"1.3  |  Dados locais nesta estação":state.message;
    ImGui::GetWindowDrawList()->PushClipRect(Ui::P(24,548),Ui::P(state.exportText.empty()?816.f:600.f,588),true);Ui::Text(24,548,576,28,message.c_str(),state.data.error?IM_COL32(231,153,151,255):muted,caption);ImGui::GetWindowDrawList()->PopClipRect();
    ImGui::SetCursorPos(Ui::P(24,548));ImGui::InvisibleButton("##notice",Ui::P(state.exportText.empty()?792.f:576.f,32));Ui::Hint(message.c_str());
    if(!state.exportText.empty()){
        if(Ui::Button(612,548,92,32,"Copiar"))Copy(state,state.exportText);
        if(Ui::Button(716,548,100,32,"Salvar",false,true))state.save=true;
    }
    ImGui::End();
}
}
