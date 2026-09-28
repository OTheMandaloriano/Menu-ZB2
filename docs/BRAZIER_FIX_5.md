# BRAZIER-FIX-5 — chegar ao alcance da interação

As capturas mostraram duas recusas: 24 tentativas sem chão, ou 20 sem chão e
quatro bloqueadas por Cube (1). Não bastava aumentar o raio do teleporte.

O preparador de colisões só comparava o pivot do LOD com o destino. Uma mesh
grande pode cobrir o destino com seu pivot fora desse raio. Agora MeshCollider
usa os bounds locais de sharedMesh e BoxCollider usa center/size, transformando
os oito cantos para o mundo. Isso funciona mesmo quando Collider.bounds está
vazio por estar desativado. LODs com subobjetos são examinados pelos colliders
do subobjeto. Bounds vazios de outros tipos não são tratados como área válida
na origem do mapa. O limite de 256 alterações por clique continua preservado.

A busca anterior também aceitava destinos fora dos 1,5 m exigidos pelo jogo.
A nova busca é centrada em InteractionPoint: quatro raios entre 0,65 e 1,40 m,
24 direções por raio. A altura final usa defaultHeight. O candidato deve ter
chão, cápsula livre e distância 3D menor que 1,45 m até a interação. A chegada
orienta o personagem em direção ao braseiro. Não acende automaticamente.

Falhas continuam restaurando colisões temporárias e preservando a posição do
jogador. Não existe fallback para dentro de paredes ou para o vazio. LastProbe
retém contagens de chão/obstáculos/alcance, LODs ativados e máscara de colisão
para leitura de diagnóstico sem controlar a tela.

18 testes de navegação passaram, incluindo piso desativado com pivot remoto,
restauração de LOD, passagem entre as antigas direções de 45 graus, alcance
de interação e orientação. Build contra o assembly auditado. Os testes usam
física simulada; os destinos concretos das capturas precisam de validação em
partida antes de declarar a correção completa.
