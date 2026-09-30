// ============================================================================
// INJECTOR.CPP - Injetor C++ do ZB2 Menu (Zumbi Blocks 2 / x64)
// OBJETIVO: auto-inject sem interacao: watcher com polling 1s, LoadLibrary
//   remoto, logs coloridos com timestamp, config externa (config.ini).
// ORIGEM: tecnica classica LoadLibrary remoto via CreateRemoteThread
//   (padrao estavel p/ DLL propria em jogo Unity sem anti-cheat de kernel).
//   Cores via SetConsoleTextAttribute (padrao GH Injector/GhostInject);
//   polling 1s + timeout (padrao Xenos/GH Injector "manual launch").
// TESTES: sem jogo -> watcher ate timeout (exit 4); com jogo -> SUCESSO +
//   HMODULE remoto (exit 0); config.ini ausente -> defaults; --nowait pula
//   a pausa final (p/ scripts).
// HISTORICO: v0.2.0 cria o injetor. v0.3.0: config.ini + timestamp + cores +
//   watcher configuravel + retry. Futuro: manualmap.cpp (--method=manual).
// ============================================================================
// CICLO INCREMENTAL: este arquivo e UMA funcao (injetor). Nao misturar.
// ============================================================================

#include <Windows.h>
#include <TlHelp32.h>
#include <ShlObj.h>
#include <KnownFolders.h>
#include <conio.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <charconv>

static const char* kAppFolder = "ZB2 Menu";
static const char* kDllFile = "kiero-dx11-base.dll"; // default (config.ini pode trocar)

// ---- Log colorido com timestamp ----
// Cores so no console real (saida redirecionada p/ arquivo sai sem ANSI/codigos).
enum class Level { Info, Ok, Warn, Err };

static HANDLE StdOut() {
    static HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    return h;
}

static bool UseColor() {
    static int v = -1;
    if (v < 0) { DWORD m = 0; v = GetConsoleMode(StdOut(), &m) ? 1 : 0; }
    return v == 1;
}

static void SetFg(WORD c) {
    if (UseColor()) SetConsoleTextAttribute(StdOut(), c);
}

// Cinza=hora, ciano=INFO, verde=OK, amarelo=AVISO, vermelho=ERRO. Sem acento
// (codepage do console quebra UTF-8 sem manifesto; regra do projeto).
static void Log(Level lv, const char* fmt, ...) {
    SYSTEMTIME st = { 0 };
    GetLocalTime(&st);
    const char* tag = "?";
    WORD col = 7;
    switch (lv) {
    case Level::Info: tag = "INFO";  col = 11; break;
    case Level::Ok:   tag = "OK";    col = 10; break;
    case Level::Warn: tag = "AVISO"; col = 14; break;
    case Level::Err:  tag = "ERRO";  col = 12; break;
    }
    char b[1024] = { 0 };
    va_list a; va_start(a, fmt);
    vsnprintf_s(b, _TRUNCATE, fmt, a);
    va_end(a);
    SetFg(8);
    printf("[%02u:%02u:%02u] ", st.wHour, st.wMinute, st.wSecond);
    SetFg(col);
    printf("[%s] ", tag);
    SetFg(7);
    printf("%s\n", b);
    SetFg(7);
    fflush(stdout);
}

// ---- Config externa (config.ini ao lado do exe; sem ele = defaults) ----
struct Cfg {
    wchar_t process[64];
    wchar_t dll[MAX_PATH];
    int timeout;        // watcher (s)
    int retry;          // tentativas de injecao
    int retryDelay;     // espera entre tentativas (s)
    int bootstrapDelay; // espera antes da 1a tentativa (s)
};

static void CfgDefaults(Cfg& c) {
    wcscpy_s(c.process, L"ZumbiBlocks2.exe");
    size_t n = 0;
    mbstowcs_s(&n, c.dll, kDllFile, _TRUNCATE);
    c.timeout = 120; c.retry = 3; c.retryDelay = 5; c.bootstrapDelay = 3;
}

static void TrimA(char* s) {
    while (*s == ' ' || *s == '\t') { memmove(s, s + 1, strlen(s)); }
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\r' || s[n - 1] == '\n')) s[--n] = 0;
}

static int ClampI(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

static bool LoadConfig(Cfg& c) {
    CfgDefaults(c);
    wchar_t self[MAX_PATH] = { 0 };
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    std::wstring dir(self);
    size_t p = dir.find_last_of(L"\\/");
    dir = (p == std::wstring::npos) ? L"." : dir.substr(0, p);
    std::wstring path = dir + L"\\config.ini";

    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"r") != 0 || !f) {
        Log(Level::Info, "config.ini ausente, usando defaults (ZumbiBlocks2.exe, 120s).");
        return false;
    }
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        TrimA(line);
        if (!line[0] || line[0] == ';' || line[0] == '#' || line[0] == '[') continue;
        char* eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;
        char* key = line; char* val = eq + 1;
        TrimA(key); TrimA(val);
        if (!_stricmp(key, "process")) {
            wchar_t w[64] = { 0 };
            size_t n = 0;
            mbstowcs_s(&n, w, val, _TRUNCATE);
            if (w[0]) wcscpy_s(c.process, w);
        } else if (!_stricmp(key, "dll")) {
            wchar_t w[MAX_PATH] = { 0 };
            size_t n = 0;
            mbstowcs_s(&n, w, val, _TRUNCATE);
            if (w[0]) wcscpy_s(c.dll, w);
        } else if (!_stricmp(key, "timeout")) c.timeout = ClampI(atoi(val), 1, 3600);
        else if (!_stricmp(key, "retry")) c.retry = ClampI(atoi(val), 1, 10);
        else if (!_stricmp(key, "retry_delay")) c.retryDelay = ClampI(atoi(val), 0, 60);
        else if (!_stricmp(key, "bootstrap_delay")) c.bootstrapDelay = ClampI(atoi(val), 0, 60);
    }
    fclose(f);
    char pa[MAX_PATH] = { 0 };
    WideCharToMultiByte(CP_ACP, 0, path.c_str(), -1, pa, MAX_PATH, nullptr, nullptr);
    Log(Level::Info, "config.ini: %s", pa);
    return true;
}

// OBJETIVO: checar admin (injetar exige SeDebugPrivilege na pratica).
// ORIGEM: CheckTokenMembership / IsUserAnAdmin (shell32).
static bool IsAdmin() {
    BOOL admin = FALSE;
    PSID g = nullptr;
    SID_IDENTIFIER_AUTHORITY nt = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&nt, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &g)) {
        CheckTokenMembership(nullptr, g, &admin);
        FreeSid(g);
    }
    return admin == TRUE;
}

// OBJETIVO: elevar SeDebugPrivilege p/ OpenProcess nao falhar no jogo.
// ORIGEM: padrao MSDN AdjustTokenPrivileges.
static bool EnableDebugPriv() {
    HANDLE t = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &t))
        return false;
    TOKEN_PRIVILEGES tp = { 0 };
    tp.PrivilegeCount = 1;
    if (!LookupPrivilegeValueW(nullptr, L"SeDebugPrivilege", &tp.Privileges[0].Luid)) {
        CloseHandle(t);
        return false;
    }
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    BOOL ok = AdjustTokenPrivileges(t, FALSE, &tp, sizeof(tp), nullptr, nullptr);
    CloseHandle(t);
    return ok && GetLastError() == ERROR_SUCCESS;
}

// OBJETIVO: achar o pid pelo nome (valida "processo aberto?" antes de tudo).
static DWORD FindProcessId(const wchar_t* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe = { 0 };
    pe.dwSize = sizeof(pe);
    DWORD pid = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, name) == 0) { pid = pe.th32ProcessID; break; }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

// OBJETIVO: watcher — polling 1s ate o processo aparecer ou o timeout.
// No console atualiza a mesma linha (\r); redirecionado loga a cada 15s.
static DWORD WaitForProcess(const wchar_t* name, int timeoutSec) {
    char nameA[64] = { 0 };
    WideCharToMultiByte(CP_ACP, 0, name, -1, nameA, sizeof(nameA), nullptr, nullptr);
    DWORD pid = FindProcessId(name);
    if (pid) return pid;
    bool console = UseColor();
    for (int s = 1; s <= timeoutSec; ++s) {
        if (console) {
            SYSTEMTIME st = { 0 };
            GetLocalTime(&st);
            SetFg(8);
            printf("\r[%02u:%02u:%02u] ", st.wHour, st.wMinute, st.wSecond);
            SetFg(11);
            printf("[INFO] ");
            SetFg(7);
            printf("Aguardando %s... %ds/%ds   ", nameA, s, timeoutSec);
            fflush(stdout);
        } else if (s == 1 || s % 15 == 0) {
            Log(Level::Info, "Aguardando %s... %ds/%ds", nameA, s, timeoutSec);
        }
        Sleep(1000);
        pid = FindProcessId(name);
        if (pid) break;
    }
    if (console) { printf("\n"); fflush(stdout); }
    return pid;
}

// OBJETIVO: garantir alvo x64 (DLL e x64; injetar x64 em x86 = crash certo).
static bool TargetIs64(DWORD pid) {
    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!h) return false;
    BOOL wow = FALSE;
    BOOL ok = IsWow64Process(h, &wow);
    CloseHandle(h);
    return ok && !wow; // nao-WOW64 em SO 64-bit => x64
}

// OBJETIVO: ler IMAGE_FILE_HEADER.Machine da DLL p/ validar arquitetura.
static bool DllIs64(const std::wstring& path) {
    HANDLE f = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return false;
    IMAGE_DOS_HEADER dos = { 0 };
    DWORD r = 0;
    bool ok = false;
    if (ReadFile(f, &dos, sizeof(dos), &r, nullptr) && r == sizeof(dos) && dos.e_magic == IMAGE_DOS_SIGNATURE) {
        SetFilePointer(f, dos.e_lfanew, nullptr, FILE_BEGIN);
        IMAGE_NT_HEADERS64 nt = { 0 };
        if (ReadFile(f, &nt, sizeof(nt), &r, nullptr) && r == sizeof(nt) &&
            nt.Signature == IMAGE_NT_SIGNATURE)
            ok = (nt.FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64);
    }
    CloseHandle(f);
    return ok;
}

// OBJETIVO: localizar a DLL + fingerprint.
// ORIGEM: config.ini manda (caminho literal ou nome p/ cadeia de busca);
//   sem caminho: relativo ao exe, build\Release_x64, caminho dev.
//   Item 0 (16/09): loga o PATH COMPLETO resolvido + SizeOfImage lido do PE.
static DWORD PeSizeOfImage(const std::wstring& path) {
    HANDLE f = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return 0;
    IMAGE_DOS_HEADER dos = { 0 };
    DWORD r = 0;
    DWORD soi = 0;
    if (ReadFile(f, &dos, sizeof(dos), &r, nullptr) && r == sizeof(dos) && dos.e_magic == IMAGE_DOS_SIGNATURE) {
        SetFilePointer(f, dos.e_lfanew, nullptr, FILE_BEGIN);
        IMAGE_NT_HEADERS64 nt = { 0 };
        if (ReadFile(f, &nt, sizeof(nt), &r, nullptr) && r == sizeof(nt) &&
            nt.Signature == IMAGE_NT_SIGNATURE)
            soi = nt.OptionalHeader.SizeOfImage;
    }
    CloseHandle(f);
    return soi;
}

static bool FindDll(const Cfg& cfg, std::wstring& out, ULONGLONG& size, FILETIME& mtime) {
    std::wstring want(cfg.dll);
    std::vector<std::wstring> cand;
    if (want.find(L'\\') != std::wstring::npos || want.find(L'/') != std::wstring::npos) {
        cand.push_back(want); // caminho literal da config (absoluto ou relativo)
    } else {
        wchar_t self[MAX_PATH] = { 0 };
        GetModuleFileNameW(nullptr, self, MAX_PATH);
        std::wstring dir(self);
        size_t p = dir.find_last_of(L"\\/");
        dir = (p == std::wstring::npos) ? L"." : dir.substr(0, p);
        cand.push_back(dir + L"\\" + want);                       // lado a lado
        cand.push_back(dir + L"\\build\\Release_x64\\" + want);   // raiz do projeto
        cand.push_back(dir + L"\\..\\build\\Release_x64\\" + want);// injector\..
        // (REMOVIDO 25/09: caminho fixo D:\Projeto\... quebrava em outro PC —
        // o injetor tentava a pasta do dev antes de falhar. Portatil: so os
        // 3 relativos acima + config.ini.)
    }

    for (size_t i = 0; i < cand.size(); ++i) {
        WIN32_FILE_ATTRIBUTE_DATA fa = { 0 };
        if (GetFileAttributesExW(cand[i].c_str(), GetFileExInfoStandard, &fa)) {
            out = cand[i];
            size = ((ULONGLONG)fa.nFileSizeHigh << 32) | fa.nFileSizeLow;
            mtime = fa.ftLastWriteTime;
            char pa[MAX_PATH] = { 0 };
            WideCharToMultiByte(CP_ACP, 0, cand[i].c_str(), -1, pa, MAX_PATH, nullptr, nullptr);
            Log(Level::Info, "DLL caminho[%llu]: %s (SizeOfImage=0x%lX).", (ULONGLONG)i, pa, (unsigned long)PeSizeOfImage(cand[i]));
            return true;
        }
    }
    return false;
}

static void DocsDir(char* out, size_t cap) {
    out[0] = 0;
    PWSTR w = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_CREATE, nullptr, &w)) && w) {
        WideCharToMultiByte(CP_ACP, 0, w, -1, out, (int)cap, nullptr, nullptr);
        CoTaskMemFree(w);
    }
}

// OBJETIVO: relatar se a DLL mudou desde a ultima injecao (hash leve: size+mtime).
static void ReportDllState(ULONGLONG size, const FILETIME& mt) {
    char docs[MAX_PATH] = { 0 };
    DocsDir(docs, sizeof(docs));
    if (!docs[0]) { Log(Level::Info, "DLL %llu bytes (estado nao persistido).", size); return; }
    char app[MAX_PATH] = { 0 }, st[MAX_PATH] = { 0 };
    _snprintf_s(app, _TRUNCATE, "%s\\%s", docs, kAppFolder);
    _snprintf_s(st, _TRUNCATE, "%s\\injector_state.txt", app);
    CreateDirectoryA(app, nullptr);

    ULONGLONG lo = ((ULONGLONG)mt.dwHighDateTime << 32) | mt.dwLowDateTime;
    FILE* f = nullptr;
    char prev[128] = { 0 };
    if (fopen_s(&f, st, "r") == 0 && f) {
        fgets(prev, sizeof(prev), f);
        fclose(f);
    }
    char cur[128] = { 0 };
    _snprintf_s(cur, _TRUNCATE, "%llu-%llu", size, lo);
    if (prev[0] == 0)
        Log(Level::Info, "DLL %llu bytes (primeira injecao registrada).", size);
    else if (strcmp(prev, cur) == 0)
        Log(Level::Info, "DLL inalterada desde a ultima injecao (%s).", cur);
    else
        Log(Level::Info, "DLL ATUALIZADA (antes %s, agora %s) - injetando nova versao.", prev, cur);
    if (fopen_s(&f, st, "w") == 0 && f) { fputs(cur, f); fclose(f); }
}

// OBJETIVO: injecao LoadLibraryW remota (estavel p/ modulo proprio).
// Passos: OpenProcess -> Alloc -> Write(dll path) -> CreateRemoteThread(LoadLibraryW)
//   -> Wait -> le exit code (HMODULE) -> Free. Falha em qualquer passo = abortar
//   sem tocar no alvo alem do necessario.
static bool Inject(DWORD pid, const std::wstring& dll) {
    HANDLE h = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid);
    if (!h) { Log(Level::Err, "OpenProcess falhou (GLE=%lu). Rode como admin.", GetLastError()); return false; }

    SIZE_T cb = (dll.size() + 1) * sizeof(wchar_t);
    LPVOID rem = VirtualAllocEx(h, nullptr, cb, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!rem) { Log(Level::Err, "VirtualAllocEx falhou (GLE=%lu).", GetLastError()); CloseHandle(h); return false; }

    if (!WriteProcessMemory(h, rem, dll.c_str(), cb, nullptr)) {
        Log(Level::Err, "WriteProcessMemory falhou (GLE=%lu).", GetLastError());
        VirtualFreeEx(h, rem, 0, MEM_RELEASE);
        CloseHandle(h);
        return false;
    }

    HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
    LPTHREAD_START_ROUTINE ll = (LPTHREAD_START_ROUTINE)GetProcAddress(k32, "LoadLibraryW");
    if (!ll) { Log(Level::Err, "LoadLibraryW nao resolvido."); VirtualFreeEx(h, rem, 0, MEM_RELEASE); CloseHandle(h); return false; }

    HANDLE th = CreateRemoteThread(h, nullptr, 0, ll, rem, 0, nullptr);
    if (!th) {
        Log(Level::Err, "CreateRemoteThread falhou (GLE=%lu).", GetLastError());
        VirtualFreeEx(h, rem, 0, MEM_RELEASE);
        CloseHandle(h);
        return false;
    }
    Log(Level::Info, "Thread remota criada, aguardando LoadLibrary (30s max)...");
    DWORD w = WaitForSingleObject(th, 30000);
    DWORD mod = 0;
    GetExitCodeThread(th, &mod);
    CloseHandle(th);
    // On timeout the remote thread may still read this buffer. Keep it alive
    // until the game exits rather than causing a use-after-free in the game.
    if (w == WAIT_OBJECT_0) VirtualFreeEx(h, rem, 0, MEM_RELEASE);
    CloseHandle(h);

    if (w != WAIT_OBJECT_0) { Log(Level::Err, "Timeout no LoadLibrary remoto."); return false; }
    if (mod == 0) { Log(Level::Err, "LoadLibrary retornou NULL (DLL nao carregou; confira arquitetura/dependencias)."); return false; }
    Log(Level::Ok, "DLL carregada no alvo (HMODULE remoto=0x%p).", (void*)(uintptr_t)mod);
    return true;
}

static void PauseExit(bool nowait) {
    if (nowait) return;
    SetFg(8);
    printf("Pressione qualquer tecla para sair...");
    SetFg(7);
    fflush(stdout);
    _getch();
    printf("\n");
}

int main(int argc, char** argv) {
    bool nowait = false;
    bool readinessProbe = false;
    DWORD requestedPid = 0;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--nowait") == 0 || strcmp(argv[i], "-n") == 0) nowait = true;
        else if(strcmp(argv[i],"--readiness-probe")==0)readinessProbe=true;
        else if (strcmp(argv[i], "--pid") == 0) {
            if (++i >= argc) return 6;
            const char* end = argv[i] + strlen(argv[i]);
            auto parsed = std::from_chars(argv[i], end, requestedPid);
            if (parsed.ec != std::errc() || parsed.ptr != end || !requestedPid) return 6;
        }
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "/?") == 0) {
            printf("Uso: injector.exe [--nowait]\n");
            printf("  Le config.ini ao lado do exe (process/dll/timeout/retry).\n");
            printf("  Sem config.ini usa defaults: ZumbiBlocks2.exe + kiero-dx11-base.dll + 120s.\n");
            printf("  --nowait: pula a pausa final (p/ scripts).\n");
            return 0;
        }
    }

    SetFg(11);
    printf("ZB2 Menu Injector v0.3.0 (x64) - auto-inject\n");
    SetFg(7);
    printf("===========================================\n");
    fflush(stdout);

    // 0. Config externa
    Cfg cfg;
    LoadConfig(cfg);
    if (requestedPid) { cfg.retry = 1; cfg.bootstrapDelay = 0; cfg.timeout = 0; }
    if(readinessProbe){if(!requestedPid)return 6;wcscpy_s(cfg.dll,L"ZB2.Readiness.dll");}
    char procA[64] = { 0 }, dllA[MAX_PATH] = { 0 };
    WideCharToMultiByte(CP_ACP, 0, cfg.process, -1, procA, sizeof(procA), nullptr, nullptr);
    WideCharToMultiByte(CP_ACP, 0, cfg.dll, -1, dllA, sizeof(dllA), nullptr, nullptr);
    Log(Level::Info, "Alvo: %s | DLL: %s | timeout %ds | retry %dx.",
        procA, dllA, cfg.timeout, cfg.retry);

    // 1. Permissoes
    if (!IsAdmin())
        Log(Level::Warn, "Sem privilegios de administrador - tentando mesmo assim (pode falhar).");
    else
        Log(Level::Info, "Rodando como administrador.");
    if (!EnableDebugPriv())
        Log(Level::Warn, "SeDebugPrivilege nao elevado (GLE=%lu).", GetLastError());
    else
        Log(Level::Info, "SeDebugPrivilege elevado.");

    // 2. DLL automatica
    std::wstring dll;
    ULONGLONG size = 0;
    FILETIME mt = { 0 };
    if (!FindDll(cfg, dll, size, mt)) {
        Log(Level::Err, "DLL '%s' nao encontrada (config.ini ou ao lado do injetor).", dllA);
        Log(Level::Err, "Compile a DLL antes (kiero-dx11-base.vcxproj Release|x64).");
        PauseExit(nowait);
        return 2;
    }
    char dllFound[MAX_PATH] = { 0 };
    WideCharToMultiByte(CP_ACP, 0, dll.c_str(), -1, dllFound, MAX_PATH, nullptr, nullptr);
    Log(Level::Info, "DLL localizada: %s (%llu bytes).", dllFound, size);
    if (!DllIs64(dll)) {
        Log(Level::Err, "DLL nao e x64 (ou PE invalido) - injecao abortada p/ nao crashar o jogo.");
        PauseExit(nowait);
        return 3;
    }
    ReportDllState(size, mt);

    // 3. Watcher: espera o processo aparecer (polling 1s ate timeout).
    DWORD pid = requestedPid ? requestedPid : WaitForProcess(cfg.process, cfg.timeout);
    if (requestedPid && FindProcessId(cfg.process) != requestedPid) return 6;
    if (!pid) {
        Log(Level::Err, "Processo %s nao apareceu em %ds. Abra o jogo e rode de novo.", procA, cfg.timeout);
        PauseExit(nowait);
        return 4;
    }
    Log(Level::Ok, "Processo encontrado (PID %lu).", pid);
    if (!TargetIs64(pid)) {
        Log(Level::Err, "Alvo nao e x64 - abortando (incompativel com a DLL).");
        PauseExit(nowait);
        return 5;
    }

    // 4. Injecao automatica com retry (bootstrap do jogo pode recusar a 1a).
    bool ok = false;
    for (int t = 1; t <= cfg.retry && !ok; ++t) {
        if (t > 1) { Log(Level::Info, "Tentativa %d/%d em %ds...", t, cfg.retry, cfg.retryDelay); Sleep((DWORD)cfg.retryDelay * 1000); }
        else if (cfg.bootstrapDelay > 0) { Log(Level::Info, "Injetando em %ds (jogo termina o bootstrap)...", cfg.bootstrapDelay); Sleep((DWORD)cfg.bootstrapDelay * 1000); }
        Log(Level::Info, "Tentativa %d/%d...", t, cfg.retry);
        ok = Inject(pid, dll);
    }

    int rc;
    if (ok) {
        Log(Level::Ok, readinessProbe ? "Monitor de prontidao carregado. O menu ainda aguarda a cena da partida." : "SUCESSO: pressione INSERT no jogo p/ abrir o menu.");
        Log(Level::Info, "Log da DLL em Documents\\%s\\logs\\debug_log.txt.", kAppFolder);
        rc = 0;
    } else {
        Log(Level::Err, "FALHA na injecao - veja as mensagens acima.");
        rc = 1;
    }
    PauseExit(nowait);
    return rc;
}
