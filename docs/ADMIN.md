# ZB2 Admin — guia da equipe

São dois aplicativos diferentes:

| Aplicativo | Quem usa | Para quê |
|---|---|---|
| **ZB2 Menu** | Cliente | Ativar a licença e carregar o menu no jogo |
| **ZB2 Admin** | Proprietário e integrantes autorizados | Emitir licenças, escolher prazos e consultar o histórico local |

O Admin é um executável Windows. A equipe não precisa de Python nem de terminal.
As duas interfaces usam Dear ImGui, sem barra de título ou controles WinForms.
Os avisos de dependências ficam em AVISOS.txt, fora das telas de uso.
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
6. Clique em **Salvar** e envie o `.zb2license` ao cliente.
7. O cliente usa **Abrir arquivo** no loader e clica em **Ativar**.

O prazo começa quando a licença é gerada. Gerar novamente cria outra licença;
não cancela automaticamente a anterior. O histórico permite buscar pelo nome,
ID do computador ou integrante e copiar/salvar uma emissão anterior.
O badge “Ativa” informa a validade por data; não significa que o cliente já ativou.
Cada linha permite copiar a licença diretamente e mostra datas com dígitos de largura fixa.

## Autorizar um integrante que usa outro PC

**No PC do integrante:**

1. Entregue a ele **EQUIPE.zip**, gerado em `dist/entrega`.
2. Ele extrai o pacote e abre `ZB2Admin.exe`. Na primeira abertura, o aplicativo mostra **Minha estação**.
3. Ele informa o nome e clica em **Criar solicitação**.
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

1. Abra **Minha estação → Importar autorização**.
2. Selecione o `.zb2issuer` recebido para a estação daquele integrante.
3. Abra **Licenças** e emita dentro dos limites concedidos.

Você pode reenviar a autorização pela lista **Equipe → Estações autorizadas →
Salvar autorização selecionada**. Renovar a autorização não invalida uma anterior
que ainda esteja dentro do prazo. Não há revogação instantânea offline.

## Não confundir os arquivos

**Cliente:** envie **CLIENTE.zip** e o `.zb2license` gerado para o computador dele.
**Integrante emissor:** envie **EQUIPE.zip**; após receber a solicitação dele,
devolva o `.zb2issuer`. Um integrante que apenas joga recebe o pacote de cliente.
Não envie a pasta build, a chave principal ou a pasta de dados de uma estação.

| Arquivo | Destino |
|---|---|
| `.zb2license` | Cliente: ativação no ZB2 Menu |
| `.zb2station` | Proprietário: solicitação pública de uma estação da equipe |
| `.zb2issuer` | Integrante: autorização para emitir naquele PC |
| `.dpapi` | Chave principal protegida: somente o proprietário; não enviar à equipe ou ao cliente |

## Dados e limites desta fase offline

- Os dados do Admin ficam em `%LOCALAPPDATA%/ZB2Admin`. A nova interface lê a
  mesma pasta e preserva as estações, licenças e autorizações já existentes.
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

`scripts/build_admin.py --tests` compila o frontend ImGui com MSVC e o serviço
sem interface com o compilador .NET Framework. `tests/run_admin_validation.py
--key CAMINHO_DPAPI` verifica o emissor e a interoperabilidade C++. O teste
`tests/test_admin_bridge.py --key CAMINHO_DPAPI` percorre os pipes reais entre
o código nativo e o serviço. `build/admin-native/admin-native-tests.exe` renderiza
as telas em 100%, 150% e 200% e verifica navegação/ID completo, sem controlar a tela.

Licenças antigas ZB2L1 permanecem aceitas. As novas ZB2L2 carregam uma assinatura
do integrante e uma autorização assinada pelo proprietário. O loader confere
produto, identidade, assinatura, prazo da autorização e duração da licença.
