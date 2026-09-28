# Revisão MAGNET-SIL-3

- O ESP de braseiros omite os acesos imediatamente. A lista de navegação mantém
  índices estáveis e continua informando o estado do destino.
- O teleporte prepara os LODs de colisão até 16 m do alvo (máximo 256 alterações
  por clique) e sincroniza transforms antes de verificar chão. Falhas restauram
  os LODs que estavam desligados. No sucesso, o jogador passa a estar próximo e
  os LODs ativados ficam sob a gestão normal de proximidade do jogo.
- A busca usa três distâncias, oito direções e a altura real do personagem.
  O destino corresponde ao centro do personagem, mantendo seus pés acima do
  chão. Obstáculos continuam sendo motivo de recusa; não há teleporte cego.
- A silhueta fixa viewport e matrizes view/projection explicitamente para
  a orientação da textura Unity na composição D3D11, em vez de herdar a projeção ao trocar render targets.
  A correção de alinhamento ainda exige confirmação visual em partida.

## Magnet

**Agrupamento → Distribuído** preserva as posições separadas anteriores.
**Sobreposto (mesmo ponto)** usa um destino compartilhado por todos os atraídos,
incluindo bosses. Nesse modo a distância frontal determina o ponto; a distância
separada de bosses não se aplica. Os corpos ficam retidos para não se dispersarem,
mas seus colliders de acerto não são desligados. Verificações de terreno ignoram
corpos de zumbis, impedindo que o próximo atraído seja colocado sobre a cabeça
do anterior; paredes e outros obstáculos continuam sendo verificados.

O agrupamento combina com os três destinos: seguir, fixar pela mira ou fixar à
frente. Movimentos seguem limitados em lotes (16 por passagem de 100 ms quando
sobrepostos), portanto hordas grandes não são movidas todas no mesmo frame.
Desligar restaura física/animação. A animação de nascimento continua protegida.
O modo não cria inimigos, não modifica penetração de projéteis nem distribui
dano automaticamente entre todos os alvos. Solo/host continuam sendo necessários.

## Validação

36 verificações de Magnet, 13 de navegação dos braseiros, 44 de ESP de mundo e
19 de ciclo de vida do efeito visual. Builds contra o assembly auditado.
Testes simulados não comprovam alinhamento visual, streaming de uma construção
específica ou o comportamento de todos os colliders na partida.

A máscara é atualizada também no callback de pré-render da câmera, com a matriz
Unity não convertida duas vezes. A composição inverte o eixo V da RenderTexture
para a convenção de UV do ImGui. Referência: https://docs.unity3d.com/ScriptReference/Rendering.CommandBuffer.SetViewProjectionMatrices.html
