# Auditoria de modificadores e ESP — 26/09/2026

## Defeitos confirmados e corrigidos

- Box desmarcado caía no `else` do fallback 3D. Agora todas as formas exigem o checkbox Box.
- Snapline ignorava `iSnapFrom`: Base, Topo e Centro agora afetam a origem efetiva.
- Itens e pontos do mundo tinham interface/configuração, mas nenhum produtor ou renderizador. Adicionado snapshot separado, sem dependência de zumbis/aimbot.
- Caches nativos de restauração tinham capacidades fixas (16 armas), ponteiros não protegidos pelo GC e eram zerados nas transições sem restaurar assets compartilhados.
- Fast Knife dividia valores já modificados, acumulando aceleração; No Sway tinha estado estático sombreado e restauração booleana com tamanho incorreto.
- Full Auto tinha checkbox separado sem aplicação própria. Agora seu controle é independente de Rapid Fire.
- Infinite Items/Ammo elevavam silenciosamente o `stackMax` compartilhado de certos itens de 20 para 50, sem restauração. Removida essa alteração de limite; reposição usa o limite do jogo.

## Contrato de restauração

`OriginalValues` retém referências gerenciadas e captura cada membro antes da primeira escrita. Todo ajuste parte desse original; mover de 5x para 2x produz original × 2. Desligar restaura; objetos que saem do inventário são restaurados; troca de jogador/cena restaura antes de descartar o registro. Falhas mantêm o original para nova tentativa. Não existe limite artificial de 16 registros.

Aplicado a Rapid Fire (rof), Full Auto (fullAuto/burstCount), No Recoil (Vector2/randomness), No Spread, Tight Aim (spread/recoil/escala real do Transform da mira), No Sway (gunSway), Speed, Super Jump (jumpSpeed e threshold negativo), Roll Speed e Fast Knife (Duration). Duration usa a propriedade real, sem offsets presumidos de dicionários/structs. Transições normalizadas dos golpes permanecem no padrão do jogo.

Todos executam no callback Unity Update, nunca no Present. Não há alteração do protocolo de rede nesta revisão. O mesmo adaptador é usado em solo, host e cliente. Isso não comprova que o servidor aceite cada efeito: validação nas três partidas permanece necessária.

A primeira captura precisa ocorrer em processo novo: uma DLL não pode recuperar retroativamente o original já perdido pela versão anterior.

## ESP

- Itens: `MapHash.GetCell(...).loot`, inclusive objetos fora do alcance visual do LOD, desde que existam localmente; nome traduzido, categoria, distância, cor, raio. Até 512 candidatos próximos, projeção atualizada por ciclo e descoberta a cada 500 ms ou mudança de filtro/raio.
- Raros: tier 3 ou superior, categoria adicional, sem duplicar desenho da mesma arma.
- Pontos: lista real de `InterestPointController`, posição dinâmica por Transform ou pos3D. Helicóptero, boss, bomba/ponto desconhecido, loot fixo, bancadas, fogueira, mercador e respawn. Somente pontos disponíveis no cliente; não inventa dados ausentes da sincronização.
- Até 256 marcadores projetados; rejeita atrás da câmera, coordenadas inválidas e fora da tela. Snapshot expira em 500 ms e é limpo na transição. Filtros também são respeitados pelo renderizador imediatamente após desativar.
- O controle `%` é independente da barra de vida. Precisa estar desmarcado para ocultar o percentual.

## Pendências identificadas pela auditoria

Não confundir falta de implementação com problema no slider: Saitama, explosões, entrega de itens por botões, aliados, chams, ímãs, teleporte, spawn manual, hora/velocidade do mundo, FOV de câmera, terceira pessoa, anti-AFK, noclip, no-fall e revive não tinham execução ligada aos controles. Permanecem explicitamente desabilitados como pendentes. O marcador direcional de onda também está pendente; as ondas escolhem pontos de spawn dinamicamente, não há ponto de interesse equivalente pronto.

HP/stamina, munição, itens, moedas e desbloqueios ainda são rotinas legadas separadas. Desligar cessa a escrita; não desfaz consumo, compras, dano evitado, loot recebido nem progressão. Reverter esses eventos para uma fotografia antiga destruiria alterações legítimas da partida. A migração completa dessas rotinas e sua sincronização não está concluída. Recarga instantânea ainda possui restrição de cliente; não afirmar suporte universal.

## Verificação

- Compilação Release x64 e compilação gerenciada contra o SHA-256 exato do Assembly-CSharp instalado.
- Testes do adaptador de modificadores: sequência 5x → 2x → 1x → off, repetição sem acúmulo, bool/int/Vector2/Transform, sobreposição de funções, 71 armas, retirada de arma, troca de objeto/jogador/cena, original retido após falha de restauração.
- Testes reais do coletor com objetos simulados: filtros, raros, raio imediato, mestre independente, projeção, proximidade, jogador morto, ABI UTF-8, capacidade e sentinela contra escrita fora do buffer.
- Regressões existentes de aim/Harmony, snapshots e exclusão entre renderização/Unity; teste das opções Box/origem.
- Nenhum teste simulado substitui teste dentro da partida. Não houve controle da tela.
