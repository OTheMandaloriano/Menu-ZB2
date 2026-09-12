# CHANGELOG.md (ZB2 Menu)

## [v0.2.0] - 2026-09-12
- feat: injetor C++ automatico (injector/injector.cpp + injector.vcxproj)
- Auto-localizacao da DLL (relativo + fixo dev), deteccao de update (size+mtime em Documents\ZB2 Menu\injector_state.txt)
- LoadLibrary remoto via CreateRemoteThread; validacoes: admin, SeDebugPrivilege, jogo aberto, alvo x64, DLL x64 (PE check)
- Manifest RequireAdministrator; build Release|x64 0 erros (injector.exe SHA e09af16c)
- Teste in-game: PID 25388, HMODULE remoto OK, hook D3D11 + GUI + WndProc confirmados no debug_log.txt

## [v0.1.0] - 2026-09-12
- feat: base D3D11 a partir de kiero-dx9-base (GitHub) adaptada p/ Unity 6 Mono x64
- Hook Present (8) + ResizeBuffers (13), ImGui 1.89.9 DX11, menu 4 abas PLAYER-VISUAL-MISC-SETTINGS
- Preview interativo drag-and-drop + health bar auto-orientada + cor por HP
- Log isolado em Documents, cursor fix, vcxproj Release|x64 sem DirectX SDK legado
