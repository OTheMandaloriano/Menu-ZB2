# Contribuir

Leia o README, o NOTICE e as auditorias da área antes de alterar o projeto.

- Faça uma mudança lógica por commit. Use a convenção existente: `fix:`, `feat:`, `docs:`, `test:`, `build:`, `chore:` e descrição em português.
- Explique causa, comportamento resultante, testes executados e limites da validação.
- Preserve alterações locais de outras pessoas. Não use force-push para reorganizar histórico compartilhado.
- Não versione tokens, configurações pessoais, artefatos de build ou binários do jogo.
- Execute os testes aplicáveis e atualize o CHANGELOG. Não apresente testes simulados como validação dentro do jogo.
- Chamadas Unity pertencem ao callback do jogo. A interface troca snapshots e não acessa objetos Unity.

Use identidade Git vinculada à sua conta, preferencialmente com e-mail noreply do GitHub.

## Estrutura dos fontes

O menu nativo fica em `src/menu`; o projeto Visual Studio permanece na raiz.
Admin e loader ficam em `apps`. Consulte `docs/DESENVOLVIMENTO.md` e rode
`python tests/check_repository_layout.py` ao alterar caminhos ou referências.
Saídas de testes/compilação ficam em `build`, entregas em `dist` e material privado
em `.local`. Não crie cópias de fontes ou scripts de sessão na raiz.
