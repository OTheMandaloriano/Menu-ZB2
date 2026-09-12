# ARQUITETURA.md — Respostas P0 (auditoria de arquitetura do ESP)

Data: 2026-09-12. Referências: `mono.cpp` (worker), `main.cpp` (hkPresent), `gui.cpp` (render).

## 1. Onde os dados do ESP são lidos?
**Worker thread separada (`EspThread`, 30Hz), nunca no `hkPresent`.**
`BuildEsp()` lê TODOS os campos de TODAS as entidades de uma vez (lista →
HP → nome → eye/foot ou bounds → W2S). Uma leitura por entidade por ciclo,
compartilhada por todos os elementos (box, nome, dist, vida, %).

## 2. Existe snapshot?
**Sim.** `s_esp[128]` sob `CRITICAL_SECTION`; o render consome via
`Mono::GetEsp()` (cópia). Transições logadas (`[ESP+]`/`[ESP-]`). O render
nunca toca na memória do jogo.

## 3. VP própria ou do motor?
**Própria (desde `a9576c4`).** `worldToCameraMatrix` + `projectionMatrix`
lidas 1x por ciclo, `VP = P*V` no nosso código, `clip.w` com guard nosso
(`> 0.05`). Fallback = motor com `z > 1m`. Sim, vemos o `clip.w` antes de dividir.

## 4. Frame counter / tick ID?
**Não há.** Worker 30Hz vs render 60fps: snapshot tem até ~33ms de idade por
design (documentado). Não é fonte de glitch visual (posições continuam válidas).

## 5. Hook duplo de Present?
**Bind único** (`kiero::bind(8)` uma vez no `MainThread`) + guard
`DXGI_PRESENT_TEST` (não desenha em present de teste). Sem evidência de flicker
geral; se surgir, o próximo passo é contador de chamadas por frame.

## Conclusão P0
Não há race condition estrutural: snapshot + gather separado + VP própria +
SEH por acesso + filtros no gather. Glitches restantes combatidos com
validação em camadas + log diagnóstico. Double-buffering com frame counter
fica como evolução se residual aparecer.
