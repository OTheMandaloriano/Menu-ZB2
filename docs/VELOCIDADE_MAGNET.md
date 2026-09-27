# Velocidade normal e atração contínua

NoClip e Speed Hack iniciam em 1×. NoClip usa `PlayerMovement.walkSpeed` original, não `NoClip.speed` de debug (10 na definição). O registro de originais fornece a base mesmo quando Speed Hack está ativo. O multiplicador de NoClip é independente e não acumula. Segurar Shift aplica boost temporário de 4×; soltar retorna ao multiplicador escolhido sem alterar a base. Desativar restaura a velocidade e a física anteriores do componente NoClip.

Mensagens permanentes de ativação foram retiradas. Permanecem mensagens acionáveis: restauração de inventário pendente, impossibilidade de ativar dentro de obstáculo, velocidade indisponível ou falta de autoridade do Magnet.

## Referência wohax

Foram lidos os arquivos públicos `block/WFX/MAG3/magnetic.lua` e `block/WFX/4Jhonny/MAGNETIC.LUA` de https://github.com/ioisaque/wohax. `Vac` escolhe posição associada ao ator ou posição guardada e chama `SetVacPos`; `VacHandler` filtra entidades vivas, equipe e opcionalmente visibilidade. O comportamento nativo de `SetVacPos` não é definido nesses trechos Lua; não é possível inferir deles a sincronização com um servidor Unity.

A adaptação usa ponto compacto à frente do personagem, acompanha seu movimento e volta a reunir zumbis que se afastarem. Pequenos deslocamentos estáveis evitam coordenadas idênticas entre corpos. Mantém limite de quatro movimentos por 100 ms, cursor rotativo, verificação de chão, espaço livre e distância. Não usa anéis crescentes nem impede reposicionamento depois do primeiro uso.

## Modos de jogo

NoClip controla somente o jogador local e não possui bloqueio por modo. Funcionamento aceito pelo servidor no modo cliente continua sujeito a validação em partida.

O Magnet usa `Zombie.TeleportTo`, que transmite `BroadcastZombieTeleport` quando `IsOnlineServer` é verdadeiro. Na build auditada, `ServerListener.TreatZombieTeleport` chama `LogUnexpectedPackage`, portanto o servidor não aceita esse comando vindo de um cliente comum. O Magnet mantém solo/host; não promete mover a sala como cliente nem simula sucesso exclusivamente local. Suporte cooperativo a pedidos de clientes exigiria uma implementação autorizada também no host, ausente nesta revisão.
