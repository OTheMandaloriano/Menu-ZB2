# Histórico

Formato baseado em Keep a Changelog. O projeto ainda não tem release estável numerada.

## [Não lançado]

- Galeria de sete prévias de interface com dados sintéticos, manifesto de origem e verificação de atualização em CI.
- Guias de instalação, dependências e recuperação com a limitação atual de DPAPI entre computadores.
- Distribuição preparada como pré-release por GitHub Releases; sem atualização automática.


- Organiza 37 fontes nativos em `src/menu`, preservando o conteúdo e os nomes dos binários.
- Atualiza o projeto Visual Studio e os testes; inclui uma verificação de caminhos do repositório.
- Reescreve o README com a distribuição funcional, guias por público e limitações atuais.
- Centraliza o histórico técnico no índice de documentação e mantém o guia em inglês alinhado.

## Registros anteriores

As entradas abaixo descrevem etapas históricas. Prévia de loader e ações demonstrativas
foram substituídas pelo cliente funcional documentado no README atual.

- Loader 0.2 adota o visual compacto aprovado, com ativação/painel e sem sidebar.
- Separa entrada, janela, recursos D3D11, tema e telas; renderização testada em 100%/200%.
- Mantém ações de licença e carregamento explicitamente demonstrativas.

- Prévia nativa do loader com telas de ativação, início e pacote; sem validação/injeção real.
- Empacotador determinístico com allowlist, manifesto e verificação de integridade; pacote de desenvolvimento explicitamente não assinado.

- Teleportes locais, aplicação de hora e fila limitada de criação de inimigos conectados aos métodos do jogo.
- FOV, terceira pessoa e velocidade do dia com restauração; proteção de queda independente e reviver apenas quando caído.
- Anti-AFK substituído por Executar em segundo plano, sem promessa de evitar expulsão.

- Saitama conecta socos locais identificados a dano e impulso, em solo/host.
- Explosivos têm ativação explícita, pavio, contato, raio/dano e restauração de catálogo.
- Novo seletor pesquisável adiciona itens permitidos pelo fluxo nativo de inventário, sem descartar no chão se cheio.
- Power Drop e botões de slot sem implementação retirados da interface.

- Destinos fixos preservam posição e modo chão/ar ao alternar NoClip.
- Magnet mantém modelos promovidos no LOD enquanto ativo, independentemente da mira.
- Tooltips detalham captura, destino, tipo de alvo e distâncias; disparos têm medições separadas de verificação e execução nativa.

- Magnet promove zumbis comuns descarregados dentro do raio, além do grupo anterior e bosses.
- Chams/silhueta ficam no grupo ESP de zumbis, com master e distância compartilhados.
- Ponto de cabeça funciona sem esqueleto; bosses usam envelope do modelo também em caixas 2D/cantos.
- Rigs não padrão resolvem alvos separadamente, sem substituir pescoço/peito/pelve por olhos.

- Grupo anterior do Magnet pode ser recolhido no novo ponto, mesmo fora do raio de captura; retenção fixa anterior é liberada.
- NoClip permite capturar ponto 3D também no destino pela mira.
- Fast Knife atua na velocidade da animação local de ataque e restaura ao sair dela.
- Mira usa a referência de olhos do jogo como fallback para rigs ausentes/diferentes; skeleton respeita prazo dentro dos loops.

- Magnet compensa referência dos pés, ignora atores/cadáveres no chão e recolhe distribuídos que se afastam.
- NoClip alinha os atraídos pela referência dos olhos em frente à câmera, com bloqueio por obstáculos.
- Limita trabalho por passagem, reutiliza chão/cápsula do grupo e evita escritas de pose redundantes.
- Consulta de mira usa a máscara do disparo, expande o buffer em hordas e trata alvos retidos na mesma âncora.
- FOV/360 voltam a checkboxes mutuamente exclusivos; cor e Mostrar círculo permanecem separados.

- Preparação de colisões dos braseiros usa bounds geométricos de meshes/boxes, não apenas o pivot do LOD.
- Teleporte verifica 96 candidatos ao redor da interação e só aceita chegada a menos de 1,45 m, voltada ao braseiro.
- Diagnóstico informa LODs preparados, máscara, ausência de chão, colisão e alcance.

- Área de mira unificada em Círculo FOV/360; exibição independente e cor persistida.
- Pontos/Mundo usa somente categorias; remove a lista duplicada e ignora máscaras antigas ocultas.
- Magnet retém âncora, desativa root motion enquanto preso e trata troca do corpo físico.
- Teleporte usa a máscara de chão do jogador e informa o motivo das recusas.
- Rótulo e ajuda do único God Mode corrigidos para refletir a proteção com perks.

- Agrupamento sobreposto do Magnet compartilha destino entre os atraídos.
- ESP omite braseiros acesos; teleporte prepara colisões próximas e respeita a altura do jogador.
- Silhueta passa a definir viewport e matrizes explicitamente; confirmação visual pendente.

- God Mode bloqueia dano antes do desconto no jogador local e usa MaxHealth com perks.
- Silhueta cria view explícita para textura typeless e preserva o motivo de falha.
- Procedimento autorizado de atualização/injeção registrado na skill Mono.

- ESP de braseiros passa a usar o registro completo, incluindo não descobertos e acesos.
- Navegação por braseiro com teleporte local, chão/colisão validados e clique consumido uma vez.
- Itens e Pontos/Mundo ganham controles independentes de distância, box e linha.
- HP de zumbi comum não sincronizado no cliente deixa de ser apresentado como 100% real.
- Revisão ESP-PYRE-1 identificada na aba VISUAL; CE confirmou os valores padrão dos aliados remotos.

- Modos fixos do Magnet retêm os atraídos automaticamente, esperam o nascimento e restauram ao desligar.
- Novo destino Fixar à minha frente captura chão próximo uma vez; seguir mantém congelamento opcional.
- Reauditoria de HP documenta que a barra/% real de aliados segue sem fonte numérica sincronizada.

- Silhueta independente do Chams: borda externa, cores visível/oculta e espessura persistida.
- Shader validado em D3D11 WARP; integração com profundidade do jogo ainda pendente de teste em partida, sem suporte inicial a MSAA.
- Auditoria documenta a ausência de caminho confirmado para Magnet de amigos e cliente sem host.

- Item Magnet alterna por pressionamento, sem exigir segurar a tecla.
- Modal de loot mostra quantidade selecionada e permite selecionar pela imagem; Todos permitidos inclui todas as categorias.
- Limites locais transformados do renderer substituem bounds de mundo potencialmente antigos em loot distante.
- Contadores de varredura e falhas de destino/modelo ajudam o diagnóstico em partida.


- ESP deixa de tratar HP remoto não sincronizado como percentual real; informa o estado recebido.
- Item Magnet ganha modal visual com filtros próprios, raio de até 1.000 m e modo mapa inteiro.
- Varredura de loot usa cursor persistente e altura baseada no modelo para apoiar no chão.
- Magnet de inimigos separa seguir o jogador de fixar um ponto de chão escolhido pela mira.
- Magnet de amigos permanece sem implementação autoritativa; limitações de rede documentadas.


- Captura do overlay respeita a necessidade de mouse/teclado, preservando o acesso à UI do jogo fora da janela.
- Cursor é coordenado pela Unity e restaurado ao fechar; o caminho de UI continua funcionando durante pausa.
- Magnet e congelamento aguardam a conclusão do estado de surgimento dos inimigos.


- Itens bloqueados pela edição, ocultos ou internos são excluídos do catálogo/ESP e das operações de loot.
- Equipe mantém somente box, nome, distância, barra/percentual, esqueleto, linha e raio.
- Congelamento de atraídos executa suspensão/restauração de física e animação com watchdog.
- Item Magnet move loot permitido ao chão próximo em lotes, com atualização da célula e sincronização nativa; sem auto-coleta.
- A especificação ampliada de aliados, alvo Steam e redirecionamento de spawn permanece incompleta e documentada.


### Catálogo, equipe e estabilidade

- Catálogo de itens em modal com nomes do banco e ícones locais sob demanda; busca normalizada.
- Entrada do Windows encaminhada por fila para evitar descarte durante renderização.
- Chams com camada separada de profundidade e liberação ao desligar; requer shader compatível.
- Equipe com tipos de box, ossos reais, percentual, cores e editor de arraste independente.
- Horda ativa exibe centro de entidades carregadas; não promete spawn futuro.
- Magnet usa posições estáveis por ponto de reunião, distância de bosses separada e limite de carregamento por identidade.
- Especificação ampliada analisada; escolta de aliados, loot e avatares Steam permanecem em planejamento.


- ESP de equipe usa jogadores reais, nome, HP, box 2D e raio próprios.
- Itens ancoram no centro visual; áreas de loot são identificadas explicitamente.
- Magnet permite filtrar zumbis/bosses; bosses existentes ignoram o raio de origem.


- Boost temporário de 4× com Shift restaurado no NoClip por solicitação do proprietário; soltar retorna à velocidade selecionada.


### Ajustes de velocidade e Magnet

- NoClip e Speed Hack iniciam em 1×; NoClip usa caminhada original sem aceleração oculta de Shift.
- Removidas mensagens permanentes de ativação. Avisos acionáveis permanecem.
- Magnet segue um ponto compacto continuamente, com limites de trabalho e autoridade do host preservados.


### Removido

- Launcher, atualizador, empacotador e testes exclusivos do launcher, por decisão do proprietário. Distribuição volta ao injetor.

### Alterado

- Tecla padrão do NoClip passa de F6 para N; seleção personalizada continua disponível.

## [0.2.1] - 2026-09-27

Prévia privada; validação em partida pendente.

### Adicionado

- Atalhos configuráveis de alternância para NoClip (F6) e Magnet (H), salvos nos presets.
- Magnet de zumbis carregados para solo/host, em lotes e com verificação de destino.
- Launcher com navegação lateral, cartão do menu e páginas Início, Atualizações e Preferências.

### Corrigido

- Captura de atalho ignora a tecla ou clique que abriu o seletor.
- Novo executável pode atualizar o runtime embutido offline quando já existe uma versão anterior.

## [0.2.0] - 2026-09-26

Prévia privada, validação em partida ainda pendente.

### Corrigido

- Slots respeitam a capacidade real; desativação não deixa itens fora da área acessível.
- Categorias do ESP deixam de ser cortadas pela disposição dos seletores de cor.

### Adicionado

- NoClip local com controle de entrada, velocidade e restauração da física.
- Busca e filtros individuais por ID de item e tipo de ponto.
- Launcher com pacote embutido, atualização manual/local e consulta automática opcional ao canal privado.
- Instalação por versão em Documentos/ZB2Menu, com hashes e preservação de presets.

### Verificado

- Cliente do launcher testado contra o canal privado: consulta, download, hash, instalação isolada e ausência de atualização repetida.

## Histórico anterior à prévia

### Adicionado

- Documentação de entrada em português e inglês, orientações de contribuição e separação entre código e distribuição.
- Adaptadores gerenciados, testes de integração Harmony e testes do renderizador ImGui.
- Visualização de itens, pontos do mundo e registros de zumbis distantes.

### Corrigido

- Raios de itens e pontos fixos independentes, inclusive nos presets.
- Disputa entre atualização Unity e renderização que suprimia quadros do menu e ESP.
- Overflow do relógio monotônico em máquinas com uptime prolongado.
- Restauração de parâmetros de armas e movimento a partir de valores originais por objeto.
- Box respeita checkbox; snapline respeita a origem configurada.
- Injetor usa caminhos relativos para distribuição portátil.

### Manutenção

- Artefatos gerados ficam fora do versionamento atual.
- Backups e resíduos de builds anteriores foram retirados da árvore ativa.

### Pendente

- Validação prolongada nos três modos de jogo, controle de acesso próprio e revisão de licenciamento para eventual distribuição pública.
