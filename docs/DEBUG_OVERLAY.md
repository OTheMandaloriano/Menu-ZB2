# Debug overlay

O overlay agora mostra:

- `LOCAL HP` e `STAM`: jogador local.
- `ALIADOS`: outros jogadores presentes na lista local de jogadores.
- `HP1`: HP do primeiro aliado vivo disponível; `0` quando não há aliado vivo.
- `ZUMBIS` e `hp0`: contagem e HP do primeiro zumbi vivo lido.
- `HOST`: `LobbyPlayer.playerName` do host. O jogo não expõe um título personalizado da sala nessa API; esse é o nome de exibição correspondente.
- `SALA ID`: `MultiplayerController.GetLobbyCode()`, o ID Steam da sala.
- `LOBBY`, `SINGLE`, `CLIENTE` ou `HOST`: estado atualizado mesmo quando os modificadores estão desligados.

Dados do lobby são atualizados uma vez por segundo quando a cena de partida está pronta. O modo pode atualizar no lobby carregado. Dados de HP, contagem de jogadores e zumbis atualizam no ciclo de leitura existente, inclusive com o overlay de debug ligado e os modificadores desligados. Se o nome ou código ainda não estiver disponível, o campo mostra `---`.

`ALIADOS` conta jogadores não locais encontrados em `PlayersController.players`; o jogo é cooperativo PvE nesta cadeia. `HP1` não é média nem HP total do grupo. Valores ausentes são zerados a cada leitura, para que a saída/ morte de um colega não deixe HP antigo na tela.
