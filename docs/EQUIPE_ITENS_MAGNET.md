# Equipe, itens e áreas de loot

## Origem da confusão

A tag genérica `Jogador` vinha de `InterestPoint.Type.OtherPlayerDot`, e `Armas brancas` vinha de `InterestPoint.Type.Melee`. São pontos do mapa, não identificação do objeto visível. Uma área de loot pode ter posição e conteúdo diferentes do item mais próximo. A imagem não prova qual item estava no chão.

O ESP de equipe agora consulta `PlayersController.players`, exclui o jogador local e mortos, usa o nome de `lobbyPlayer`. A revisão de saúde remota substitui HP numérico não sincronizado pelo estado vivo/caído. Box 2D vem da projeção do hitbox real. Nome, distância, HP, linha, cor e raio são independentes. Não usa marcadores do mapa nem exige Itens/Pontos ligados. Esqueleto e box 3D de aliados não foram implementados nem apresentados como disponíveis.

Itens usam nome traduzido do banco e centro combinado dos renderers ativos, com fallback para a posição do objeto quando o modelo não existe. Textos são centralizados sobre a âncora, com ligação visual curta. Pontos de loot se apresentam como `Área: ...`, para não fingir identificação exata de um item.

Referência da ancoragem: https://docs.unity3d.com/kr/6000.0/ScriptReference/Renderer-bounds.html. A Unity define bounds em espaço de mundo e recomenda seu centro como aproximação visual melhor que o pivô em objetos assimétricos.

## Magnet

Filtro: zumbis e bosses, somente zumbis ou somente bosses. Zumbis comuns respeitam o raio de origem. Bosses existentes ignoram esse raio. Se estiverem nas listas Prop/Unloaded, o método do próprio jogo converte no máximo um boss por passagem, fora da enumeração que modifica a lista.

Não cria bosses que ainda não existem nem inventa dados ausentes da sessão. Mantém destinos com chão/espaço livre e lotes limitados. A autoridade continua solo/host; não altera jogadores de equipe. A reunião contínua por ponto compacto permanece como adaptação da referência wohax.

## Validação

Compilação nativa e gerenciada contra o jogo auditado. Testes de coleta: marcador genérico de mapa não entra como equipe, exclusão local/morto, nome real, ABI e centro de renderer. Testes ImGui: independência de mestres e elementos. Magnet: boss existente a 900 m com raio de 10 m, conversão de descarregado e filtro de zumbis. Validação visual e de física em partida ainda pendente.

A saúde remota não fornece percentual confiável nesta build. Veja [auditoria de rede](MAPA_INTEIRO_E_SAUDE.md).
