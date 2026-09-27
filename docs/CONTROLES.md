# Atalhos e Magnet

## NoClip

N alterna o NoClip por padrão. A tecla pode ser escolhida no controle NoClip ou em SETTINGS e é salva no preset. Um pressionamento gera uma alternância; segurar a tecla não alterna repetidamente. Menu aberto, captura de atalho, chat, console, pausa e perda de foco bloqueiam atalhos. INSERT/DELETE ficam reservados ao menu; quando há conflito, o atalho conflitante não é executado. O botão de captura ignora as teclas já pressionadas ao abri-lo.

## Magnet

H alterna por padrão e também é configurável. Reúne zumbis vivos **carregados localmente**, dentro do raio configurado, em torno de um ponto compacto que acompanha o jogador. A atração é contínua enquanto ativada, sem repetir teleporte se o alvo já estiver perto do ponto. Usa `Zombie.TeleportTo`, que já contém a sincronização do host. Há limite de quatro movimentos por passagem de 100 ms, pontos separados, verificação de chão e rejeição de destino ocupado.

Requer solo ou host. Cliente sem autoridade exibe uma mensagem e não altera posições. Não arrasta jogadores, não cria zumbis descarregados, não congela a IA e não mata inimigos. Desligar interrompe a reunião; as posições já aplicadas não são revertidas, pois os inimigos continuam simulando e se movendo no jogo.

## Verificações

Build contra a instalação auditada, regressões de atalhos/presets e simulação do Magnet (autoridade, lotes, chão, bloqueios, morte e repetição). O comportamento em partida ainda precisa de teste pelo usuário.

Veja [velocidade normal e auditoria da referência](VELOCIDADE_MAGNET.md).
