# GROUP-MELEE-7

## Um grupo, um destino

O raio é aplicado para capturar novos inimigos, não para esquecer membros do
grupo anterior. IDs atraídos são mantidos enquanto o mapa é o mesmo. Uma nova
ativação em B pode recolher membros de A fora do raio; membros descarregados
passam pela rotina nativa ForceLoadRealZombie, em lotes. Ao substituir um ponto
fixo, a retenção anterior é liberada antes de prender no novo destino. Isso não
significa transferir uma horda inteira num único frame.

Com NoClip ativo, o ponto pela mira também pode ser capturado no espaço à frente
da câmera, em vez de exigir chão. Um ponto fixo não é revalidado contra a direção
atual da câmera a cada movimento posterior; sua localização continua fixa.

## Fast Knife

O dnSpy mostrou que MeleeAttack inicia a animação e PlayerMeleeState acompanha
seu normalizedTime. Alterar Duration no catálogo não controlava essa progressão.
Agora somente o Animator do jogador local é multiplicado enquanto executa o
clipe do ataque atual. Sair do clipe ou desativar restaura a velocidade original.
Não altera dano, stamina ou o catálogo compartilhado de golpes. A execução em
partida ainda precisa ser comparada com a função desligada.

## Cobertura da mira e custo visual

ReadAimBone rejeitava rigs maiores que 64 ou sem nomes esperados. Agora usa a
referência zombieEyeRef oferecida pelo jogo quando o osso não pode ser obtido;
para cabeça em rigs diferentes ela também evita a busca repetida por nomes.
Distância, visibilidade, estado vivo e política de seleção continuam valendo.
Esse fallback pode mirar nos olhos em vez do osso escolhido se o rig não o expõe.
Ainda precisa ser verificado nos bosses concretos relatados pelo usuário.

O limite temporal da coleta visual também é verificado dentro dos loops de
ossos, inclusive rigs não padrão. Isso reduz estouros por trabalho acumulado;
não interrompe uma chamada Unity individual lenta nem garante FPS com hordas.

Para comparar desempenho, usar a mesma horda: menu sem ESP, ESP sem esqueleto,
ESP completo, depois Magnet. A alternativa menos custosa é reduzir detalhe de
ossos/marcações; limitar inimigos processados aumenta o tempo para reunir o grupo.

## Pendências explícitas

HP numérico de aliados continua indisponível pela rota auditada. As leituras CE
mostraram valores padrão de 50/75, inclusive em aliado caído; isso não é porcentagem
real. Nada nesta revisão implementa sua sincronização. Tampouco a medição anterior
de 0,67 ms do Magnet representa a horda da captura com ESP de 96,1 ms.

Validação local: 45 testes de Magnet, 233 de modificadores e 44 de ESP de mundo;
build gerenciado/nativo. Testes de mods simulados não confirmam todos os rigs ou
a taxa de quadros da partida do usuário.
