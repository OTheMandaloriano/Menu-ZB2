# ESP-RANGE-8

O Magnet anteriormente promovia somente bosses e membros já capturados. Agora
também promove registros comuns dentro do raio, em lotes, usando a rotina nativa
ForceLoadRealZombie. Continuam os filtros de alvo, autoridade, nascimento e destino
válido; 300 m não garante mover instantaneamente tudo nem remover obstáculos.

Chams e silhueta ficam junto ao ESP dos zumbis. Seu master e raio controlam os
efeitos; a distância usa o jogador local quando disponível. O ponto da cabeça
ganha coleta independente de Skeleton. Sem esqueleto, usa a referência de olhos
do jogo como marcador de cabeça, sem consultar o restante do rig.

Bosses passam a fornecer envelope dos renderizadores do modelo para caixas 2D,
cantos e 3D. Rigs não padrão usam humanoid mapping quando disponível ou nomes
normalizados explícitos de cabeça/pescoço/peito/pelve. Ausência de pescoço, peito
ou pelve não vira cabeça silenciosamente. Head pode usar zombieEyeRef. A cache
tem chaves fracas e é renovada para acompanhar objetos/rigs alterados. Nomes
específicos desconhecidos ainda exigem auditoria do boss concreto.

Na leitura CE da sala desta revisão havia cinco jogadores: quatro registros
remotos em healthFast=50 e healthSlow=75; o local em 133,3/133,3. Não foi encontrada
uma fonte numérica remota confiável. Barra/% real de aliados permanece pendente;
não se apresenta essa limitação como corrigida.

Testes locais: 47 Magnet, 21 ciclo de vida de efeitos, 43 overlay/presets e seis
mapeamento de rigs/envelope. Builds gerenciado e x64. Não substituem validação
dos bosses reais nem comprovação de HP remoto.
