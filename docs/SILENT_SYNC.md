# Silent Aim e sincronização do disparo

## Causas confirmadas

1. CanRedirectShot aceitava apenas SinglePlayer; AimBridge também verificava IsSinglePlayer. Host e cliente eram bloqueados pela implementação, não por uma impossibilidade demonstrada do jogo.
2. O Prefix em PhysicalGun.Shoot recebia uma cópia da struct ShotPath. O tiro local era alterado, mas PlayerArms.ShootGun preservava a variável antiga usada depois em SyncShotOnline.
3. A coleta limitada por tempo começava sempre no primeiro inimigo. Sob carga, candidatos no fim da lista podiam nunca chegar à seleção, inclusive no 360°.

## Alteração

Um transpiler Harmony substitui a chamada PhysicalGun.Shoot dentro de PlayerArms.ShootGun por um adaptador com ShotPath por referência. O adaptador valida alvo/obstáculos, altera a direção na variável local e chama o disparo original uma única vez. O código original de ShootGun depois sincroniza a mesma direção. Não existe envio adicional ou uma segunda rotina de tiro.

O casamento da chamada exige assinatura e argumentos conhecidos; se o método mudar, a instalação falha com diagnóstico. A build gerenciada continua verificando o SHA-256 de Assembly-CSharp.dll.

Silent é habilitado em solo, host e cliente quando existe jogador local válido e ativação por tecla/Auto Aim/Auto Fire. Menu aberto, foco perdido, alvo morto, pedido antigo ou obstáculo suspendem o redirecionamento. Armas com fluxo separado (como lançadores) continuam fora dessa implementação.

O modo 360 usa direção/ângulo 3D sem exigir projeção na tela. A coleta rotaciona o índice inicial quando o orçamento termina. Consultas de visibilidade usam o adaptador gerenciado, que ignora os colliders do próprio jogador.

## Evidência e limites

IL confirmado pelo DnSpyMCP: PlayerArms.ShootGun armazena ShotPath na local 1, passa-a para PhysicalGun.Shoot e depois usa convergingDirection na chamada SyncShotOnline. O transpiler descobre a local na instrução, em vez de fixar o índice 1.

Os testes usam Harmony real e um fluxo simulado de ShootGun que consome munição, executa Shoot e registra a direção sincronizada. Confirmam uma chamada original e direção compartilhada, alvo atrás, parede, vida, foco e preservação de tiros remotos. Isso não substitui teste com dois processos conectados.

Logs para validação: `[AIM-SELECT]` mostra modo, 360, Silent, ativação, candidatos e quantos estão atrás; `[AIM-BRIDGE]` mostra redirecionamentos e bloqueios. A inicialização registra `ShotPath compartilhado com SyncShotOnline`.

Não há garantia de dano remoto antes de validar host e cliente em partida. Documentos anteriores que dizem que sincronização implica impossibilidade de Silent online estão superados por esta análise.
