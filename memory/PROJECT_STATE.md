# PROJECT_STATE.md - Estado atual (ZB2 Menu)

## Auditoria runtime #1 (2026-09-12, partida ao vivo) - CONCLUIDA
- Assembly-CSharp catalogado (1358 classes), offsets validados: Player HP/stamina,
  Zombie HP chain, Daytime curTime, local x aliado via HasLocalControl.
- Ver memory/OFFSETS.md, CLASSES_UTEIS.md, dump/sessao-2026-09-12_00-05.txt.

## Fase 1 - Infraestrutura
- [x] 1. Hook D3D11 + ImGui funcional - TESTADO IN-GAME (Present 8 OK, GUI OK, WndProc OK)
- [x] 1b. Injetor C++ automatico (LoadLibrary remoto, auto-DLL, hash/timestamp, admin/validacoes) - TESTADO (PID 25388, HMODULE remoto OK)
- [x] 2. Sistema de log - VALIDADO (Documents\kiero-dx11-base\logs\debug_log.txt escreve)
- [ ] 3. Hotkeys com modal
- [ ] 4. Configs JSON

## Fase 2 - Leitura (sem render)
- [x] 5a. Reflection Unity Mono (enumeracao + invocacao OK via CE; falta C++ mono API na DLL)
- [ ] 6. Debug overlay (Local HP, zumbis detectados)

Metodologia: 1 funcao por ciclo, commit pt-BR, teste in-game antes de avancar.
