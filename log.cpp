#include "log.h"
#include <cstdio>
#include <cstdarg>
#include <cstring>

#include <ShlObj.h>
#include <KnownFolders.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

namespace Log {

    static HMODULE          g_hModule = nullptr;
    static char             g_appName[64]      = { 0 };
    static char             g_appDir[MAX_PATH]  = { 0 };
    static char             g_logDir[MAX_PATH]  = { 0 };
    static char             g_logPath[MAX_PATH] = { 0 };
    static char             g_dllPath[MAX_PATH] = { 0 };
    static bool             g_ready          = false;
    static bool             g_csInit         = false;
    static bool             g_headerWritten  = false;
    static CRITICAL_SECTION g_cs;

    static const DWORD kMaxLogBytes = 512 * 1024; // 512 KB, depois rotaciona

    // ------------------------------------------------------------
    // Extrai o basename da DLL (sem pasta e sem .dll).
    // Ex: "C:\Games\MinhaHack.dll" -> "MinhaHack"
    // ------------------------------------------------------------
    static void GetDllBasename(HMODULE hModule, char* out, size_t cap) {
        if (!out || cap == 0) return;
        out[0] = '\0';
        if (!hModule) {
            strncpy_s(out, cap, "GameMenu", _TRUNCATE);
            return;
        }

        char fullPath[MAX_PATH] = { 0 };
        if (GetModuleFileNameA(hModule, fullPath, MAX_PATH) == 0) {
            strncpy_s(out, cap, "GameMenu", _TRUNCATE);
            return;
        }

        const char* pSlash = strrchr(fullPath, '\\');
        const char* pSlash2 = strrchr(fullPath, '/');
        const char* pFile = pSlash ? pSlash + 1 : (pSlash2 ? pSlash2 + 1 : fullPath);

        strncpy_s(out, cap, pFile, _TRUNCATE);

        char* pDot = strrchr(out, '.');
        if (pDot && (_stricmp(pDot, ".dll") == 0 || _stricmp(pDot, ".exe") == 0)) {
            *pDot = '\0';
        }

        if (out[0] == '\0') {
            strncpy_s(out, cap, "GameMenu", _TRUNCATE);
        }
    }

    // ------------------------------------------------------------
    // Sanitiza o nome da pasta: permite letras, números, espaço,
    // '-', '_', '.', '(', ')'. Remove o resto (inclui \ / : * ? " < > |
    // e qualquer tentativa de ".." / traversal). Nunca retorna vazio.
    // ------------------------------------------------------------
    static void SanitizeAppName(const char* in, char* out, size_t cap) {
        if (cap == 0) return;
        out[0] = '\0';
        if (!in || !*in) in = "GameMenu";

        size_t j = 0;
        for (size_t i = 0; in[i] != '\0' && j + 1 < cap && j < 48; ++i) {
            char c = in[i];
            bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                      (c >= '0' && c <= '9') ||
                      c == ' ' || c == '-' || c == '_' || c == '.' ||
                      c == '(' || c == ')';
            if (ok) out[j++] = c;
        }
        out[j] = '\0';

        // Remove espaços/pontos do fim (Windows não aceita).
        while (j > 0 && (out[j - 1] == ' ' || out[j - 1] == '.'))
            out[--j] = '\0';

        // Remove "." / ".." residuais.
        if (out[0] == '\0' || strcmp(out, ".") == 0 || strcmp(out, "..") == 0)
            strncpy_s(out, cap, "GameMenu", _TRUNCATE);
    }

    // ------------------------------------------------------------
    // Resolve uma Known Folder para ANSI. Retorna false se falhar.
    // ------------------------------------------------------------
    static bool GetKnownDirAnsi(REFKNOWNFOLDERID rfid, char* out, size_t cap) {
        if (!out || cap == 0) return false;
        out[0] = '\0';

        PWSTR wpath = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(rfid, KF_FLAG_CREATE, nullptr, &wpath)) && wpath) {
            int n = WideCharToMultiByte(CP_ACP, 0, wpath, -1, out, (int)cap, nullptr, nullptr);
            CoTaskMemFree(wpath);
            return n > 1 && out[0] != '\0';
        }
        if (wpath) CoTaskMemFree(wpath);

        // Fallback ANSI legado (pré-Vista / ambientes travados).
        int csidl = (rfid == FOLDERID_Documents) ? CSIDL_MYDOCUMENTS : CSIDL_LOCAL_APPDATA;
        char ansi[MAX_PATH] = { 0 };
        if (SUCCEEDED(SHGetFolderPathA(nullptr, csidl | CSIDL_FLAG_CREATE, nullptr, 0, ansi)) && ansi[0] != '\0') {
            strncpy_s(out, cap, ansi, _TRUNCATE);
            return true;
        }
        return false;
    }

    // ------------------------------------------------------------
    // Monta <base>\<App>\logs\debug_log.txt. Ordem:
    //   1) Documents  2) LocalAppData  3) desabilita (sem TEMP/jogo).
    // Sem I/O aqui além das consultas de Known Folder: seguro no DllMain.
    // ------------------------------------------------------------
    void SetModule(HMODULE hModule, const char* appFolderName) {
        if (!g_csInit) {
            InitializeCriticalSection(&g_cs);
            g_csInit = true;
        }

        g_hModule = hModule;
        g_ready = false;
        g_headerWritten = false;
        g_appDir[0] = g_logDir[0] = g_logPath[0] = g_dllPath[0] = '\0';

        // Se appFolderName não foi passado, deriva do basename da DLL.
        if (appFolderName && *appFolderName) {
            SanitizeAppName(appFolderName, g_appName, sizeof(g_appName));
        } else {
            char baseDllName[64] = { 0 };
            GetDllBasename(hModule, baseDllName, sizeof(baseDllName));
            SanitizeAppName(baseDllName, g_appName, sizeof(g_appName));
        }

        // Guarda o caminho da DLL só para o cabeçalho de diagnóstico.
        if (hModule)
            GetModuleFileNameA(hModule, g_dllPath, MAX_PATH);

        char base[MAX_PATH] = { 0 };
        if (!GetKnownDirAnsi(FOLDERID_Documents, base, sizeof(base))) {
            // Fallback responsável: per-user, gravável sem admin.
            if (!GetKnownDirAnsi(FOLDERID_LocalAppData, base, sizeof(base)))
                return; // sem local seguro -> log desabilitado, sem crash
        }

        _snprintf_s(g_appDir, _TRUNCATE, "%s\\%s", base, g_appName);
        _snprintf_s(g_logDir, _TRUNCATE, "%s\\%s\\logs", base, g_appName);
        _snprintf_s(g_logPath, _TRUNCATE, "%s\\%s\\logs\\debug_log.txt", base, g_appName);

        g_ready = (g_appDir[0] != '\0' && g_logDir[0] != '\0' && g_logPath[0] != '\0');
    }

    const char* GetPath() {
        return g_ready ? g_logPath : "";
    }

    const char* GetDir() {
        return g_ready ? g_appDir : "";
    }

    const char* GetLogDir() {
        return g_ready ? g_logDir : "";
    }

    static void EnsureDirectory() {
        if (!g_ready) return;
        if (g_appDir[0]) CreateDirectoryA(g_appDir, nullptr);
        if (g_logDir[0]) CreateDirectoryA(g_logDir, nullptr);
    }

    // Rotação simples: passou de 512 KB, o atual vira debug_log.bak.
    static void RotateIfNeeded() {
        if (!g_ready) return;
        WIN32_FILE_ATTRIBUTE_DATA fad = { 0 };
        if (!GetFileAttributesExA(g_logPath, GetFileExInfoStandard, &fad))
            return; // ainda não existe
        ULONGLONG size = ((ULONGLONG)fad.nFileSizeHigh << 32) | fad.nFileSizeLow;
        if (size < kMaxLogBytes) return;

        char bak[MAX_PATH] = { 0 };
        _snprintf_s(bak, _TRUNCATE, "%s\\debug_log.bak", g_logDir);
        MoveFileExA(g_logPath, bak, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    }

    // Cabeçalho de sessão: quem, onde, quando. Uma vez por processo.
    static void WriteHeaderLocked(FILE* f) {
        if (!f || g_headerWritten) return;
        g_headerWritten = true;

        SYSTEMTIME st = { 0 };
        GetLocalTime(&st);

        char exePath[MAX_PATH] = { 0 };
        GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        const char* exeName = strrchr(exePath, '\\');
        exeName = exeName ? exeName + 1 : exePath;

#ifdef _WIN64
        const char* arch = "x64";
#else
        const char* arch = "x86";
#endif

        fprintf(f, "==================================================\n");
        fprintf(f, "[%02u/%02u/%04u %02u:%02u:%02u] %s | arch=%s | pid=%lu | host=%s\n",
            (unsigned)st.wDay, (unsigned)st.wMonth, (unsigned)st.wYear,
            (unsigned)st.wHour, (unsigned)st.wMinute, (unsigned)st.wSecond,
            g_appName, arch, (unsigned long)GetCurrentProcessId(), exeName);
        if (g_dllPath[0]) fprintf(f, "dll=%s\n", g_dllPath);
        fprintf(f, "log=%s\n", g_logPath);
        fprintf(f, "==================================================\n");
        fflush(f);
    }

    static void WriteLine(const char* level, const char* message) {
        if (!g_ready || !message || !g_csInit)
            return;

        EnterCriticalSection(&g_cs);

        EnsureDirectory();
        RotateIfNeeded();

        FILE* f = nullptr;
        if (fopen_s(&f, g_logPath, "a") == 0 && f) {
            WriteHeaderLocked(f);
            SYSTEMTIME st = { 0 };
            GetLocalTime(&st);
            fprintf(f, "[%02u:%02u:%02u][TID %lu][%s] %s\n",
                (unsigned)st.wHour, (unsigned)st.wMinute, (unsigned)st.wSecond,
                (unsigned long)GetCurrentThreadId(),
                level ? level : "INFO", message);
            fclose(f);
        }

        LeaveCriticalSection(&g_cs);
    }

    static void WriteFormatted(const char* level, const char* fmt, va_list args) {
        char buf[1024] = { 0 };
        vsnprintf_s(buf, _TRUNCATE, fmt, args);
        WriteLine(level, buf);
    }

    void Write(const char* level, const char* message) { WriteLine(level, message); }
    void Info(const char* message)  { WriteLine("INFO", message); }
    void Warn(const char* message)  { WriteLine("WARN", message); }
    void Error(const char* message) { WriteLine("ERROR", message); }

    void Infof(const char* fmt, ...) {
        if (!fmt) return;
        va_list args; va_start(args, fmt);
        WriteFormatted("INFO", fmt, args);
        va_end(args);
    }

    void Warnf(const char* fmt, ...) {
        if (!fmt) return;
        va_list args; va_start(args, fmt);
        WriteFormatted("WARN", fmt, args);
        va_end(args);
    }

    void Errorf(const char* fmt, ...) {
        if (!fmt) return;
        va_list args; va_start(args, fmt);
        WriteFormatted("ERROR", fmt, args);
        va_end(args);
    }

    void Shutdown() {
        if (!g_csInit) return;
        DeleteCriticalSection(&g_cs);
        g_csInit = false;
    }

} // namespace Log
