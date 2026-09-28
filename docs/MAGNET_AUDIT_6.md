# MAGNET-AUDIT-6

## Achados

A captura relatou ESP de 96,1 ms. Os logs posteriores disponíveis estavam em
outra condição, próximos de 1 ms; não comprovam reprodução da mesma carga.
Foram encontrados custos redundantes e falhas de posicionamento:

- A origem do ZombieObject era posta no chão sem considerar zombieFootRef.
- A busca de chão aceitava ZombieDoll/cadáveres e o modo distribuído usava
  ainda um caminho de raycast diferente do sobreposto.
- A presença em placed impedia recolher um distribuído que andasse para longe.
- A consulta de mira com 64 colisões podia saturar em grupos densos e recusar
  a visibilidade. Usava todas as camadas em vez de PlayerArms.shotLayerMask.
- A máscara da silhueta era reconstruída no Update e novamente no pré-render.

## Alterações

O ponto de apoio é convertido para a origem do modelo pelo deslocamento real
dos pés. A busca usa groundMask do jogador e ignora jogadores, zumbis e dolls.
Sobrepostos reutilizam o chão da âncora e o teste de espaço do grupo por passagem.
Distribuídos que saem da posição são elegíveis novamente; slots de mortos são
removidos periodicamente. Há orçamento de tentativas e de varredura por passagem.

Com NoClip efetivamente ativo, Seguir e Fixar à frente usam um ponto diante da
câmera e alinham zombieEyeRef a ele. Os corpos são retidos no ar; um obstáculo
entre câmera e destino impede a colocação. Fixar pela mira mantém a exigência
de um ponto de chão. Mudanças de modo/NoClip invalidam a âncora anterior.

Pin evita reescrever pose, cinemática e animação quando não mudaram. Pulse reutiliza
a lista de trabalho. A silhueta só monta seus comandos no pré-render. LastMilliseconds
e PeakMilliseconds permitem medir o custo do Magnet ativo em memória.

Após mudanças de pose, SyncTransforms é chamado uma vez ao final da passagem,
antes das consultas seguintes de mira, para não usar colliders na posição antiga.

A mira usa a máscara real dos disparos e repete uma consulta saturada com buffers
de 256 e, no máximo, 1024 colisões. Persistindo a saturação, continua recusando.
Outro zumbi vivo retido na mesma âncora pode representar o ponto compartilhado;
uma parede não recebe essa exceção. O disparo original continua decidindo impacto,
penetração e dano; a mudança não causa dano em área nem atravessa paredes.

FOV e 360 voltam a checkboxes separados com estado mutuamente exclusivo.
Mostrar círculo e a cor continuam sendo controles visuais próprios.

## Verificação e limites

44 testes de Magnet, incluindo pés deslocados, cadáver sobre o chão, posição
pela câmera, recolher novamente e identidade da âncora. 30 regressões Harmony
de mira, incluindo grupo com 100 colisões e bloqueio por parede/saturação.
Testes de ciclo de vida da silhueta e modificadores também passaram.

Isso não comprova desempenho com a horda da captura nem alinhamento de todo rig
de boss. Precisam de validação em partida após instalar a revisão. Entidades
não carregadas e autoridade solo/host continuam sendo limitações do Magnet.
