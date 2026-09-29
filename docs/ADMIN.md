# ZB2 Admin — guia da equipe

São dois aplicativos diferentes:

| Aplicativo | Quem usa | Para quê |
|---|---|---|
| **ZB2 Menu** | Cliente | Ativar a licença e carregar o menu no jogo |
| **ZB2 Admin** | Proprietário e integrantes autorizados | Emitir licenças, escolher prazos e consultar o histórico local |

O Admin é um executável Windows. A equipe não precisa de Python nem de terminal.
Sua chave principal não é incluída em nenhum dos executáveis distribuídos.
A estação do proprietário foi configurada no usuário Windows WeFagundes.
A cópia protegida para esse usuário fica em
`D:/ZB2-Retomada/loader-private/issuer-owner.dpapi`; ela não deve ser enviada à equipe.

## Emitir uma licença para um cliente

1. Peça ao cliente para abrir **ZB2 Menu → Copiar ID** e enviar o ID.
2. Abra **ZB2 Admin → Licenças**.
3. Preencha **Nome do cliente** e **ID do computador do cliente**.
4. Escolha o **Prazo de uso**: 7, 15, 30, 90, 180 ou 365 dias. Para outro prazo,
   digite a quantidade no campo numérico. O limite da sua estação aparece abaixo.
5. Confira a data em **Válida até** e clique em **Gerar licença**.
6. Clique em **Salvar arquivo** e envie o `.zb2license` ao cliente.
7. O cliente usa **Abrir arquivo** no loader e clica em **Ativar**.

O prazo começa quando a licença é gerada. Gerar novamente cria outra licença;
não cancela automaticamente a anterior. O histórico permite buscar pelo nome,
ID do computador ou integrante e copiar/salvar uma emissão anterior.
“Válida” informa a validade por data; não significa que o cliente já ativou.

## Autorizar um integrante que usa outro PC

**No PC do integrante:**

1. Entregue a ele somente o `ZB2Admin.exe`.
2. Na primeira abertura, o aplicativo mostra **Esta estação**.
3. Ele informa o nome e clica em **Criar solicitação para o proprietário**.
4. Salva o arquivo `.zb2station` e envia para você. Esse arquivo contém apenas
   identificação e chave pública; a chave privada permanece no PC dele.

**No seu PC, como proprietário:**

1. Abra **Equipe → Abrir solicitação** e selecione o arquivo recebido.
2. Confira o nome do integrante.
3. Escolha **Autorização para emitir (dias)** e **Máximo de dias por licença**.
4. Clique em **Gerar autorização** e envie o arquivo `.zb2issuer` ao integrante.

Exemplo: autorização por **365 dias**, com máximo de **30 dias por licença**.
Durante a autorização, ele poderá emitir licenças de 7, 15 ou 30 dias, mas não de
90 dias. Uma licença também não pode ultrapassar a validade da autorização dele.

**De volta ao PC do integrante:**

1. Abra **Esta estação → Importar autorização**.
2. Selecione o `.zb2issuer` recebido.
3. Abra **Licenças** e emita dentro dos limites concedidos.

Você pode reenviar a autorização pela lista **Equipe → Estações autorizadas →
Salvar autorização selecionada**. Renovar a autorização não invalida uma anterior
que ainda esteja dentro do prazo. Não há revogação instantânea offline.

## Não confundir os arquivos

| Arquivo | Destino |
|---|---|
| `.zb2license` | Cliente: ativação no ZB2 Menu |
| `.zb2station` | Proprietário: solicitação pública de uma estação da equipe |
| `.zb2issuer` | Integrante: autorização para emitir naquele PC |
| `.dpapi` | Chave principal protegida: somente o proprietário; não enviar à equipe ou ao cliente |

## Dados e limites desta fase offline

- Os dados do Admin ficam em `%LOCALAPPDATA%/ZB2Admin`. Na estação proprietária,
  **Esta estação → Abrir pasta de dados** abre essa pasta.
- Cada integrante possui histórico local no próprio PC. Não existe lista central
  automática de todas as emissões da equipe, sincronização ou painel na nuvem.
- A proteção DPAPI depende do usuário/perfil Windows. Copiar apenas os arquivos
  para outro usuário não migra a chave. Faça backup protegido do perfil e dos dados.
- O aplicativo não controla pagamentos ou quantidade de emissões. O limite
  configurável é de **dias por licença**, validado também pelo loader.
- Os registros locais não são um log inviolável. Os arquivos de licença e de
  autorização possuem assinaturas; alterar seus limites invalida a assinatura.
- A proteção direta da DLL continua fora desta revisão. Consulte as limitações
  do [loader offline](LOADER.md) antes de usar o sistema comercialmente.

## Manutenção técnica

`scripts/build_admin.py --tests` compila o Admin e seus testes com o compilador
.NET Framework do Windows. `tests/run_admin_validation.py --key CAMINHO_DPAPI`
verifica o emissor, os controles em um formulário oculto e a interoperabilidade
com o verificador C++ compilado. Os testes não controlam a tela do usuário.

Licenças antigas ZB2L1 permanecem aceitas. As novas ZB2L2 carregam uma assinatura
do integrante e uma autorização assinada pelo proprietário. O loader confere
produto, identidade, assinatura, prazo da autorização e duração da licença.
