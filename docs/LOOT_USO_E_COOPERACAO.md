# Uso do Item Magnet e limites cooperativos

## Item Magnet

A tecla configurada agora alterna ligado/desligado; não exige manter pressionada. O checkbox mantém o mesmo estado. X é o padrão de novos presets; um preset antigo pode continuar usando J. A chave não é sobrescrita silenciosamente. Conflitos com as teclas do menu, NoClip e Magnet de inimigos suspendem o atalho conflitante.

O modal explica a sequência, mostra a quantidade selecionada e aceita clique na imagem ou no checkbox. **Todos permitidos** marca a lista e coloca a categoria em Todos, evitando que uma categoria anterior exclua silenciosamente itens selecionados. Bloqueados, ocultos, reservados e sacos continuam excluídos.

Mapa inteiro mantém cursor incremental e lotes; não significa criar itens que não existem na sessão. O posicionamento usa `Renderer.localBounds` transformado para o mundo, em vez de confiar em bounds de mundo potencialmente antigos em modelos desativados pelo LOD. Sem geometria ou chão válidos, o item permanece no lugar. Contadores mostram total movido, voltas da varredura e tentativas sem chão/modelo; contagens de falha representam tentativas, não quantidades únicas de itens.

## Amigo sem menu no próprio computador

O usuário confirmou que o amigo concorda, mas somente seu PC usará o menu. Nesse cenário, não foi identificado um comando nativo suportado para executar a movimentação no jogador local do amigo. Os caminhos auditados de posição alimentam representações remotas; o jogador local processa seu próprio movimento e publica sua posição. Um teleporte exclusivamente local poderia parecer funcionar por um instante e ser corrigido pela rede.

O mesmo vale para o HP real: o caminho auditado sincroniza estado vivo/caído/morto, não o valor numérico local. A exibição permanece honesta sobre a indisponibilidade, sem inventar percentual. Não houve tentativa de enviar comandos pelo chat, alterar o amigo ou substituir a identidade de outro cliente durante esta investigação.

Magnet de amigos com confirmação de execução e HP real entre participantes exigem uma solução cooperativa com suporte no lado do amigo, ou outro mecanismo nativo ainda não demonstrado. Não foram implementados nesta revisão. Os testes locais não podem provar esses comportamentos remotos.

## Validação

Build contra o assembly auditado, testes de clique/preset do renderizador e varredura de loot, incluindo mais entradas do que o orçamento de uma passagem, itens a quilômetros e modelo distante com posição local corrigida. A hipótese de LOD e os motivos concretos de exclusão ainda precisam de leitura dos contadores na partida. Nenhuma interface foi controlada automaticamente.
