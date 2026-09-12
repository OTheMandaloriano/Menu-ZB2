# CHANGELOG.md (ZB2 Menu)

## [v0.3.0] - 2026-09-12
- feat: sistema de hotkeys com modal (Fase 1 item 3)
- HotkeyButton(): modal captura proxima tecla/mouse (borda de subida), ESC cancela
- Nomes via GetKeyNameTextA + labels p/ botoes do mouse; WndProc usa Config::iMenuKey (DELETE de fallback)
- Hotkeys: Menu, Aim, Enemy Magnet, Item Magnet na aba SETTINGS
- Build Release|x64 0 erros (DLL SHA a70a5c12)

## [v0.2.1] - 2026-09-12
- fix: ordem janela-antes-do-hook + guarda !g_hWindow (menu invisivel) - TESTADO OK
- Incidente: eject remoto com hook ativo derrubou o jogo -> updates exigem restart

## [v0.2.0] - 2026-09-12
- feat: injetor C++ automatico (LoadLibrary remoto, validacoes) - TESTADO OK

## [v0.1.0] - 2026-09-12
- feat: base D3D11 adaptada p/ Unity 6 Mono x64, menu 4 abas, preview drag-and-drop
