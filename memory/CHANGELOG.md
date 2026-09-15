# CHANGELOG.md (ZB2 Menu)

## [não lançado] - 2026-09-15
- perf: orçamento de 220 invokes/ciclo + LOS em rodízio (perto sempre, longe 1/3) + cull fora da tela + teto 96 ent/ciclo (anti-crash em horda, crash 15:51 LODController)
- fix: watchdog SceneAlive no respawn + probe SEH na câmera + TryEnter nos locks + Map DO_NOT_WAIT (anti-hang tela branca)
- fix: offset-16 no hitBuf cru (m_Distance raw=28, não 44) + histerese LOS anti-flicker (~3 ciclos)
- fix: P0 crash pós-kill (GetName fora do caminho quente) + sem invoke em collider morto do LOD
- feat: auditoria [SIG]/[FIELDS] (Raycast/5 + Linecast/4 confirmados, offset via API)

## [v0.6.0] - 2026-09-12
- feat: debug overlay com dados vivos + leitura de entidades (Fase 2 item 6)
- WalkList<T> com validacao (size, vetor, probe) + cruzamento totalRealZombies
- Local via mono_runtime_invoke(get_HasLocalControl); HP/stamina/aliado/zumbis/day
- Overlay: LOCAL HP/STAM/ALIADO/ZUMBIS/DAY no canto (fora do menu)
- Build Release|x64 0 erros (DLL SHA af9052f5)

## [v0.5.0] - 2026-09-12
- feat: reflection C++ via binding dinamico - TESTADO IN-GAME OK

## [v0.4.0] - 2026-09-12
- feat: configs JSON com presets - TESTADO IN-GAME OK

## [v0.3.0] - 2026-09-12
- feat: hotkeys com modal - TESTADO IN-GAME OK

## [v0.2.1] - 2026-09-12
- fix: ordem janela-antes-do-hook - TESTADO OK

## [v0.2.0] - 2026-09-12
- feat: injetor C++ automatico - TESTADO OK

## [v0.1.0] - 2026-09-12
- feat: base D3D11 adaptada p/ Unity 6 Mono x64, menu 4 abas, preview drag-and-drop
