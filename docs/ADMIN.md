# ZB2 Admin — guia simples (1.4)

## Você é o proprietário

Neste computador, a estação verificada é **WeFagundes — Proprietário**.
Abra o atalho **ZB2 Admin - Proprietario**. Seu nome e papel aparecem no topo.
Você não precisa criar uma solicitação de integrante para usar sua própria conta.

- **Clientes:** preparar o ZIP para quem vai usar o menu no jogo.
- **Minha equipe:** autorizar outro PC a gerar licenças para clientes.
- **Meu acesso:** consultar o acesso deste computador. Criar solicitação nesta
  tela configura ESTE PC como integrante; não cadastra outra pessoa à distância.
- **Ajuda:** explica os destinos dentro do aplicativo.

## Enviar ao cliente: um único ZIP

1. Peça ao cliente o ID exibido no ZB2 Menu. Se ele ainda não possui o programa,
   envie o CLIENTE.zip inicial para ele abrir e clicar em Copiar ID.
2. Abra **Clientes** no Admin.
3. Preencha nome, ID do computador e prazo em dias.
4. Clique em **Gerar ZIP do cliente** e escolha onde salvar.
5. Envie SOMENTE o ZIP gerado para esse cliente.

O ZIP personalizado contém `ZB2Menu.exe` e `licenca.zb2license`.
O cliente extrai TUDO para uma pasta e abre `ZB2Menu.exe`. A licença é verificada
para aquele PC e aplicada automaticamente. Não executar diretamente dentro do ZIP.

A opção **Gerar somente a chave** continua disponível. Também é possível copiar
uma licença na linha do histórico e o cliente colar a chave manualmente.

Para reenviar uma emissão existente, selecione-a no histórico e use **Gerar ZIP
pronto**. Isso não cria outra licença nem reinicia o prazo. O prazo começa na emissão.
Uma renovação válida com vencimento posterior é aplicada ao abrir o novo pacote.

## Enviar a um integrante que vai emitir

O integrante precisa preparar o próprio PC uma única vez:

1. Envie o EQUIPE.zip inicial. Ele extrai e abre o Admin.
2. Em **Meu acesso**, ele informa o próprio nome e cria uma solicitação.
3. Ele envia o `.zb2station` para você.
4. Você abre **Minha equipe → Abrir solicitação** e escolhe os limites.
5. Clique em **Autorizar e gerar ZIP** e envie o ZIP gerado ao integrante.
6. Ele extrai tudo e abre `ZB2Admin.exe` no PC que criou a solicitação.
   A autorização incluída é importada automaticamente.

O ZIP personalizado contém `ZB2Admin.exe` e `autorizacao.zb2issuer`.
Para reenviar, selecione a autorização na lista e use **Gerar ZIP do integrante**.

Exemplo de limites: autorização por 365 dias, licenças de no máximo 30 dias.
O integrante não recebe sua chave principal nem pode autorizar outros integrantes.
Se alguém da equipe apenas vai jogar, envie o pacote DE CLIENTE, não o Admin.

## Cadastrei este PC como integrante por engano

A opção **Sou o proprietário: recuperar acesso** exige a chave original do
proprietário, protegida pelo Windows. Ela não promove ninguém apenas pelo nome.
A recuperação preserva o histórico e guarda uma cópia protegida da configuração
anterior. Não envie a chave `.dpapi` a clientes ou integrantes.

No seu perfil WeFagundes, a chave de manutenção fica em
`D:/ZB2-Retomada/loader-private/issuer-owner.dpapi`; o Admin já está configurado.
Os dados ficam em `%LOCALAPPDATA%/ZB2Admin`. Não distribua essa pasta.

## O que não vai no ZIP

Nenhum AVISOS.txt, chave privada, histórico, pedido de estação ou arquivo de build
é colocado no pacote personalizado. Atribuições das dependências ficam embutidas,
acessíveis em Créditos, sem arquivos extras na pasta do cliente.

Os ZIPs iniciais CLIENTE.zip/EQUIPE.zip servem para começar o cadastro; ainda não
contêm uma licença/autorização pessoal. Os ZIPs prontos são gerados dentro do Admin.

## Limites offline

Cada PC mantém seu próprio histórico. Não há sincronização automática, controle
de pagamento ou revogação imediata. O ID do cliente e a solicitação do integrante
continuam necessários para vincular o acesso ao computador correto.

A proteção direta da DLL e a validação real do AUTO-INJECT em partida continuam
com os limites documentados em [LOADER.md](LOADER.md) e [AUTO_INJECT.md](AUTO_INJECT.md).

## Validação técnica

- `scripts/build_loader.py`: primeiro compilar o cliente.
- `scripts/build_admin.py --tests`: embutir esse cliente no Admin e compilar.
- `tests/test_personalized_packages.py --key CAMINHO_DPAPI`: ZIPs, importação,
  recuperação, renovação, PC incorreto e preservação do histórico.
- `tests/run_admin_validation.py --key CAMINHO_DPAPI`: assinatura e permissões.
- `scripts/package_delivery.py`: produzir os ZIPs iniciais sem dados pessoais.
