# Código, distribuição e atualização

## Código privado

`OTheMandaloriano/Menu-ZB2` guarda fontes, testes, documentação e histórico de desenvolvimento. O nome de apresentação é Menu ZB2; o identificador GitHub usa hífen porque nomes de repositório não contêm espaços.

Builds atuais, configurações pessoais, tokens e bibliotecas proprietárias do jogo não entram nos novos commits. O histórico local existente é preservado; versões antigas podem conter artefatos removidos em commits posteriores. Não tornar o repositório público sem revisar também esse histórico.

## Canal de distribuição separado

Planejado: outro repositório ou serviço exclusivamente para Releases, sem copiar os fontes. Seu nome e sua visibilidade ainda serão definidos. Nenhum canal de distribuição é criado por esta publicação do código.

Um pacote de release deverá identificar versão, commit de origem, compatibilidade do jogo e hashes dos arquivos. Símbolos de diagnóstico ficam arquivados por versão e não precisam acompanhar o pacote do usuário.

## Launcher planejado

Entrega de um executável que prepara os componentes em `Documentos/ZB2Menu/runtime/<versão>`, usando o caminho de Documentos fornecido pelo Windows. Configurações, logs e licenças ficam em subpastas próprias. A implementação atual ainda usa o injetor e arquivos separados.

O atualizador deverá consultar Releases, verificar integridade/autenticidade, instalar uma versão completa antes de ativá-la e preservar uma versão anterior para recuperação. Não substituir uma DLL em uso. Presets e logs não devem ser removidos durante a atualização.

Releases públicas permitem download sem uma credencial pessoal embutida. Releases privadas exigem autenticação autorizada do usuário ou um serviço intermediário. Nunca incorporar o token do proprietário ao executável. Controle de acesso ao menu é uma função distinta da verificação de atualização e ainda não foi implementada.
