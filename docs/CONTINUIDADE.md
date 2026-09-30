# Continuidade para o próximo agente

Snapshot de contexto: 30/09/2026. Antes de alterar qualquer coisa, reconfira o estado
real do disco e do Git. Este arquivo não substitui testes nem uma inspeção atual.

## Entrada obrigatória

1. Ler AGENTS.md, COMECE-AQUI.md e ARQUITETURA.md.
2. Trabalhar em D:/Projeto/ZB2 Menu, a fonte canônica do projeto.
3. Conferir git status, diff e diff --cached. Há mudanças locais acumuladas e arquivos
   novos. Não usar reset/clean nem sobrescrever arquivos com uma cópia antiga.
4. Este guia acompanha a entrega de continuidade do cliente 1.14 e Admin 1.13,
   posterior ao commit 0787c00. Compare HEAD com origin/main antes de trabalhar;
   não use um hash antigo anotado aqui como prova do estado remoto atual.
   O proprietário autorizou a publicação desta entrega. Novas publicações precisam
   de autorização própria, salvo um fluxo contínuo explicitamente autorizado.

## Estado do produto

- Cliente instalado: 1.14-local, com mensagens de monitoramento corrigidas e Detalhes.
- Admin: interface 1.13, recompilada incorporando o cliente 1.14.
- Auto-inject existe. Não confundir com auto-update, que não existe ainda.
- Runtime empacotado declarado nos builds: commit 21759653045fed481d11e6ccd02122ad558250e7.
  Isso identifica o runtime da entrega, não prova que a árvore atual gere os mesmos bytes.
- A identidade visual é DEADBLOCK. Identificador assinado de produto continua Menu-ZB2.
- Licenças/autorizações são offline; não há sincronização nem revogação instantânea.
- Arquivos de entrega atuais: dist/admin, dist/loader e os dois ZIPs em dist/entrega.

## Cuidados com a emissão

O perfil do proprietário WeFagundes fica em Documentos/ZB2Menu/Admin. Material privado
de manutenção fica em .local/private. Não imprimir, copiar para testes ou distribuir
essas chaves. DPAPI depende do usuário Windows: uma sessão de sandbox não equivale
ao usuário real. Nunca resolver falha DPAPI regenerando a chave do proprietário.
Usar perfis de teste separados dentro de build e a credencial de teste apropriada.

## Se for atualizar o menu agora

1. Identificar precisamente a mudança pedida. Ler a skill local pertinente antes
   de mexer em Mono, hooks, ESP ou offsets.
2. Verificar a versão/assinatura das assemblies do jogo. Não inferir offsets por nome.
3. Compilar e testar o runtime; registrar commit real, hashes e limitações.
4. Empacotar o loader com a versão correta e testar licença, extração e estados.
5. Recompilar o Admin, pois ele incorpora o loader.
6. Rodar testes nativos, auto-inject/readiness, assinatura e pacotes personalizados.
7. Gerar ZIPs iniciais via scripts/package_delivery.py.
8. Conferir que ZIPs não contêm chave privada, perfil, histórico, símbolos ou fontes.
9. Atualizar dist com os aplicativos fechados; preservar Documentos e verificar acesso.
10. Atualizar os guias, o registro de entrega e os resultados reais dos testes.

Não executar uma segunda injeção para validar uma DLL já carregada. Na última
validação, o jogo já tinha menu, monitor e Mono; houve consulta somente de leitura.
Isso não equivale a um teste completo de uma nova injeção ou nova versão do jogo.

## Testes e evidências existentes

- tests/loader_functional_tests.cpp: interface, licença, pacote, leitura game-status.
- tests/auto_inject_tests.cpp: lógica de prontidão e limite de tentativas.
- tests/admin_native_tests.cpp: telas, ações, cache-test e render-profile.
- tests/test_personalized_packages.py: ZIP inicial sem ID, ZIP licenciado, equipe e importação.
- tests/run_admin_validation.py e test_loader_functional.py: validação criptográfica.
- build/v24-validation.json: última entrega do cliente documentada nesta sessão.
- .local/audits/2026-09-30-cleanup: inventários e 25 exclusões autorizadas.
- .local/audits/virustotal-2026-09-30: respostas brutas e pedido de revisão não enviado.

Os diretórios build e .local não são publicados no Git. Seus relatórios podem não
existir em uma cópia nova do repositório. Regenerar evidências quando necessário,
sem fabricar sucesso com base apenas no texto deste documento.

## Pendências reais

- Atualizador GitHub: apenas projeto em ATUALIZACAO-GITHUB.md. Repositório de código
  privado; não colocar token de acesso no cliente.
- Limpeza automática do runtime do cliente e recuperação automática: não implementadas.
- Instalação/reparo automático de dependências do Windows: não implementado.
- Authenticode: executáveis sem assinatura de publicador.
- Antivírus: ZIP anterior com 5 detecções, executável contido com 7; Microsoft undetected.
  O cliente atual não tinha relatório VT na consulta. Não declarar falso positivo confirmado.
- Uma chave de API foi compartilhada em conversa. Não reutilizá-la nem salvá-la no código.

## Como encerrar uma atualização

Informar versão de cada componente, alterações, testes realmente executados,
limitações, quais pacotes enviar e estado da publicação no GitHub. Separar recursos
implementados de propostas. Usar texto simples e não prometer zero resíduos ou
ausência de detecção por antivírus.

Organização atual: fontes nativos em `src/menu/`. Consulte [DESENVOLVIMENTO.md](DESENVOLVIMENTO.md).
