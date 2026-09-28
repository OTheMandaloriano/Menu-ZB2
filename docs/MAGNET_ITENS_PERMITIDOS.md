# Itens permitidos, equipe simplificada e Magnet

## Itens bloqueados

`ItemEligibility` centraliza a regra: rejeita ID None, item oculto, reservado/interno e `Pricing.IsBlocked`. O catálogo, a obtenção de ícones, a coleta e revalidação do ESP e o Item Magnet usam a mesma regra. A criação automática de pilhas de munição também consulta `AllowedId` antes de chamar o método do jogo. Não é feita tentativa de desbloquear equipamentos proibidos pela edição instalada.

## Equipe

Controles visíveis: box/tipo/cor, nome, distância, barra de vida, percentual, esqueleto, linha/origem e raio. Ponto na cabeça, escolha de layout e editor de arraste foram retirados. Chaves antigas podem continuar no preset para compatibilidade, mas não desenham ponto na cabeça nem aplicam layout personalizado de equipe.

## Congelamento de inimigos atraídos

O checkbox Congelar atraídos agora executa uma rotina: guarda o estado de física e a velocidade da animação, suspende os métodos de movimento/ação dos zumbis reunidos e restaura ao desligar. Morte, troca de autoridade e expiração do sinal de atualização liberam a entidade. Não zera cooldowns nem cria inimigos. Requer solo/host. O efeito gráfico e de rede precisa de validação em partida, especialmente bosses.

## Item Magnet

Disponível em solo/host. Usa a tecla configurável de segurar (X como padrão de novos presets) ou ativação pelo checkbox. Presets anteriores preservam a tecla escolhida. Move no máximo dois itens por passagem de 250 ms para o chão próximo ao jogador, respeitando categoria e filtros individuais próprios. Mapa inteiro remove o limite espacial; o modo por raio aceita até 1.000 m. A varredura mantém cursor para alcançar todas as células existentes.

Não pega itens automaticamente. Rejeita sacos de loot e loot reservado a outro jogador, itens bloqueados, ocultos ou internos. Conserva a instância e o ID, transfere a propriedade da célula espacial, reposiciona e usa as mensagens do jogo para remover/reanunciar a representação aos clientes. Há restauração da célula/posição em caso de falha durante a movimentação local. Não cria uma cópia do item nem chama AddItem como fallback quando a mochila está cheia.

## Escopo restante da especificação

O documento completo ainda **não está entregue**. Horda seguindo outro jogador, redirecionamento de spawn, escolta/agrupamento de aliados, modal com fotos Steam, coleta automática e regras detalhadas por subtipo continuam pendentes. Não são apresentados como recursos ativos nesta revisão. Kill On Spawn e campos sem implementação não foram reativados. Essas pendências precisam de auditoria e validação próprias de identidade, estado e sincronização.

## Verificação

Build nativa e gerenciada; testes de catálogo bloqueado, cache do ESP, modificação/restauração, suspensão por Harmony, movimentação de loot entre células sem duplicação e renderização da equipe. Nenhum teste simulado comprova o resultado em uma sessão multiplayer real.

Atualização: [saúde remota, mapa inteiro e ponto fixo](MAPA_INTEIRO_E_SAUDE.md).
