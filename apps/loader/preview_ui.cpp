#include "preview_ui.h"
#include "../../imgui/imgui.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

namespace {
int page=0;
void Label(const char* title,const char* value){
    ImGui::TextDisabled("%s",title);ImGui::SameLine(145);ImGui::TextWrapped("%s",value);ImGui::Spacing();
}
void Banner(const char* title,const char* body){
    ImGui::TextColored(ImVec4(.38f,.81f,.76f,1),"%s",title);
    ImGui::Spacing();ImGui::TextWrapped("%s",body);ImGui::Spacing();
}
void Activation(){
    Banner("Ativacao offline", "Uma licenca assinada permite verificar emissor, produto e validade sem guardar sua chave privada no cliente.");
    Label("ETAPA ATUAL","Previa visual - nenhuma licenca e validada nesta versao");
    ImGui::BeginDisabled();
    ImGui::Button("Gerar solicitacao de ativacao",ImVec2(-1,40));
    ImGui::Button("Importar licenca assinada",ImVec2(-1,40));
    ImGui::EndDisabled();ImGui::Spacing();
    ImGui::TextWrapped("O emissor sera uma ferramenta separada, usada somente pelo proprietario. Nao existe chave de teste que libere o menu aqui.");
    ImGui::Spacing();
    if(ImGui::Button("Ver demonstracao do painel",ImVec2(-1,40)))page=1;
}
void Dashboard(){
    Banner("Seu menu. Uma entrada.","Tela de demonstracao. O estado do jogo e a licenca nao sao consultados por esta previa.");
    ImGui::BeginChild("summary",ImVec2(0,190),true);
    Label("PRODUTO","ZB2 Menu");Label("LICENCA","Nao validada - integracao pendente");
    Label("VALIDADE","Disponivel apos ativacao assinada");Label("JOGO","Nao consultado nesta previa");
    ImGui::EndChild();ImGui::Spacing();
    ImGui::BeginDisabled();ImGui::Button("Carregar menu",ImVec2(-1,44));ImGui::EndDisabled();
    ImGui::TextDisabled("Carregamento indisponivel no prototipo visual.");
}
void Package(){
    Banner("Distribuicao organizada","Voce entrega um executavel. Os componentes internos continuam separados por responsabilidade.");
    auto resource=FindResourceW(nullptr,MAKEINTRESOURCEW(101),MAKEINTRESOURCEW(10));
    DWORD size=resource?SizeofResource(nullptr,resource):0;
    if(size)ImGui::Text("Pacote interno: %.2f MB",size/(1024.f*1024.f));
    else ImGui::TextDisabled("Pacote nao incorporado neste render de teste");
    Label("FORMATO","ZIP deterministico + manifesto SHA-256");
    Label("AUTENTICIDADE","Nao assinado - desenvolvimento");
    Label("DESTINO PROPOSTO","Documentos/ZB2Menu/runtime/<versao>");
    ImGui::TextWrapped("Nenhum arquivo e instalado ou extraido por esta previa. Atualizacoes automaticas e verificacao de assinatura entram nas proximas etapas.");
    ImGui::Spacing();ImGui::TextWrapped("PDB e MAP ficam no ambiente privado de diagnostico. Licencas de terceiros continuam incluidas no pacote.");
}
}

void DrawLoaderPreview(){
    auto& io=ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0,0));ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("ZB2 Menu",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);
    ImGui::TextColored(ImVec4(.38f,.81f,.76f,1),"ZB2");ImGui::SameLine();ImGui::TextUnformatted("MENU");
    ImGui::SameLine(ImGui::GetWindowWidth()-175);ImGui::TextDisabled("PREVIA 0.1 | OFFLINE");
    ImGui::Spacing();ImGui::Separator();ImGui::Spacing();
    ImGui::BeginChild("nav",ImVec2(145,-28));
    const char* tabs[]={"Ativacao","Inicio","Pacote"};
    for(int i=0;i<3;++i){if(ImGui::Selectable(tabs[i],page==i,0,ImVec2(0,38)))page=i;}
    ImGui::Spacing();ImGui::TextWrapped("Ambiente de teste local");ImGui::EndChild();ImGui::SameLine();
    ImGui::BeginChild("content",ImVec2(0,-28));
    if(page==0)Activation();else if(page==1)Dashboard();else Package();
    ImGui::EndChild();ImGui::Separator();ImGui::TextDisabled("Prototipo visual | Sem rede, ativacao real ou injecao");
    ImGui::End();
}
