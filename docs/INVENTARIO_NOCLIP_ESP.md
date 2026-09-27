# Inventário, NoClip e filtros individuais

## Slots

A implementação anterior escrevia `UnlockedMiscSlotsCount=16` sem aumentar a lista real e forçava `UsableSize=16x20` sem respeitar `TotalSize`. `TryGetEmptyMiscSlotIndex` e operações de empilhamento percorrem a quantidade desbloqueada e acessam a lista; essa inconsistência permite exceções de índice.

A nova integração usa `MiscCount`, `TotalSize`, `SetUnlockedMiscSlotsCount` e `SetUsableSize`, preserva os valores originais por inventário e notifica a mudança uma vez. Não reconstrói nem limpa o inventário durante o arraste. Ao desligar, se houver itens ocupando a área extra, exibe uma pendência de restauração até o usuário movê-los. Não apaga nem move itens automaticamente.

Uma sessão já alterada pela versão antiga precisa ser reiniciada antes do teste: o novo código não pode reconstruir originais perdidos pela versão anterior.

## NoClip

Usa o componente existente `NoClip`, preservando `enabled`, velocidade, `Rigidbody.isKinematic` e `detectCollisions`. Somente o componente do jogador local controlado recebe o prefixo de entrada. Não habilita `DebugEnabled` global. WASD movimenta; Space sobe; Ctrl desce; Shift não acrescenta multiplicador oculto. O padrão é 1× da caminhada original. Vetor diagonal é normalizado.

Menu aberto, perda de foco e pausa bloqueiam o movimento. Solicitação expirada e morte restauram a física. Se a desativação ocorrer dentro de um obstáculo, retorna à última posição livre antes de restaurar colisões. O teste de sobreposição usa uma aproximação conservadora do hitbox. Correções de posição impostas pelo servidor continuam possíveis no modo cliente.

## ESP

Checkboxes colocados após seletores de cor com `SameLine` podiam sair da coluna e parecer ausentes. Agora cada categoria ocupa uma linha. A lista por item usa os IDs do enum da build auditada, com busca e seleção Todos/Nenhum. Categorias continuam sendo o primeiro filtro; nomes individuais são o segundo. Filtros são salvos nos presets e publicados pelo snapshot de configuração.

Pontos recebem seleção individual dos tipos reais do jogo, incluindo sepulturas e outros jogadores. A posição do próprio jogador é ignorada. Tipos sem registros locais sincronizados não são fabricados. O marcador de onda continua pendente porque não corresponde a um ponto fixo da lista do jogo.

## Verificação

Testes usam Harmony e modelos controlados para conferir limites do inventário, arraste sem atualização repetida, restauração com área ocupada, NoClip, watchdog, filtros e presets. Compilação usa o assembly exato instalado. Não substituem validação em partida solo/host/cliente.
