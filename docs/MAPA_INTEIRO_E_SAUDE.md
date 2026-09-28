# Item Magnet independente, ponto fixo e saúde remota

## Vida dos aliados: origem do percentual incorreto

O ESP usava `healthFast / MaxHealth` de uma cópia remota do jogador. A auditoria desta build encontrou:

- `PlayerMain.UpdateLocallyControlled` executa `ProcessHealth`; o caminho remoto não executa essa atualização.
- `PlayerPositionSynchronizer.SyncHealth` transmite um `HealthState`.
- `GenericListener.OnPlayerHealthState` recebe identidade e um byte de estado e chama `SetHealthState`, sem HP numérico.
- `MaxHealth` retorna 100 para jogadores sem controle local.

Logo, o valor local remanescente pode produzir 50% ou outro percentual sem representar o HP do amigo. A correção não converte 50 em 100: remove a medição falsa. Com barra/percentual habilitados, o ESP informa **Vivo | HP não sincronizado** ou **Caído**. Mortos são excluídos pelo estado, sem inferi-lo de um float desatualizado. HP numérico real de amigos exige uma fonte adicional confiável, ausente no protocolo auditado. Esse suporte não foi implementado.

## Modal de loot

**Escolher itens para puxar** abre o mesmo catálogo visual usado pelo ESP, mas com seleção e busca independentes. As quatro máscaras `iLootFilter0..3` são salvas no preset e enviadas somente ao Item Magnet. Itens bloqueados pela edição, internos/ocultos, sacos e reservas de outro jogador continuam excluídos. Categoria e filtros individuais se combinam.

## Mapa inteiro

`Mapa inteiro` remove o limite de distância. Desativado, o raio configurável aceita 10 a 1.000 m. A rotina percorre todas as células do mapa com cursor persistente, orçamento de leitura por passagem e até dois deslocamentos a cada 250 ms. O limite por passagem é de processamento, não de alcance ou de quantidade total elegível. Não cria itens nem força regiões não geradas a produzir loot.

Mantém identidade, item e propriedade da célula. O raio de chão usa a máscara de terreno do personagem. A base dos bounds visuais determina a altura do pivô para apoiar o objeto no chão. Sem dimensões ou terreno válidos, o item permanece no lugar; não se inventa uma altura fixa. A mesma identidade não é movida repetidamente enquanto a ativação estiver em andamento. Uma nova ativação permite outra coleta espacial. Os pacotes de rede e a física precisam de teste em partida.

## Magnet de inimigos

Há dois destinos:

- **Seguir minha posição:** o ponto acompanha deslocamentos relevantes do jogador.
- **Fixar no ponto da mira:** na ativação, um raycast de até 1.000 m procura chão válido. O ponto e a direção dos bosses permanecem fixos, mesmo quando o jogador anda ou olha para outro lugar. Sem chão válido, informa o motivo e aguarda uma mira válida.

H desativa/libera; a próxima ativação captura um novo ponto. Com Congelar atraídos, inimigos concluídos ficam presos no destino até a liberação. Sem congelamento, conservam a IA normal. O término do surgimento continua sendo aguardado antes de mover/congelar. Busca de zumbis comuns ainda respeita o raio configurado; bosses existentes ignoram o raio de origem.

## Por que Magnet de amigos ainda não está entregue

`GenericListener.OnPlayerPositionSync` alimenta o buffer de posições. `PlayerMain.UpdateNonLocallyControlled` consome esse buffer; o jogador controlado localmente executa outro caminho e publica sua própria posição. Os teleportes de van/helicóptero auditados atuam em `MyPlayer`, não comandam remotamente outro participante.

Um teleporte imposto somente à representação no host não demonstra que o amigo se moveu no próprio computador. Uma implementação cooperativa precisa de suporte de comando também no cliente do aliado, autorização/seleção de identidade e confirmação de execução. Não foram criados botões que simulem esse resultado. Modal de jogadores, escolta, redirecionamento de spawn e silhueta permanecem pendentes.

## Verificações

Modelos testados: HP remoto desatualizado, estados de queda/morte, filtros independentes em presets, loot existente a 2.000 m com mapa inteiro, raio de 1.000 m, deslocamento do pivô, ponto de inimigos fixo após mover câmera/jogador e novo ponto ao reativar. Compilação exata do jogo e regressões de entrada/pausa mantidas. Validação ao vivo ainda pendente.
