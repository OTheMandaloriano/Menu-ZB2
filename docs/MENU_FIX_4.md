# MENU-FIX-4

Há um único God Mode. Seu rótulo/tooltip deixam de descrever o reparo antigo
em 100 HP. Os identificadores GOD-SIL-2/MAGNET-SIL-3 eram revisões do pacote.

## Controles

- Área de mira escolhe Círculo FOV ou 360 graus. A política de seleção deriva
  dessa escolha, sem um segundo checkbox de limite. Mostrar círculo e cor são
  preferências visuais separadas. Em 360 o círculo fica oculto por não representar
  a seleção. FOV seleciona um alvo elegível; não distribui dano pela área.
- Pontos/Mundo mantém as categorias principais, distância, box, linhas e raio.
  A lista duplicada de tipos e a nota sobre Equipe foram retiradas. O campo
  legado de máscara ainda pode existir em presets, mas não é aplicado na coleta;
  isso evita filtros antigos invisíveis ocultando categorias marcadas.

## Magnet

O congelamento anterior suspendia atualizações, mas não reafirmava a posição e
ignorava chamadas Hold posteriores para um zumbi já retido. Agora cada entrada
guarda destino e orientação. Pulse e os hooks reaplicam a âncora, mantém o corpo
cinemático e desativam root motion temporariamente. A substituição do corpo ou
animador reaplica a propriedade de controle à nova instância. Desligar restaura
cinemática, velocidade da animação e root motion, sem restaurar a posição anterior
ao teleporte. Não desliga colliders usados para receber tiros.

Os testes cobrem deslocamento externo, corpo substituído, seguir com destino
comum e restauração. A retenção em partidas longas, hordas e bosses ainda precisa
de confirmação prática. Os limites de carregamento, nascimento e autoridade
continuam valendo; não significa que todo zumbi do mapa já esteja carregado.

## Teleporte

A busca usa `player.movement.groundMask`, como o chão do personagem, em vez de
considerar todas as camadas. O ponto inicial usa a base do braseiro para reduzir
a chance de encontrar teto acima da interação. A cápsula de chegada continua
obrigatória. A recusa informa tentativas sem chão, tentativas bloqueadas e nome
do collider bloqueador. Não há fallback que teleporta para dentro de obstáculos.
Essa alteração ainda não comprova que todos os destinos anteriormente recusados
ficaram acessíveis; o diagnóstico da partida deve orientar qualquer correção
adicional.

## Verificação

40 testes de Magnet, 13 de teleporte e 41 de overlay/presets, incluindo FOV sem
o checkbox legado, ocultação em 360 e persistência da cor. A verificação da
fronteira entre threads aceita explicitamente as duas políticas derivadas
(FOV e máscara legada de pontos), sem liberar leituras Config na thread Unity.

O watchdog do Magnet conta frames sem heartbeat, em vez de liberar a multidão
por uma única pausa de mais de meio segundo. Isso evita a restauração física
em um frame lento; 60 frames sem atualização ainda liberam a propriedade.
