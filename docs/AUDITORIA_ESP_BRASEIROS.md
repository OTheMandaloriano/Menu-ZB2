# Auditoria de memória, ESP e braseiros

## Evidência de memória (CE MCP + dnSpy)

O CE MCP 12.2 respondeu e as leituras foram feitas sem alteração de HP,
posição ou comandos de rede. Na primeira partida havia um jogador com
133,3 HP, 21 zumbis e 56 entradas em `allWorkbenches`. Os offsets Mono
de `ZombieHealth.amount` e `max` foram confirmados como 32 e 16.

Na sala seguinte foram encontrados quatro jogadores. Três registros
remotos continham `healthFast=50` e `healthSlow=75`, inclusive um caído
(`healthState=1`). O registro local tinha 133,3 nos dois campos. Assim,
ler esses valores pela memória não fornece a vida real do aliado.
O suporte a barra/% numérica de aliados permanece pendente.

`Zombie.TakeDamage` no cliente transmite `SendZombieDamage`; quem desconta
`health.amount` é o caminho autoritativo. O broadcast numérico presente nesse
método é específico para bosses. `GenericListener.OnBossHealth` recebe o
float e chama `ZombieNetSyncs.SyncBosshealth`. Por isso a cópia local de um
zumbi comum pode permanecer em 80/80 apesar do dano no servidor.

O snapshot agora marca HP numérico indisponível para zumbi comum quando
o menu identifica modo cliente. Evita mostrar 100% fictício, sem criar
estimativa de dano. Solo/host continuam mostrando amount/max; bosses
mantêm a rota numérica. Não houve reprodução de dano parcial em solo/host
nesta sessão; não se declara esse teste ao vivo concluído.

## Braseiros

Antes o ESP usava somente `InterestPointController.points`, que depende de
descoberta. `WorkbenchInteractions.OnPyreLit` ainda remove o ponto ao acender.
O adaptador agora lê a coleção completa `allWorkbenches`, filtra
`PyreInteractable` e mostra braseiros acesos e apagados. Não ativa a descoberta
do jogo e não acende braseiros automaticamente. A cache é atualizada a cada
segundo e descartada na troca de cena. O ESP continua respeitando filtros,
raio, projeção e limite de marcadores; a navegação usa a lista completa.

Em **MISC → Braseiros / Fogueiras**, escolha o índice com o seletor ou os
botões Anterior/Próximo. A descrição informa aceso/apagado e distância.
**Ir ao braseiro selecionado** move somente o jogador local para um ponto
com chão e sem colisão ao lado do alvo. Cada clique é consumido uma vez;
requisições durante troca de cena são descartadas. Sem espaço livre, o
teleporte é recusado com mensagem. A aceitação de movimento pelo servidor
no modo cliente continua dependendo da partida. Não se move outro jogador.

## Controles visuais e versão

Itens e Pontos/Mundo agora têm distância, linha com origem Base/Topo/Centro
e box independentes e persistidos. Itens com bounds projetados recebem a
caixa do modelo. Sem bounds, a caixa destaca o marcador, não simula geometria.
O ESP de inimigos preserva controles independentes já existentes.

A aba VISUAL identifica este conjunto como **ESP-PYRE-1** e contém
**Silhueta → Contorno do corpo**, incluída na compilação anterior. A silhueta
segue com validação em partida pendente e com as limitações registradas em
[Silhueta e autoridade](SILHUETA_E_AUTORIDADE.md). Uma DLL antiga já carregada
não adquire novas opções por copiar arquivos: é necessário reiniciar e injetar.

## Verificação

- Builds nativo x64 e gerenciado contra o assembly auditado.
- Testes de teleporte: seleção, clique único, ausência de chão, obstáculo,
  colisão própria, bloqueio de alvo remoto e troca de mapa.
- Testes de ESP: braseiro aceso/sem marcador descoberto, filtro, bounds de
  item, controles independentes e atualização do cálculo de 80/80 para 32/80.
- A integração em partida dos novos braseiros, teleporte e silhueta ainda
  deve ser validada após recarregar a DLL; não foi automatizada pela tela.
