# Identidade pública e portabilidade

## Créditos

Marca do produto: DEADBLOCK. Conta pública confirmada no GitHub: `OTheMandaloriano`.
Nome de exibição: The Mandalorian. Use o identificador público nos créditos do projeto,
nunca o nome civil, nome do computador ou nome da conta Windows do desenvolvedor.
Fixtures usam nomes fictícios como OperadorTeste.

## Pastas do usuário

Admin e loader resolvem Documentos com `SHGetKnownFolderPath(FOLDERID_Documents)`.
O menu usa a mesma API, com fallback documentado para LocalAppData quando necessário.
Não concatenar `C:/Users`, não presumir que Documents esteja no disco C e não
inserir o nome da conta do desenvolvedor em código ou instruções de instalação.

O caminho exibido em um log local pode conter o nome da conta que executou o programa.
Isso é diferente de um caminho fixo compilado. Não publicar logs pessoais brutos.
Também é normal o Admin mostrar o responsável pela estação e o usuário Windows
daquela instalação. Esses valores são dados locais, não créditos do produto.

Preservar `Documentos/ZB2Menu` e o identificador assinado `Menu-ZB2`: mudar esses
contratos sem uma migração poderia perder configurações ou invalidar licenças.
Documentos é uma pasta conhecida do Windows; o diretório pode variar entre PCs.

## Nomes técnicos

A migração troca o projeto e a DLL principal por `Deadblock.Menu.vcxproj`
e `Deadblock.Menu.dll`. O identificador interno da rotina de mira passa a
`DeadblockAim`. Aplique juntos projeto, DLL, DllImport da ponte gerenciada, helper,
configuração, manifesto, leitura de módulos e testes. Uma troca isolada não é segura.

Essa migração foi validada em cópia isolada, aplicada aos fontes e instalada localmente no cliente 1.15 e Admin 1.14.
A compilação e testes fora do jogo não substituem validação da nova DLL em partida.

O reconhecimento do nome antigo pode continuar no código exclusivamente para
compatibilidade, evitando carga duplicada. Dependências como Kiero, ImGui e Harmony
mantêm seus nomes e atribuições. Referências de pesquisa permanecem na documentação
histórica; renomear a interface não autoriza apagar a procedência de código terceiro.

## Revisão obrigatória

Execute `python tests/check_portable_identity.py`. Não gere relatórios públicos com
nomes de usuários, licenças, chaves, tokens ou caminhos completos do perfil.
Arquivos novos devem passar pela mesma verificação antes do commit.

Corrigir os arquivos atuais não apaga nomes de commits antigos. Reescrever o histórico
Git é outra operação e exige planejamento, backup e autorização específica.

## Instalação e atualização em outro PC

O usuário não precisa informar nem editar seu nome de conta para instalar.
O programa consulta a pasta Documentos do usuário Windows que está executando-o.
O argumento --data do Admin é uma opção técnica para diagnóstico; não copiar
atalhos de desenvolvimento com um caminho absoluto para o computador do cliente.
Distribuir os ZIPs gerados pelo Admin, sem incluir atalhos pessoais ou perfil de estação.
Ao atualizar, preservar os dados do mesmo usuário Windows. Mudar de usuário ou PC
pode exigir reativação ou autorização porque DPAPI e a licença têm vínculos próprios.

O teste check_portable_identity.py verifica código, testes, guias e arquivos de
entrada. Ele é uma barreira estática, não comprovação de compatibilidade com todos
os nomes Unicode ou pastas redirecionadas. O logger nativo ainda usa conversão ANSI
e limites de caminho legados; o loader/Admin recusam alguns caminhos redirecionados
por segurança. Esses limites exigem testes específicos antes de prometer suporte total.

## Cópias antigas e histórico

Use a branch main atual ou o projeto oficial. Uma página de commit antigo continua
mostrando o conteúdo daquela data. A correção dos guias atuais não reescreve commits
anteriores. Cópias de trabalho fora do projeto oficial também podem estar desatualizadas.
Perfis locais, logs e backups podem conter o nome da conta real; não são distribuídos
nem devem ser anexados integralmente a relatórios públicos.
