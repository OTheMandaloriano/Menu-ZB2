#pragma once
#include <Windows.h>

// ============================================================================
// LOG DE DIAGNÓSTICO (padrão release-friendly)
// ============================================================================
// Destino único (ver "Política de Artefatos" no README):
//   %USERPROFILE%\Documents\<NomeDaSuaDLL>\logs\debug_log.txt
//
// Resolução via Known Folder API (SHGetKnownFolderPath + FOLDERID_Documents,
// fallback LocalAppData, fallback ANSI CSIDL). Nunca hardcoded ("C:\..."),
// nunca %TEMP%, nunca pasta do jogo.
//
// Nome da pasta do menu:
//   - Padrão: derivado automaticamente do nome da DLL (sem a extensão .dll)
//   - Customizado: passe explicitamente em Log::SetModule(hModule, "MeuProjeto").
//
// Uso no DllMain:
//   Log::SetModule(hModule); // DLL_PROCESS_ATTACH (só monta strings, sem I/O)
// ============================================================================

namespace Log {
    void        SetModule(HMODULE hModule, const char* appFolderName = nullptr);
    const char* GetPath();    // ...\logs\debug_log.txt ("" se desabilitado)
    const char* GetDir();     // ...\Documents\<App>   ("" se desabilitado)
    const char* GetLogDir();  // ...\Documents\<App>\logs
    void        Write(const char* level, const char* message);
    void        Info(const char* message);
    void        Warn(const char* message);
    void        Error(const char* message);
    void        Infof(const char* fmt, ...);
    void        Warnf(const char* fmt, ...);
    void        Errorf(const char* fmt, ...);
    void        Shutdown();
}
