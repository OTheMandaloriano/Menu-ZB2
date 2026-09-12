# CHANGELOG.md (ZB2 Menu)

## [v0.5.0] - 2026-09-12
- feat: reflection C++ na DLL via binding dinamico (Fase 2 item 5)
- mono.h/mono.cpp: GetModuleHandle(mono-2.0-bdwgc.dll) + 8 funcoes, attach, resolve
  de 12 classes + 3 campos + 9 metodos (nomes da auditoria #1)
- Leitura viva: DaytimeController.instance -> curTime/dayLen com SEH, throttle 30f
- Mono::Tick() no Present; sem link contra o Mono
- Build Release|x64 0 erros (DLL SHA 45b3df07)

## [v0.4.0] - 2026-09-12
- feat: configs JSON com presets - TESTADO IN-GAME OK

## [v0.3.0] - 2026-09-12
- feat: hotkeys com modal - TESTADO IN-GAME OK (Aim=X)

## [v0.2.1] - 2026-09-12
- fix: ordem janela-antes-do-hook - TESTADO OK

## [v0.2.0] - 2026-09-12
- feat: injetor C++ automatico - TESTADO OK

## [v0.1.0] - 2026-09-12
- feat: base D3D11 adaptada p/ Unity 6 Mono x64, menu 4 abas, preview drag-and-drop
