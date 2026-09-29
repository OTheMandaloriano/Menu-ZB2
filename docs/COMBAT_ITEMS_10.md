# COMBAT-ITEMS-10

Os controles da antiga área desabilitada foram auditados via dnSpy MCP.
Power Drop não tinha definição nem implementação; o usuário também não reconhece
o efeito. Foi removido da interface e da configuração, sem inventar uma ação.

## Saitama

Aplica somente aos quatro socos desarmados identificados por PlayerMeleeAttackID,
com clipe de ataque correspondente ativo, jogador local e alvo Zombie. Exige
solo/host. Um prefixo em Damage.ProcessDamage clona o dano e atribui 4 milhões;
não altera o objeto de dano original, tiros, facas ou jogadores remotos.
Libera a retenção do Magnet e aplica velocidade para frente/cima. O ragdoll que
recebe CopyArmature do mesmo ZombieObject recebe o impulso uma única vez.

Ainda não foi comprovado em partida que todo boss morra de um golpe ou que todo
rig voe da mesma forma: proteções do boss e lógica de morte continuam no jogo.
Desligar impede novos socos especiais, mas não desfaz mortes ou trajetórias.

## Explosivos

Modificar explosivos é a ativação explícita. Pavio aceita 0,1–10 s, raio 1–30 m,
dano máximo 10–10000. Apenas Frag e Dynamite são alterados. Contato usa o campo
contactDestroy, cujo caminho nativo detona após colisão. Sliders recalculam a
partir do valor original; dano mínimo e orçamento total preservam suas proporções.
Desligar/restaurar autoridade restaura os parâmetros. O catálogo só é reaplicado
quando mudam controles ou referências, não é percorrido novamente a cada frame.

É uma alteração do catálogo na instância solo/host: pode afetar explosivos de
outros jogadores e os que já estão lançados. Não promete a mesma simulação em
clientes sem o mod, pois a mensagem nativa de explosão transmite o ID, não esses
parâmetros personalizados. Nenhum bypass de autoridade foi implementado.

## Inventário

Modal pesquisável com catálogo permitido, seleção única, imagens disponíveis e
quantidade. A ação usa FindPlaceFor e PutLootIntoPosition. Pilhas respeitam stackMax;
armas/equipamentos não empilháveis são uma unidade por clique. Não há inserção
forçada em slots nem substituição do equipamento existente. Se não há espaço,
nada é adicionado ou descartado. Itens bloqueados/reservados continuam excluídos.
Pedido é consumido uma vez e descartado durante loading ou sem autoridade.
Adicionar não é modificador reversível: o item permanece ao desligar a função.

## Validação

18 testes com hooks Harmony: aplicação, slider, restauração, autoridade perdida,
itens bloqueados, inventário cheio, pilha, pedido repetido/loading, soco, impulso
no ragdoll, faca/tiro não afetados e desativação. Builds gerenciado/x64 e regressões
da mira/overlay passaram. A integração em partida e a física visual ainda requerem
validação após instalar/injetar; nenhum soco, explosão ou item foi acionado
automaticamente na partida do usuário durante o desenvolvimento.
