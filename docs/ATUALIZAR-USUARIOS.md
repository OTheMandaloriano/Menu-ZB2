# Atualizar clientes e equipe sem perder acesso

Hoje as atualizações são enviadas manualmente. Um commit no GitHub não atualiza o
programa instalado no PC de ninguém. Um ZIP já enviado também não muda sozinho.

## Se apenas o painel Admin mudou

O desenvolvedor compila o Admin. Proprietário e integrantes recebem o novo Admin;
o cliente de jogo não precisa receber outra versão se o loader/menu não mudou.

## Se o menu ou o loader mudou

1. O desenvolvedor compila e testa o menu, quando necessário, e depois o loader.
2. Recompila o Admin para incorporar o novo loader. Sem isso, o painel continua
   gerando ZIPs com o cliente antigo, mesmo que o arquivo em dist/loader seja novo.
3. Gera novamente os ZIPs iniciais em dist/entrega.
4. O proprietário atualiza seu Admin e envia os novos pacotes da equipe.
5. Os integrantes atualizam seus Admins antes de gerar pacotes para clientes.

## Cliente que já possui licença

1. No Admin atualizado, localize a licença ainda válida em Clientes.
2. Clique no ícone ZIP dessa linha e salve o pacote atualizado.
3. Envie-o ao cliente. Reexportar não altera a validade e não exige um novo ID.
4. O cliente fecha o jogo e o loader.
5. Extrai o pacote na pasta fixa onde mantém o aplicativo, substituindo o programa
   antigo. Não executa diretamente de dentro do ZIP.
6. Abre ZB2Menu.exe, confere a versão e depois abre o jogo.

Não apagar Documentos/ZB2Menu. A licença e as configurações ficam nessa pasta.
Se a licença venceu, é necessário renovar; a atualização do programa não a renova.
Se a pessoa mudou de computador, obtenha o novo ID e trate como nova vinculação.

## Integrante que já está autorizado

1. No Admin atualizado do proprietário, abra Minha equipe.
2. Clique com o botão direito na estação autorizada e salve o ZIP dela novamente.
3. Envie esse ZIP ao integrante. Ele fecha o Admin antigo antes de extrair e substituir.
4. Abre o novo ZB2Admin.exe no mesmo PC e usuário Windows e confere Meu acesso.
5. O histórico e a chave da estação continuam em Documentos/ZB2Menu/Admin.

Não criar uma nova estação apenas para atualizar o executável. Não copiar o perfil
do proprietário para o PC de um integrante. Autorização vencida exige renovação.

## Downloads antigos

Os ZIPs recebidos e pastas de extração adicionais pertencem ao usuário. O aplicativo
não os apaga. Use uma pasta fixa para evitar Cliente (1), Cliente (2) e atalhos
apontando para versões antigas. Só descarte o download anterior depois de confirmar
o funcionamento do novo e conservar a recuperação necessária.

O retorno automático à versão anterior ainda não existe. Em caso de falha, a equipe
deve fornecer uma compilação anterior conhecida, compatível e verificada. Não tente
voltar a DLL com a partida em andamento.
