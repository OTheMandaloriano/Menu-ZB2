# AUTO-INJECT — revisão 1.3

O loader monitora o processo em uma worker, inclusive minimizado. AUTO-INJECT
vem ativado por padrão e a preferência é salva em `auto-inject.txt`, junto ao
estado do loader. O botão manual solicita uma tentativa quando a cena estiver
pronta; não ignora as verificações de prontidão, assinatura e versão.

## Sequência

1. Licença válida e uma única instância do jogo, identificada por PID e horário
   de criação. A mera presença do processo não autoriza carregar o menu.
2. Acompanhar incrementalmente `Player.log` do perfil Windows. Exigir caminho
   Managed correto, carregamento dos assemblies e fim do reset do domínio Mono.
   Log antigo, truncado, incompleto ou referente a outro caminho não libera.
3. Instalar o pacote assinado e carregar `ZB2.Readiness.dll` uma única vez nesse
   processo. Essa sonda não instala hooks, não invoca métodos Unity, não altera
   gameplay e não lê endereços de heap fixos.
4. A sonda usa exports Mono para resolver `ZBMain.instance`, `mapIsLoaded`,
   `MainCamera.instance` e `ZombieLoader.Instance`. Publica estado em memória
   compartilhada, identificado pelo processo e com timestamp/sequência.
5. Exigir todos os estados válidos por pelo menos 750 ms, amostra recente,
   evento de entrada no mundo no log e nenhuma limpeza posterior. Revalidar
   licença e estado imediatamente antes de executar o helper.
6. Carregar o menu principal uma vez. A sonda encerra sua worker após detectar
   a DLL principal; permanece mapeada até o processo terminar. Não reinjetar.

Desligar o toggle cancela a intenção automática antes de iniciar o helper.
Uma operação já iniciada termina normalmente; não se encerra uma thread remota
ou o jogo para fingir cancelamento. Falha não produz repetição automática.
O cliente deve reiniciar o jogo após uma tentativa não confirmada.

## Evidência estática e limites

Assembly auditado: `c41a298975d35f0dad0a05531bce6e0b6e274d0ddf265217d65ce3ac5cbc84e1`.
`ZBMain.OnEnteredMap` (MethodDef 0x06000014) define `mapIsLoaded = true`;
`ZBMain.CleanUp` (0x06000015) limpa esse campo. `MatchController.StartMatch`
registra o início antes da geração do mapa; por isso esse log sozinho não basta.
`ClientListener.OnInitialMapInfo` registra o hash antes de chamar OnEnteredMap;
também não basta sozinho. A sonda confirma os campos após esses eventos.

Não há garantia de que todo mapa, mod ou futura versão do jogo esteja coberto.
O módulo principal continua bloqueado por hash da assembly. A prontidão pode
ficar aguardando se o log estiver desativado/redirecionado ou o jogo rodar em
outro perfil Windows. Não se substitui isso por um timer fixo ou clique forçado.

Foram implementados testes de estados, reinício, falha, toggle, limpeza,
identidade do processo, amostra antiga e estabilização. A compilação e esses
testes não equivalem a validação real no jogo: o usuário optou por testar depois.

## Interface 1.3

Loader 440 × 270. Primário azul #2563eb, ícones FA6, secundários com borda fina,
pressionamento com escala 0.98. No Admin, cada linha mostra validade, badge e
cópia imediata. “Ativa” significa dentro da validade, não ativação confirmada
no computador do cliente. Datas usam células numéricas de avanço fixo; não se
declara que uma propriedade CSS `tnum` é executada pelo Dear ImGui.

## Validação em partida — 30/09/2026

PID 7964: sonda confirmou assemblies=1, map=1, objects=1 e unsupported=0.
O fluxo do loader carregou kiero-dx11-base.dll às 01:28:32, confirmado na lista
de módulos; debug_log registrou Unity Update e entidades ativas. A tela inicial
com Tecle ENTER não é uma partida e deve aguardar. Corrigida mensagem do helper
que anunciava INSERT ao carregar somente a sonda. Não houve reinjeção da DLL.
