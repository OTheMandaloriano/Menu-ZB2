# Histórico

Formato baseado em Keep a Changelog. O projeto ainda não tem release estável numerada.

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
