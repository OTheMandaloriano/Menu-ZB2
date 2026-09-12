# CHANGELOG.md (ZB2 Menu)

## [v0.4.0] - 2026-09-12
- feat: configs JSON com presets (Fase 1 item 4)
- SaveConfig/LoadConfig(): todo o estado (aimbot, weapon, ESP, preview, magnets,
  teleports, host, utils, hotkeys) em Documents\<DLL>\configs\<preset>.json
- Writer fprintf + parser por chaves, sem libs externas; tipos bool/int/float/vec4/str
- UI em SETTINGS: Preset + Salvar + Carregar + status; chaves ausentes mantem valor
- Build Release|x64 0 erros (DLL SHA fad7ea6f)

## [v0.3.0] - 2026-09-12
- feat: hotkeys com modal (Menu/Aim/Magnets) - TESTADO IN-GAME OK (Aim=X)

## [v0.2.1] - 2026-09-12
- fix: ordem janela-antes-do-hook (menu invisivel) - TESTADO OK

## [v0.2.0] - 2026-09-12
- feat: injetor C++ automatico (LoadLibrary remoto) - TESTADO OK

## [v0.1.0] - 2026-09-12
- feat: base D3D11 adaptada p/ Unity 6 Mono x64, menu 4 abas, preview drag-and-drop
