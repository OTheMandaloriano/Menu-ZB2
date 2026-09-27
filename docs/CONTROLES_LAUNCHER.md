# Launcher e controles, prévia 0.2.1

## Interface

O arquivo `cylone.rar` fornecido pelo usuário foi lido como referência de disposição e cores. Não foram executados seus binários nem incorporados seu framework, marcas, fontes ou imagens. A nova interface mantém a implementação existente de pacotes/atualizações e usa desenho próprio: navegação lateral, cartão de produto, status e páginas Início, Atualizações e Preferências.

O modo de prévia `ZB2Menu.exe --preview caminho.png` renderiza os controles em imagens locais sem mostrar uma janela, modificar configurações ou injetar o menu. Não captura a tela do usuário. O novo executável também verifica o pacote embutido quando uma versão local anterior já existe, permitindo atualização offline com o jogo fechado.

## NoClip

F6 alterna o NoClip por padrão. A tecla pode ser escolhida no controle NoClip ou em SETTINGS e é salva no preset. Um pressionamento gera uma alternância; segurar a tecla não alterna repetidamente. Menu aberto, captura de atalho, chat, console, pausa e perda de foco bloqueiam atalhos. INSERT/DELETE ficam reservados ao menu; quando há conflito, o atalho conflitante não é executado. O botão de captura ignora as teclas já pressionadas ao abri-lo.

## Magnet

H alterna por padrão e também é configurável. Reúne zumbis vivos **carregados localmente**, dentro do raio configurado, em pequenos grupos em frente ao jogador. Cada ativação move uma vez cada ID elegível. Usa `Zombie.TeleportTo`, que já contém a sincronização do host. Há limite de quatro movimentos por passagem de 100 ms, pontos separados, verificação de chão e rejeição de destino ocupado.

Requer solo ou host. Cliente sem autoridade exibe uma mensagem e não altera posições. Não arrasta jogadores, não cria zumbis descarregados, não congela a IA e não mata inimigos. Desligar interrompe a reunião; as posições já aplicadas não são revertidas, pois os inimigos continuam simulando e se movendo no jogo.

## Verificações

Build contra a instalação auditada, regressões de atalhos/presets e simulação do Magnet (autoridade, lotes, chão, bloqueios, morte e repetição). Prévia visual gerada fora da tela. O comportamento em partida ainda precisa de teste pelo usuário.
