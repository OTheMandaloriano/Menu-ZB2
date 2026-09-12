# CHANGELOG.md (ZB2 Menu)

## [v0.2.1] - 2026-09-12
- fix: corrige ordem de inicializacao (janela ANTES do hook Present) + guarda !g_hWindow no hkPresent
- Causa (diagnostico do operador): bind(8) antes de GetProcessWindow -> ImGui com HWND nulo -> DisplaySize (0,0) -> menu invisivel, cursor visivel
- Build Release|x64 0 erros (DLL SHA d48ff275)
- Incidente: eject remoto via FreeLibrary com hook ativo derrubou o jogo -> updates exigem restart do jogo

## [v0.2.0] - 2026-09-12
- feat: injetor C++ automatico (injector/injector.cpp + injector.vcxproj)
- Auto-localizacao da DLL, deteccao de update, LoadLibrary remoto, validacoes
- Teste in-game: PID 25388, HMODULE remoto OK, hook + GUI + WndProc no log

## [v0.1.0] - 2026-09-12
- feat: base D3D11 a partir de kiero-dx9-base (GitHub) adaptada p/ Unity 6 Mono x64
- Hook Present (8) + ResizeBuffers (13), ImGui 1.89.9 DX11, menu 4 abas
- Preview interativo drag-and-drop + health bar auto-orientada + cor por HP
