# UTILITIES-11

Implementação dos controles antes desabilitados da aba MISC, baseada nos métodos
auditados com dnSpy MCP. Ações executam na thread Unity por requisição numerada,
uma vez por clique; o render apenas publica configurações e recebe snapshots.

- Salvar posição copia XYZ do jogador. Teleportar procura chão e cápsula livre
  perto do destino. Bancadas selecionam a mais próxima da categoria; braseiro
  ignora acesos; mercador exige van existente. Não move jogadores remotos.
- Aplicar hora altera curTime uma vez. Velocidade do dia altera duração do ciclo,
  sem modificar Time.timeScale. Exige solo/host e restaura a duração ao desligar.
- Criação de zumbis usa fila de um por 0,25 s, com cancelamento, chão e espaço
  verificados. O tipo inicial é o civil comum; a contagem é de tentativas, pois
  o jogo pode recusar locais perto de boss. Máximo de 256 modelos ativos.
- Boss permite Riot, Queen e Reaper. Recusa tipo já existente e local bloqueado;
  utiliza SpawnBoss e sincronização nativa quando host online. Quantidade não
  cria múltiplos bosses por clique. Não altera regras de progressão/recompensa.
- FOV personalizado e terceira pessoa usam campos/modo nativos e restauram
  seus valores anteriores. Distância continua sujeita à colisão da câmera.
- Executar em segundo plano controla Application.runInBackground. Não simula
  teclas, não impede regras de AFK do servidor e não é anunciado como Anti-AFK.
- Sem dano de queda intercepta somente DamageType.Fall do jogador local, com
  heartbeat para não permanecer preso após interrupção do runtime.
- Levantar quando caído chama Revive apenas no estado Dying. Não ressuscita
  mortos nem reinicia partida. Sua aceitação em cliente requer teste real.

Criação, mudança de hora e teleportes são ações: desligar não desfaz seus efeitos.
Modificadores de câmera/tempo/background são restaurados. Solicitações em loading
são consumidas sem reprodução posterior. A rotina de utilidades verifica mapa e
jogador diretamente para permitir o comando de levantar quando HP é zero.

23 testes com Harmony verificaram restauração, autoridade, queda, teleportes,
pedido único, fila/throttle/cancelamento, boss duplicado e loading. Builds x64 e
gerenciado e regressões de mira/overlay passaram. Física de destinos concretos,
câmera durante ADS e sincronização de criação precisam de validação em partida.
