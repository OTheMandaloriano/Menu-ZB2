# Alcance AIM / ESP — 26/09/2026

## Causa verificada no Assembly-CSharp

`ZombieLoader.GetTargetZombieState` separa inimigos em `zombies` (Real), `zombieProps` e `unloadedZombies`. `sleepDistance` tem padrão 75 m; props dependem de `min(250, fogEndDistance)`. O menu consultava apenas Real. Aumentar o limite não criava registros nessa lista. Esqueleto/head-dot também tinham corte independente fixo de 50 m.

## Correção

- ESP consulta os três estados. Real mantém box/ossos/vida existentes. Prop/Unloaded usam posição conhecida com losango, nome e distância; sem inventar HP, ossos ou tamanho do corpo. Respeita os checkboxes individuais e o alcance do ESP, independente do AIM.
- O renderizador reaplica o limite para ocultar snapshots antigos imediatamente ao diminuir o slider; rejeita também entradas fora da tela. Dados distantes expiram em 500 ms.
- Esqueleto real passa a seguir o limite ESP, mantendo orçamento de invokes.
- AIM mantém até oito candidatos extras e permite no máximo duas novas cargas por passagem normal do LOD. A integração substitui apenas as três chamadas de decisão dentro de `UpdateLoadState`, com validação exata do número de chamadas. Não altera listas durante sua enumeração nem chama `ForceLoadRealZombie` pelo overlay.
- Candidatos já promovidos têm preferência em empate para evitar descarregar/recarregar a cada passagem. Reduzir alcance, desativar ou expirar a solicitação devolve a decisão ao jogo. `goActive` e comportamento original de ondas permanecem intactos.
- A seleção de tiro ainda exige modelo real, HP vivo, FOV/360, distância e visibilidade. Solicitar maior alcance não garante dano: física, arma e servidor continuam impondo suas regras. Registros ausentes da sincronização não podem ser inferidos.
- Corrigido também o esvaziamento do snapshot de itens/pontos quando AIM e ESP de zumbis estavam ambos desligados.

## Verificação

Testes Harmony reais sobre modelos simulados cobrem os três estados, limite de candidatos/cargas, retenção, redução de distância, expiração, FOV versus 360, decisão original de ondas e buffer ABI. Testes do renderizador ImGui cobrem 200→220 m, filtros, ausência de box com checkbox desmarcado e snapshots antigos. Build gerenciada contra assembly exato e Release x64. A validação em jogo dos três modos ainda é pendente; logs `[RANGE]` e `[DISTANCE]` registram a configuração e as contagens observadas.
