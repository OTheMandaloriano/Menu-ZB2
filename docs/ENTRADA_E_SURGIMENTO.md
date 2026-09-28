# Entrada do menu e congelamento após surgimento

## Mouse e comandos

A fila de entrada anterior consumia todo teclado/mouse enquanto o menu estava aberto, enquanto o jogo continuava gerenciando `Cursor.lockState`. O código também escondia o cursor pelo contador Win32 e impunha uma área de confinamento ao fechar. Essa combinação podia bloquear navegação e não preservar o estado do menu do jogo.

Agora eventos continuam chegando ao ImGui pelo renderizador, mas o WndProc só impede propagação quando mouse/teclado são capturados pela interface, além da tecla de alternar o menu. Ao abrir, o cursor Unity é liberado; o ponteiro é desenhado pelo ImGui. Ao fechar ou perder foco, o estado anterior de lock/visibilidade é restaurado e o controlador do próprio jogo volta a decidir.

O leitor de ações de gameplay é limpo enquanto nosso menu está aberto, para não mirar/atirar/andar ao editar controles. O módulo de eventos da UI do jogo é suspenso somente enquanto nossa interface captura entrada, evitando clique atravessando a janela. Fora dela, a UI do jogo pode receber interação. Fechar o overlay devolve os comandos. Não se abre o menu do jogo artificialmente, não se altera o contador global ShowCursor nem se força um confinamento novo ao fechar.

A atualização do cursor usa uma leitura atômica de flags nativas em um callback gerenciado separado do trabalho de jogo. Continua sendo atualizada com `Time.timeScale=0`, enquanto modificadores e leituras da simulação permanecem suspensos. Present não invoca Unity.

## Surgimento de inimigos

`ZombieState.Spawning` e a transição para esse estado não podem ser congelados nem teleportados pelo Magnet. A animação e os métodos originais continuam até o estado mudar. Na próxima passagem elegível, o Magnet pode posicionar o inimigo e congelá-lo.

O hook de congelamento também revalida esse estado. Se uma entidade já congelada voltar a surgir, libera física/animação antes de executar o método original. Desligar continua restaurando os valores guardados.

## Verificação

Testes Harmony: cursor originalmente travado e originalmente livre; abrir/fechar; captura impede cliques na UI do jogo; perda de foco; callback de cursor durante pausa; animação de surgimento não suspensa; congelamento somente após surgir; liberação se voltar ao estado Spawning. Testes nativos verificam roteamento e ordem da fila. A validação de navegação e animação na partida ainda é necessária.
