# Loader nativo

Prévia visual 0.2, com a direção compacta aprovada pelo usuário. O executável
não instala componentes, não valida licenças e não injeta o menu nesta etapa.

| Arquivo | Responsabilidade |
|---|---|
| main.cpp | Entrada do executável |
| window.cpp/h | Janela, mensagens, arraste, minimizar/fechar e DPI |
| graphics.cpp/h | Recursos D3D11, redimensionamento e apresentação; libera recursos COM |
| preview_ui.cpp/h | Estado de prévia e telas, sem chamadas de rede, disco ou processo |
| preview_theme.cpp/h | Paleta, medidas, fontes e escala |

O empacotador fica em scripts/package_runtime.py; não faz parte da lógica das
telas. Licenciamento e atualização deverão ter módulos próprios conforme o plano
aprovado, sem colocar chave privada, gerador de licença ou token GitHub no cliente.
Esses módulos ainda não foram implementados, portanto não há diretórios vazios
ou classes fictícias fingindo fornecer essas funções.

As referências de Downloads/Loaders foram lidas estaticamente. Não foram copiadas
autenticações locais, SDKs antigos, fontes ou imagens desses arquivos. Esta prévia
usa a dependência Dear ImGui existente e as fontes Segoe do Windows. Os ícones
do protótipo são da família Segoe MDL2 instalada; não se afirma usar FA6.

## Fluxo

Janela 420 × 460 unidades lógicas, sem sidebar. Ativação aceita digitação para
avaliar o campo; o botão sempre informa que a ativação real não está disponível.
O link de demonstração abre o painel com valores explicitamente não validados.
Carregar apenas mostra feedback de prévia. Nenhuma entrada libera acesso.
O painel retorna à ativação. Controles de janela publicam ações para window.cpp.

O tratamento de DPI recria o atlas apenas na mudança de escala e invalida a
textura do backend. A prévia não faz downloads de fontes. Sem disponibilidade
de fonte/ícones, utiliza fallback; não redistribui arquivos do Windows.

## Verificação

Renderização real do ImGui em 100% e 200%, com imagens revisadas; cliques sintéticos
no contexto de teste verificam navegação, feedback e pedidos de minimizar/fechar.
Não há automação de tela do usuário. Build Release x64 confirmado separadamente.
Arraste nativo, troca real entre monitores, acessibilidade assistiva e execução
em outro PC ainda precisam de validação. Isso não é uma certificação completa
de acessibilidade ou da futura autenticação.
