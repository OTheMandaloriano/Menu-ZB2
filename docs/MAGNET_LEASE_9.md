# MAGNET-LEASE-9

O estado NoClip fazia parte da chave que invalidava o destino. Agora os modos
fixos guardam posição e escolha chão/ar na ativação. NoClip posterior não altera
essa âncora. Seguir continua seguindo a câmera quando voando, por definição.

ForceLoadRealZombie sozinho não impedia o LOD normal de descarregar o modelo
novamente. MagnetLoadLease mantém IDs promovidos no estado Real pelo hook de LOD
já existente, sem depender do FOV/ativação da mira. As promoções seguem em lotes;
a retenção termina ao desligar ou após expirar o heartbeat. Não altera goActive,
estado de nascimento ou autoridade. Pedidos de promoção não bloqueiam para sempre
novas tentativas após um modelo ter sido efetivamente carregado.

Tooltips explicam raio de captura centrado no jogador, destino fixo, alvos,
agrupamento e distância separada de bosses. Motivos de espera incluem nascimento,
carregamento e ausência de destino seguro. O raio não equivale a mover todos em
um único frame.

O travamento ao disparar ainda não foi reproduzido com medição da mesma horda.
AimBridge agora mede LastAimCheckMs separadamente de LastOriginalShotMs e
PeakOriginalShotMs, para distinguir a checagem do Silent Aim da execução original
de tiro/colisão/dano. Não altera dano nem remove colliders para aparentar rapidez.

Nas capturas de HP ausente o jogo informa CLIENTE. A revisão mantém a limitação
numérica já auditada em vez de restaurar 100% local fictício. Solo/host e bosses
continuam usando as fontes numéricas disponíveis; uma falha nesses cenários requer
reprodução específica.

49 testes de Magnet (incluindo toggle NoClip em âncora fixa), 22 testes Harmony
de LOD (incluindo mira desligada e expiração), 30 regressões de disparo. Build
gerenciado/nativo. Retenção com horda e custo de tiro precisam de teste em partida.
