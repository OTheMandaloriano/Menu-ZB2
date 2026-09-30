# Estados e orientação do cliente

## Correção da leitura do jogo

O código anterior mantinha erros de monitoramento no texto de uma ação e podia
continuar mostrando a falha depois de confirmar o menu. A versão 1.14 trata essas
falhas como observações temporárias, limpa o erro anterior ao confirmar o menu e
bloqueia uma nova injeção quando a leitura atual não está confirmada.

A leitura de módulos usa um único snapshot para menu, monitor e Mono, com tentativas
limitadas em caso de lista temporariamente inconsistente. Falhas exibem o código
Windows. Não elevam privilégios automaticamente nem desativam proteção.

## O que o usuário recebe

| Situação | Mensagem ou orientação |
|---|---|
| Sem licença | Abrir o arquivo recebido ou solicitar acesso à equipe |
| Licença inválida, expirada ou de outro PC | Motivo retornado pela validação; solicitar correção/renovação |
| Jogo fechado | Aguardando o Zumbi Blocks 2 |
| Jogo iniciando | O jogo está iniciando. Aguarde carregar |
| Fora de uma partida | Entre em uma partida para carregar o menu |
| Cena carregando | Aguardar prontidão |
| Leitura temporariamente indisponível | Verificação pendente; botão de injeção desabilitado |
| MOD-5 | Acesso negado; abrir jogo e loader no mesmo nível de permissão |
| MOD-24 / MOD-299 | Lista de módulos indisponível no momento; reavaliar na próxima consulta |
| Menu confirmado | Menu carregado. Use INSERT no jogo |
| Resultado da tentativa não confirmado | Reiniciar o jogo antes de nova tentativa |
| Pacote inválido ou alterado | Obter novamente o pacote correto; não carregar componentes não verificados |
| Versão do jogo incompatível | Atualizar o menu para uma versão compatível |

Erros têm o botão **Detalhes**, com a mensagem completa e **Copiar detalhes**.
O texto copiado contém versão, PID e mensagem; não contém chave de licença ou HWID.

## Dependências e limites

O runtime é incorporado no cliente: menu nativo, ponte gerenciada, Harmony, monitor
de prontidão e helper. Não é necessário copiar DLLs avulsas de sites de download.
Os executáveis nativos usam o runtime C/C++ estático. O Admin depende do .NET Framework
do Windows para o serviço local; o jogo fornece o seu próprio Unity Mono.
A interface usa DirectX 11 e precisa de um Windows x64 e driver compatíveis.

As validações de integridade e versão do jogo já existem. Não há ainda um instalador
universal que baixe/repare automaticamente dependências do Windows. Esse recurso
deve usar somente instaladores oficiais, com verificação e consentimento.

## SmartScreen

O aviso de aplicativo não reconhecido e fornecedor desconhecido indica uma decisão
de reputação do Windows. Um resultado sem ameaças na varredura local não remove esse
aviso. Assinatura Authenticode e distribuição consistente ajudam a identificar o
publicador, mas não garantem aprovação imediata do SmartScreen.
