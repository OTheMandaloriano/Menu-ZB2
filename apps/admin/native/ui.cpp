#include "ui.h"
#include "../../shared/widgets.h"
#include "../../loader/assets/IconsFontAwesome6.h"
#include "../../loader/license.h"
#include "../../shared/resources.h"
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
bool ValidDevice(const char* input){return strlen(input)==64&&std::all_of(input,input+64,[](unsigned char c){return std::isxdigit(c)!=0;});}
void LicensePage(UiState& state){
    if(state.data.maxDays<=0){
        Ui::Text(24,152,792,36,u8"Autorize esta estação para emitir",text,heading);
        Ui::Wrapped(24,212,680,u8"A emissão fica disponível depois que o proprietário autorizar este PC. Se você só vai usar o menu, utilize o pacote CLIENTE.");
        if(Ui::Button(24,300,300,40,u8"Abrir minha estação",false,true))Switch(state,Page::Station);return;
    }
    Ui::Text(24,136,328,28,"Pacote do cliente",text,regular);Ui::Text(400,136,416,28,u8"Histórico desta estação",text,regular);
    Label(24,180,"Cliente");Ui::Input(24,204,328,36,"##customer","Nome ou identificador",state.customer,sizeof(state.customer),0,ICON_FA_USER);
    Label(24,256,"ID do computador do cliente");Ui::Input(24,280,328,36,"##device","Cole os 64 caracteres",state.device,sizeof(state.device),0,ICON_FA_MICROCHIP);
    Label(24,332,"Prazo de uso");
    Ui::Input(24,356,328,40,"##days","Quantidade de dias",state.days,sizeof(state.days),ImGuiInputTextFlags_CharsDecimal,ICON_FA_CALENDAR);
    auto limit="Limite: "+std::to_string(state.data.maxDays)+" dias por licença.";Label(24,408,limit.c_str());
    ImGui::BeginDisabled(!ValidDevice(state.device)||strlen(state.customer)<2||Number(state.days)<1||Number(state.days)>state.data.maxDays);
    if(Ui::Button(24,440,328,40,u8"Gerar ZIP do cliente",false,true,ICON_FA_WAND_MAGIC_SPARKLES))RequestAction(state,"issue_package",{state.customer,state.device,state.days});ImGui::EndDisabled();
    ImGui::BeginDisabled(!ValidDevice(state.device)||strlen(state.customer)<2||Number(state.days)<1||Number(state.days)>state.data.maxDays);
    if(Ui::Button(24,492,328,36,"Gerar somente a chave",false,false,ICON_FA_KEY))RequestAction(state,"issue",{state.customer,state.device,state.days});ImGui::EndDisabled();
    Ui::Input(400,180,416,40,"##search","Buscar cliente ou ID",state.search,sizeof(state.search),0,ICON_FA_MAGNIFYING_GLASS);

    ImGui::SetCursorPos(Ui::P(400,232));ImGui::BeginChild("license-list",Ui::P(416,228),false);
    state.visibleLicenseCount=0;
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,Ui::P(4,6));
    if(ImGui::BeginTable("licenses",4,ImGuiTableFlags_RowBg|ImGuiTableFlags_SizingFixedFit)){
        ImGui::TableSetupColumn("Cliente",ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Validade",ImGuiTableColumnFlags_WidthFixed,108*uiScale);
        ImGui::TableSetupColumn("Status",ImGuiTableColumnFlags_WidthFixed,120*uiScale);
        ImGui::TableSetupColumn("",ImGuiTableColumnFlags_WidthFixed,62*uiScale);ImGui::TableHeadersRow();
        for(const auto& row:state.data.licenses){
            std::string haystack=row.customer+" "+row.device+" "+row.issuer,query=state.search;std::transform(haystack.begin(),haystack.end(),haystack.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});std::transform(query.begin(),query.end(),query.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});if(haystack.find(query)==std::string::npos)continue;++state.visibleLicenseCount;
            ImGui::PushID(row.id.c_str());ImGui::TableNextRow(ImGuiTableRowFlags_None,36*uiScale);ImGui::TableNextColumn();
            auto at=ImGui::GetCursorScreenPos();if(ImGui::Selectable("##row",state.selectedLicense==row.id,ImGuiSelectableFlags_SpanAllColumns|ImGuiSelectableFlags_AllowItemOverlap,ImVec2(0,24*uiScale)))state.selectedLicense=row.id;
            ImGui::GetWindowDrawList()->AddText(at,ImGui::GetColorU32(ImGuiCol_Text),row.customer.c_str());
            ImGui::TableNextColumn();Ui::Tabular(row.expiry.c_str());
            ImGui::TableNextColumn();const bool active=row.expires?row.expires>License::Now():row.status==u8"Válida"||row.status=="Ativa";
            auto label=active?std::string("Ativo")+u8" · "+row.days+"d":std::string("Expirado");Ui::Badge(label.c_str(),active);
            ImGui::TableNextColumn();if(Ui::RowCopy(ICON_FA_COPY))Copy(state,row.token);ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::PopStyleVar();
    if(state.visibleLicenseCount==0){
        const char* title=state.search[0]?"Nenhum resultado":u8"Seu histórico começa aqui";
        const char* detail=state.search[0]?"Tente outro nome ou ID.":"Gere o primeiro pacote para um cliente.";
        auto* draw=ImGui::GetWindowDrawList();auto origin=ImGui::GetWindowPos();
        float width=regular->CalcTextSizeA(regular->FontSize,10000,0,title).x;
        draw->AddText(regular,regular->FontSize,ImVec2(origin.x+(416*uiScale-width)*.5f,origin.y+88*uiScale),ImGui::GetColorU32(ImGuiCol_Text),title);
        width=caption->CalcTextSizeA(caption->FontSize,10000,0,detail).x;draw->AddText(caption,caption->FontSize,ImVec2(origin.x+(416*uiScale-width)*.5f,origin.y+116*uiScale),ImGui::GetColorU32(ImGuiCol_TextDisabled),detail);
    }ImGui::EndChild();
    const auto found=std::find_if(state.data.licenses.begin(),state.data.licenses.end(),[&](const LicenseRow& row){return row.id==state.selectedLicense;});
    ImGui::BeginDisabled(found==state.data.licenses.end());
    if(Ui::Button(400,476,202,36,"Copiar selecionada",false,false,ICON_FA_COPY)&&found!=state.data.licenses.end())Copy(state,found->token);
    if(Ui::Button(614,476,202,36,"Gerar ZIP pronto",false,false,ICON_FA_DOWNLOAD)&&found!=state.data.licenses.end())RequestAction(state,"client_package",{found->id});ImGui::EndDisabled();
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
    if(Ui::Button(24,440,328,40,u8"Autorizar e gerar ZIP",false,true,ICON_FA_WAND_MAGIC_SPARKLES))RequestAction(state,"authorize_package",{state.requestText,state.grantDays,state.maxDays});ImGui::EndDisabled();
    ImGui::SetCursorPos(Ui::P(400,180));ImGui::BeginChild("team-list",Ui::P(416,280),false);
    if(ImGui::BeginTable("team",3,ImGuiTableFlags_RowBg|ImGuiTableFlags_SizingStretchProp)){
        ImGui::TableSetupColumn("Integrante");ImGui::TableSetupColumn("Validade");ImGui::TableSetupColumn("Limite");ImGui::TableHeadersRow();
        for(const auto& row:state.data.grants){ImGui::PushID(row.id.c_str());ImGui::TableNextRow(ImGuiTableRowFlags_None,32*uiScale);ImGui::TableNextColumn();auto at=ImGui::GetCursorScreenPos();if(ImGui::Selectable("##grant",state.selectedGrant==row.id,ImGuiSelectableFlags_SpanAllColumns,ImVec2(0,26*uiScale)))state.selectedGrant=row.id;ImGui::GetWindowDrawList()->AddText(at,ImGui::GetColorU32(ImGuiCol_Text),row.name.c_str());ImGui::TableNextColumn();ImGui::TextUnformatted(row.expiry.c_str());ImGui::TableNextColumn();ImGui::TextUnformatted(row.days.c_str());ImGui::PopID();}ImGui::EndTable();
    }
    if(state.data.grants.empty())ImGui::TextDisabled("Nenhuma estacao autorizada.");ImGui::EndChild();
    const auto found=std::find_if(state.data.grants.begin(),state.data.grants.end(),[&](const GrantRow& row){return row.id==state.selectedGrant;});
    ImGui::BeginDisabled(found==state.data.grants.end());if(Ui::Button(400,476,416,36,u8"Gerar ZIP do integrante",false,false,ICON_FA_DOWNLOAD)&&found!=state.data.grants.end())RequestAction(state,"team_package",{found->id});ImGui::EndDisabled();
}
void StationPage(UiState& state){
    Ui::Text(24,136,370,28,u8"Minha estação",text,regular);Label(24,180,"ID completo deste computador");Ui::Device(24,208,352,state.data.device);
    if(Ui::Button(24,256,352,36,"Copiar ID completo",false,false,ICON_FA_COPY))Copy(state,state.data.device);
    Label(24,328,u8"Estação emissora");Ui::Text(24,352,352,24,state.data.stationId.empty()?u8"Ainda não configurada":state.data.stationId.c_str(),text,caption);
    Label(24,396,u8"Usuário Windows");Ui::Text(24,420,352,24,state.data.user.c_str(),text,regular);
    Label(24,464,u8"Responsável pela estação");Ui::Wrapped(24,488,352,state.data.name.empty()?u8"Não configurado":state.data.name.c_str(),text);
    Ui::Text(424,136,392,28,state.data.hasKey?u8"Seu acesso de emissão":u8"Autorizar este PC para emitir",text,regular);
    if(!state.data.hasKey){
        Label(424,180,"Seu nome neste PC (não cadastra outra pessoa)");Ui::Input(424,208,392,36,"##memberName","Seu nome",state.name,sizeof(state.name),0,ICON_FA_USER);
        ImGui::BeginDisabled(strlen(state.name)<2);if(Ui::Button(424,264,392,40,u8"Criar solicitação",false,true,ICON_FA_KEY))RequestAction(state,"create_station",{state.name},true);ImGui::EndDisabled();
        Ui::Wrapped(424,324,392,u8"Este botão prepara o SEU computador como integrante. Para cadastrar outro integrante, use a aba Equipe na conta do proprietário.");
        ImGui::BeginDisabled(strlen(state.name)<2);if(Ui::Button(424,420,392,36,u8"Configurar proprietário neste PC",true))state.pick=Picker::Owner;ImGui::EndDisabled();
    }else if(state.data.role=="owner"){
        if(state.data.maxDays>0){
            Ui::Text(424,192,392,28,u8"Proprietário",IM_COL32(146,186,117,255));Ui::Wrapped(424,240,392,u8"Você pode gerar licenças e autorizar integrantes. As chaves da equipe são independentes da sua.");
            if(Ui::Button(424,352,392,40,"Abrir equipe",false,true))Switch(state,Page::Team);
        }else{Ui::Wrapped(424,192,392,state.data.problem.c_str());if(Ui::Button(424,352,392,40,"Verificar novamente"))RequestAction(state,"snapshot");}
    }else if(state.data.maxDays<=0){
        Ui::Wrapped(424,192,392,u8"Este PC foi configurado como integrante emissor. O nome abaixo identifica quem usa este PC; não é um membro cadastrado à distância.");
        if(Ui::Button(424,264,392,40,u8"Salvar solicitação",false,false,ICON_FA_DOWNLOAD))RequestAction(state,"export_request",{},true);
        if(Ui::Button(424,320,392,40,u8"Importar autorização",false,true,ICON_FA_FOLDER_OPEN))state.pick=Picker::Authorization;
        Ui::Wrapped(424,392,392,u8"Recebeu um ZIP pronto? Extraia tudo e abra o Admin da pasta extraída. A autorização será reconhecida automaticamente.");
        if(Ui::Button(424,480,392,32,u8"Sou o proprietário: recuperar acesso",true))state.pick=Picker::Owner;
    }else{
        Ui::Text(424,192,392,28,u8"Emissão autorizada",IM_COL32(146,186,117,255));Label(424,244,u8"Autorização válida até");Ui::Text(424,272,392,24,state.data.grantExpiry.c_str());
        auto maximum="Até "+std::to_string(state.data.maxDays)+" dias por licença.";Ui::Wrapped(424,324,392,maximum.c_str());
        if(Ui::Button(424,404,392,40,u8"Emitir licença",false,true))Switch(state,Page::Licenses);
        if(Ui::Button(424,460,392,28,u8"Importar nova autorização",true))state.pick=Picker::Authorization;
    }
}
void SettingsPage(UiState& state){
    Ui::Text(24,136,792,32,u8"Configurações",text,heading);
    Ui::Text(24,184,370,24,"Aparência",text,regular);
    if(Ui::Toggle(24,220,360,"Reduzir movimento",state.reducedMotion)){state.saveSettings=true;Ui::SetReducedMotion(state.reducedMotion);}
    Ui::Wrapped(24,272,360,u8"Mantém os controles responsivos e desativa transições. A preferência fica salva neste perfil.");
    Ui::Text(24,352,360,24,"Diagnóstico",text,regular);
    Ui::Wrapped(24,392,360,u8"Exporte versão e contagens para suporte, sem nomes, IDs, chaves ou licenças.");
    if(Ui::Button(24,456,360,40,"Exportar diagnóstico",false,true,ICON_FA_DOWNLOAD))state.exportDiagnostics=true;
    Ui::Text(424,184,392,24,"Pastas",text,regular);
    if(Ui::Button(424,220,188,36,"Meus dados",false,false,ICON_FA_FOLDER_OPEN))state.openFolder=1;
    if(Ui::Button(624,220,192,36,"Pacotes",false,false,ICON_FA_FOLDER_OPEN))state.openFolder=2;
    if(Ui::Button(424,268,188,36,"Aplicativo",false,false,ICON_FA_DESKTOP))state.openFolder=3;
    if(Ui::Button(624,268,192,36,"Logs do menu",false,false,ICON_FA_FILE_LINES))state.openFolder=4;
    Ui::Wrapped(424,328,392,u8"Projeto: executáveis, fontes, ícones e builds.\nDocumentos: configurações, histórico e dados de uso.");
    Ui::Wrapped(424,408,392,u8"A limpeza não apaga licenças nem backups. Não há limpeza automática de logs ou da pasta Temp do computador.");
    Ui::Text(424,492,392,20,"ZB2 Admin 1.7 · ImGui / x64",muted,caption);
}
void HelpPage(){
    Ui::Text(24,136,792,32,"Pronto para enviar",text,heading);
    Ui::Panel(24,200,380,272);Ui::Panel(424,200,392,272);
    Ui::Text(44,216,340,28,"CLIENTE: um ZIP",text,regular);
    Ui::Wrapped(44,264,340,u8"1. Peça o ID do PC do cliente.\n2. Preencha nome, ID e prazo.\n3. Clique em Gerar ZIP do cliente.\n4. Envie somente esse ZIP.\n\nEle extrai tudo e abre ZB2Menu.exe. A licença do pacote é aplicada automaticamente.");
    Ui::Text(444,216,352,28,"EQUIPE: autorize o PC uma vez",text,regular);
    Ui::Wrapped(444,264,352,u8"1. O integrante envia a solicitação do PC dele.\n2. Você abre em Equipe e define os limites.\n3. Clique em Autorizar e gerar ZIP.\n4. Ele extrai e abre o Admin no PC autorizado.\n\nSó vai jogar? Envie o pacote de cliente.");
    Ui::Wrapped(24,496,670,u8"A opção de copiar/digitar chave continua disponível. Nunca envie sua chave de proprietário.");

}
}
void Draw(UiState& state){
    if(state.revision!=state.data.revision){state.revision=state.data.revision;state.message=state.data.notice;
        if(!state.started&&state.data.initialized){state.started=true;if(state.name[0]==0)strcpy_s(state.name,state.data.user.c_str());if(!state.data.hasKey||state.data.maxDays==0)state.page=Page::Station;}
        if(!state.data.requestText.empty()){state.requestText=state.data.requestText;state.requestName=state.data.requestName;state.requestId=state.data.requestId;}
        if(!state.data.output.empty()){state.exportText=state.data.output;state.exportName=state.data.filename;if(state.saveAfterReply)state.save=true;}
        state.saveAfterReply=false;
        if(state.data.maxDays>0 && Number(state.days)>state.data.maxDays)snprintf(state.days,sizeof(state.days),"%d",state.data.maxDays);
    }
    ImGui::SetNextWindowPos(ImVec2(0,0));ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("ZB2 Admin",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar);
    Ui::Text(24,16,320,36,"ZB2 Admin",text,heading);const std::string identity=state.data.name+(state.data.role=="owner"&&state.data.maxDays>0?u8" · Proprietário":state.data.maxDays>0?" · Integrante":" · Sem autorização");Ui::Text(424,20,304,28,identity.c_str(),muted,caption,2);
    if(Ui::Button(748,20,28,28,ICON_FA_MINUS,true))state.minimize=true;
    if(Ui::Button(788,20,28,28,ICON_FA_XMARK,true))state.close=true;
    const char* tabs[]={"Clientes","Minha equipe","Meu acesso",u8"Configurações","Ajuda"};
    const Page tabPages[]={Page::Licenses,Page::Team,Page::Station,Page::Settings,Page::Help};
    const char* tabIcons[]={ICON_FA_KEY,ICON_FA_USERS,ICON_FA_DESKTOP,ICON_FA_GEAR,ICON_FA_CIRCLE_QUESTION};
    for(int i=0;i<5;++i)if(Ui::Button(24+160.f*static_cast<float>(i),80,152,36,tabs[i],true,state.page==tabPages[i],tabIcons[i]))Switch(state,tabPages[i]);
    state.fade=std::min(1.f,state.fade+ImGui::GetIO().DeltaTime/.12f);ImGui::PushStyleVar(ImGuiStyleVar_Alpha,state.fade);
    if(state.page==Page::Help){
        if(state.credits){
            Ui::Text(24,144,792,32,u8"ZB2 Pro Menu — Créditos & Equipe",text,heading);
            Ui::Text(24,208,792,28,"WeFagundes & Equipe de Modding",text,regular);
            Ui::Text(24,252,792,24,"Unity Mono / DirectX 11 Hook (x64)",muted,regular);
            Ui::Text(24,292,792,24,u8"v1.7-local · Alpha Build",muted,regular);
            Ui::Text(24,332,792,24,state.data.maxDays>0?u8"Emissão autorizada neste PC":u8"Aguardando autorização",text,regular);
            if(Ui::Button(24,432,384,36,"Voltar à ajuda"))state.credits=false;
            if(Ui::Button(420,432,396,36,u8"Dependências",true))ImGui::OpenPopup("Dependencias");
            if(ImGui::BeginPopup("Dependencias")){ImGui::BeginChild("notices",Ui::P(640,240));static const auto notices=AppResources::Read(205);ImGui::TextWrapped("%s",notices.c_str());ImGui::EndChild();ImGui::EndPopup();}
        }
        else{HelpPage();if(Ui::Button(700,492,116,28,"Créditos",true))state.credits=true;}
    }
    else if(state.page==Page::Settings)SettingsPage(state);
    else if(!state.data.initialized){Ui::Text(24,200,792,40,state.data.busy?u8"Verificando estação...":u8"Não foi possível abrir a estação",text,heading,1);Ui::Wrapped(100,276,640,state.message.c_str());if(!state.data.busy&&Ui::Button(270,388,300,40,"Verificar novamente"))RequestAction(state,"snapshot");}
    else{ImGui::BeginDisabled(state.data.busy);switch(state.page){case Page::Licenses:LicensePage(state);break;case Page::Team:TeamPage(state);break;case Page::Station:StationPage(state);break;default:break;}ImGui::EndDisabled();}
    ImGui::PopStyleVar();ImGui::GetWindowDrawList()->AddLine(Ui::P(24,540),Ui::P(816,540),IM_COL32(49,49,55,255));
    const std::string message=state.data.busy?"Processando...":state.message.empty()?"1.7  |  Extraia o ZIP antes de executar":state.message;
    ImGui::GetWindowDrawList()->PushClipRect(Ui::P(24,548),Ui::P(state.exportText.empty()?816.f:600.f,588),true);Ui::Text(24,548,576,28,message.c_str(),state.data.error?IM_COL32(231,153,151,255):muted,caption);ImGui::GetWindowDrawList()->PopClipRect();
    ImGui::SetCursorPos(Ui::P(24,548));ImGui::InvisibleButton("##notice",Ui::P(state.exportText.empty()?792.f:576.f,32));Ui::Hint(message.c_str());
    if(!state.exportText.empty()){
        if(Ui::Button(612,548,92,32,"Copiar"))Copy(state,state.exportText);
        if(Ui::Button(716,548,100,32,"Salvar",false,true))state.save=true;
    }
    ImGui::End();
}
}
