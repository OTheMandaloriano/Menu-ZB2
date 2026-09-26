# Histórico

Formato baseado em Keep a Changelog. O projeto ainda não tem release estável numerada.

## [Não lançado]

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

- Launcher único, mecanismo de atualização, canal separado de Releases e validação prolongada nos três modos de jogo.
