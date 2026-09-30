# Como enviar o DEADBLOCK: passo a passo

## Primeiro: escolha quem vai receber

- **Cliente** é quem vai jogar. Recebe o ZB2Menu.exe.
- **Equipe** é quem vai emitir licenças para outros clientes. Recebe o ZB2Admin.exe.
- Se este PC já está autorizado como proprietário, não crie uma solicitação de integrante para si.

## Cliente novo: ainda não tenho o ID

1. No Admin, abra **Como enviar → CLIENTE → Salvar programa inicial**.
2. Salve `01-CLIENTE-INICIAL.zip` e envie esse ZIP ao cliente. **Não precisa de ID.**
3. O cliente extrai o ZIP, abre `ZB2Menu.exe` e clica em **Copiar ID**.
4. Ele manda o ID para você por mensagem. O programa abre sem licença; o acesso
   ao menu só é liberado depois da ativação.
5. No seu Admin, abra **Clientes**, preencha nome, ID completo e dias de uso.
6. Clique em **Gerar ZIP do cliente**, salve `02-CLIENTE-ATIVADO.zip` e envie esse
   segundo ZIP ao mesmo cliente.
7. Ele extrai o segundo ZIP e abre o `ZB2Menu.exe` que veio nele. A licença incluída
   é reconhecida automaticamente. Não precisa digitar uma chave nesse caminho.

O primeiro ZIP contém somente o programa. O segundo contém programa + licença.
O prazo começa quando você emite a licença, não quando salva o programa inicial.
Não emita uma licença antes de obter o ID real do cliente.

## Integrante novo da equipe: ainda não tenho a solicitação

1. Abra **Como enviar → EQUIPE → Salvar programa inicial**.
2. Envie `01-EQUIPE-INICIAL.zip` ao integrante.
3. Ele extrai, abre `ZB2Admin.exe` e entra em **Meu acesso**.
4. Ele informa o próprio nome, clica em **Criar solicitação**, salva o arquivo
   `.zb2station` e envia esse arquivo para você.
5. No SEU Admin, abra **Minha equipe → Abrir solicitação**.
6. Abra o arquivo recebido e defina por quantos dias o integrante poderá emitir
   e qual será o prazo máximo de cada licença que ele emitir.
7. Clique em **Autorizar e gerar ZIP**, salve `02-EQUIPE-AUTORIZADA.zip` e envie
   esse segundo ZIP ao integrante.
8. Ele extrai o segundo ZIP e abre o Admin no mesmo PC que criou a solicitação.
   A autorização é reconhecida automaticamente.

Não envie seu perfil de proprietário. O integrante cria sua própria chave no PC
dele; você envia somente a autorização. Quem só vai jogar recebe o pacote CLIENTE.

## Onde estão os arquivos prontos

Também existem duas cópias iniciais prontas em `dist/entrega/`:
`01-CLIENTE-INICIAL.zip` e `01-EQUIPE-INICIAL.zip`.
Os botões de **Como enviar** funcionam mesmo no Admin distribuído sozinho;
não dependem da pasta do projeto e não enviam nada pela internet automaticamente.

ZIPs personalizados são salvos onde você escolher; o destino inicial é
`Documentos/ZB2Menu/Pacotes`. Um arquivo salvo não foi enviado: anexe-o na conversa
com a pessoa que deve recebê-lo.

## Reenviar ou renovar

- Para reenviar a mesma licença, use o ícone ZIP na linha do histórico. Isso não
  reinicia o prazo nem cria outra licença.
- Para renovar, emita uma nova licença com o ID e o prazo desejados.
- Para recuperar o ZIP da equipe, clique com o botão direito na estação autorizada.
- A opção de copiar e colar a chave continua disponível, mas o ZIP ativado é mais simples.

## Arquivos que nunca devem ser enviados

Não envie `.local`, a pasta `Documentos/ZB2Menu/Admin`, arquivos `.dpapi`, histórico,
backups, fontes ou builds. Use apenas os ZIPs produzidos pelos botões acima.
Cada PC mantém seu histórico local; não há sincronização nem revogação instantânea offline.

## Limpeza no painel

Em Configurações, clique em Analisar resíduos. O painel mostra a quantidade e o espaço.
Se houver arquivos elegíveis, Limpar analisados remove apenas serviços antigos do Admin.
O serviço atual, dados pessoais, licenças, backups, logs e runtime do jogo são preservados.
Arquivos que mudaram ou ficaram em uso depois da análise também são preservados.
Não apague a pasta Documentos/ZB2Menu inteira. Consulte ARQUITETURA.md.
