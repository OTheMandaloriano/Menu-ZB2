# Estrutura oficial: DEADBLOCK / ZB2

## Projeto: D:/Projeto/ZB2 Menu

| Local | Conteúdo | Enviar? |
|---|---|---|
| apps, src/menu, imgui, kiero, injector e managed | Fontes e dependências do menu/loader/admin | Não |
| scripts, tests, docs, packaging, memory | Compilação, testes, guias, chave pública e conhecimento técnico | Não |
| .git | Histórico do repositório | Não |
| .agents, .opencode | Ferramentas e dependências dos agentes | Não |
| .local/private e .local/backups | Chave privada e recuperação | Nunca |
| .local/audits | Inventário completo e comprovantes de limpeza | Não |
| build/Release_x64 | Runtime do menu necessário para montar o cliente | Não enviar a pasta |
| build | Compilação, dependências verificadas e evidências de testes | Não |
| dist/admin/ZB2Admin.exe | Seu painel atual | Use localmente |
| dist/loader/ZB2Menu.exe | Programa atual do cliente | Prefira o ZIP |
| dist/entrega/01-CLIENTE-INICIAL.zip | Primeiro envio ao cliente, sem licença | Sim |
| dist/entrega/01-EQUIPE-INICIAL.zip | Primeiro envio à equipe, sem autorização | Sim |

Os arquivos .cpp/.h/.inl do menu ficam em src/menu. O projeto Visual Studio permanece
na raiz; includes, referências de projeto e testes foram ajustados juntos.
node_modules do OpenCode são dependências, não lixo do produto; ficam fora dos ZIPs.

## Dados: C:/Users/WeFagundes/Documents/ZB2Menu

`Admin` guarda estação, chave protegida, histórico e serviço em uso.
`loader` guarda ativação e runtime do cliente. `configs`, `imgui.ini` e `logs`
pertencem ao menu. `Pacotes` recebe os ZIPs que você salva para enviar.

Não apagar licenças, configurações, logs ou backups por idade. Serviços antigos
do Admin podem ser removidos pela ferramenta do painel após análise. Ela preserva
o serviço atual e os arquivos em uso. Limpeza externa manual exige aplicativos fechados. O runtime do jogo não é tratado como cache descartável.

## Atualizações e limpeza

Veja [ATUALIZACOES.md](ATUALIZACOES.md) para a rotina técnica e [ADMIN.md](ADMIN.md)
para o fluxo de cliente/equipe. Inventários e manifestos de remoção ficam em
`.local/audits`; não contêm o conteúdo de chaves ou licenças e não vão ao Git.

Mapa de código, explicação de cada arquivo de Release_x64 e política de cache: [ARQUITETURA.md](ARQUITETURA.md).
O Admin oferece análise prévia e limpeza limitada de serviços antigos em Configurações.

Entrada geral: [COMECE-AQUI.md](COMECE-AQUI.md). Resíduos e limites atuais: [RETENCAO-E-LIMPEZA.md](RETENCAO-E-LIMPEZA.md).

Organização atual: fontes nativos em `src/menu/`. Consulte [DESENVOLVIMENTO.md](DESENVOLVIMENTO.md).
