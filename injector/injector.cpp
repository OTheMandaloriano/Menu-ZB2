// ============================================================================
// INJECTOR.CPP - Injetor C++ do ZB2 Menu (Zumbi Blocks 2 / x64)
// OBJETIVO: localizar a DLL do cheat, validar ambiente e injetar de forma
//   automatica ao abrir, sem interacao manual, com mensagens claras.
// ORIGEM: tecnica classica LoadLibrary remoto via CreateRemoteThread
//   (padrao estavel p/ DLL propria em jogo Unity sem anti-cheat de kernel).
//   Manual Mapping / Thread Hijacking ficam como evolucao futura (ver HISTORICO).
// TESTES: abrir com o jogo em partida -> "SUCESSO"; sem jogo -> erro claro;
//   sem admin -> aviso; DLL x86 ou ausente -> falha antes de tocar no alvo.
// HISTORICO: v0.2.0 cria o injetor (ciclo incremental pos hook D3D11).
//   v0.2.1 (futuro): Manual Mapping; v0.2.2: re-injecao em update (watcher).
// ============================================================================
// CICLO INCREMENTAL: este arquivo e UMA funcao (injetor). Nao misturar.
// ============================================================================

#include <Windows.h>
#include <TlHelp32.h>
#include <ShlObj.h>
#include <KnownFolders.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// ---- Alvo e artefatos (nada na pasta do jogo) ----
static const wchar_t* kProcNames[] = { L"ZumbiBlocks2.exe" };
static const char* kAppFolder = "ZB2 Menu";
static const char* kDllFile = "kiero-dx11-base.dll";

static void Msg(const char* lvl, const char* fmt, ...) {
    char b[1024] = { 0 };
    va_list a; va_start(a, fmt);
    vsnprintf_s(b, _TRUNCATE, fmt, a);
    va_end(a);
    printf("[%s] %s\n", lvl, b);
    fflush(stdout);
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
    if (!LookupPrivilegeValueA(nullptr, SE_DEBUG_NAME, &tp.Privileges[0].Luid)) {
        CloseHandle(t);
        return false;
    }
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    BOOL ok = AdjustTokenPrivileges(t, FALSE, &tp, sizeof(tp), nullptr, nullptr);
    CloseHandle(t);
    return ok && GetLastError() == ERROR_SUCCESS;
}

// OBJETIVO: achar o pid do jogo (valida "processo aberto?" antes de tudo).
static DWORD FindGamePid() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe = { 0 };
    pe.dwSize = sizeof(pe);
    DWORD pid = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            for (size_t i = 0; i < sizeof(kProcNames) / sizeof(kProcNames[0]); ++i) {
                if (_wcsicmp(pe.szExeFile, kProcNames[i]) == 0) { pid = pe.th32ProcessID; break; }
            }
            if (pid) break;
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
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

// OBJETIVO: localizar a DLL (relativo primeiro, fixo depois) + fingerprint.
// ORIGEM: caminhos relativos ao exe do injetor; fallback: build\Release_x64.
//   "Atualizada?": compara size+mtime com o registro da ultima injecao
//   em %USERPROFILE%\Documents\ZB2 Menu\injector_state.txt.
//   Item 0 (16/09): loga o PATH COMPLETO resolvido + SizeOfImage lido do PE.
//   Se o crash log reportar outro SizeOfImage, o path aqui diz qual build entrou.
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
static bool FindDll(std::wstring& out, ULONGLONG& size, FILETIME& mtime) {
    wchar_t self[MAX_PATH] = { 0 };
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    std::wstring dir(self);
    size_t p = dir.find_last_of(L"\\/");
    dir = (p == std::wstring::npos) ? L"." : dir.substr(0, p);

    char dllA[MAX_PATH] = { 0 };
    strncpy_s(dllA, kDllFile, _TRUNCATE);
    wchar_t dllW[MAX_PATH] = { 0 };
    MultiByteToWideChar(CP_ACP, 0, dllA, -1, dllW, MAX_PATH);

    std::vector<std::wstring> cand;
    cand.push_back(dir + L"\\" + dllW);                       // lado a lado
    cand.push_back(dir + L"\\build\\Release_x64\\" + dllW);   // raiz do projeto
    cand.push_back(dir + L"\\..\\build\\Release_x64\\" + dllW);// injector\..
    cand.push_back(L"D:\\Projeto\\ZB2 Menu\\build\\Release_x64\\" + std::wstring(dllW)); // fixo dev

    for (size_t i = 0; i < cand.size(); ++i) {
        WIN32_FILE_ATTRIBUTE_DATA fa = { 0 };
        if (GetFileAttributesExW(cand[i].c_str(), GetFileExInfoStandard, &fa)) {
            out = cand[i];
            size = ((ULONGLONG)fa.nFileSizeHigh << 32) | fa.nFileSizeLow;
            mtime = fa.ftLastWriteTime;
            char pa[MAX_PATH] = { 0 };
            WideCharToMultiByte(CP_ACP, 0, cand[i].c_str(), -1, pa, MAX_PATH, nullptr, nullptr);
            Msg("INFO", "DLL caminho[%llu]: %s (SizeOfImage=0x%lX).", (ULONGLONG)i, pa, (unsigned long)PeSizeOfImage(cand[i]));
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
    if (!docs[0]) { Msg("INFO", "DLL %llu bytes (estado nao persistido).", size); return; }
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
        Msg("INFO", "DLL %llu bytes (primeira injecao registrada).", size);
    else if (strcmp(prev, cur) == 0)
        Msg("INFO", "DLL inalterada desde a ultima injecao (%s).", cur);
    else
        Msg("INFO", "DLL ATUALIZADA (antes %s, agora %s) - injetando nova versao.", prev, cur);
    if (fopen_s(&f, st, "w") == 0 && f) { fputs(cur, f); fclose(f); }
}

// OBJETIVO: injecao LoadLibraryW remota (estavel p/ modulo proprio).
// Passos: OpenProcess -> Alloc -> Write(dll path) -> CreateRemoteThread(LoadLibraryW)
//   -> Wait -> lee exit code (HMODULE) -> Free. Falha em qualquer passo = abortar
//   sem tocar no alvo alem do necessario.
static bool Inject(DWORD pid, const std::wstring& dll) {
    HANDLE h = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid);
    if (!h) { Msg("ERRO", "OpenProcess falhou (GLE=%lu). Rode como admin.", GetLastError()); return false; }

    SIZE_T cb = (dll.size() + 1) * sizeof(wchar_t);
    LPVOID rem = VirtualAllocEx(h, nullptr, cb, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!rem) { Msg("ERRO", "VirtualAllocEx falhou (GLE=%lu).", GetLastError()); CloseHandle(h); return false; }

    if (!WriteProcessMemory(h, rem, dll.c_str(), cb, nullptr)) {
        Msg("ERRO", "WriteProcessMemory falhou (GLE=%lu).", GetLastError());
        VirtualFreeEx(h, rem, 0, MEM_RELEASE);
        CloseHandle(h);
        return false;
    }

    HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
    LPTHREAD_START_ROUTINE ll = (LPTHREAD_START_ROUTINE)GetProcAddress(k32, "LoadLibraryW");
    if (!ll) { Msg("ERRO", "LoadLibraryW nao resolvido."); VirtualFreeEx(h, rem, 0, MEM_RELEASE); CloseHandle(h); return false; }

    HANDLE th = CreateRemoteThread(h, nullptr, 0, ll, rem, 0, nullptr);
    if (!th) {
        Msg("ERRO", "CreateRemoteThread falhou (GLE=%lu). Anti-cheat pode bloquear threads remotas.", GetLastError());
        VirtualFreeEx(h, rem, 0, MEM_RELEASE);
        CloseHandle(h);
        return false;
    }
    Msg("INFO", "Thread remota criada, aguardando LoadLibrary (30s max)...");
    DWORD w = WaitForSingleObject(th, 30000);
    DWORD mod = 0;
    GetExitCodeThread(th, &mod);
    CloseHandle(th);
    VirtualFreeEx(h, rem, 0, MEM_RELEASE);
    CloseHandle(h);

    if (w != WAIT_OBJECT_0) { Msg("ERRO", "Timeout no LoadLibrary remoto."); return false; }
    if (mod == 0) { Msg("ERRO", "LoadLibrary retornou NULL (DLL nao carregou; confira arquitetura/dependencias)."); return false; }
    Msg("OK", "DLL carregada no alvo (HMODULE remoto=0x%p).", (void*)(uintptr_t)mod);
    return true;
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    printf("ZB2 Menu Injector v0.2.0 (x64) - injecao automatica\n");
    printf("==================================================\n");

    // 1. Permissoes
    if (!IsAdmin())
        Msg("AVISO", "Sem privilegios de administrador - tentando mesmo assim (pode falhar).");
    else
        Msg("INFO", "Rodando como administrador.");
    if (!EnableDebugPriv())
        Msg("AVISO", "SeDebugPrivilege nao elevado (GLE=%lu).", GetLastError());
    else
        Msg("INFO", "SeDebugPrivilege elevado.");

    // 2. DLL automatica
    std::wstring dll;
    ULONGLONG size = 0;
    FILETIME mt = { 0 };
    if (!FindDll(dll, size, mt)) {
        Msg("ERRO", "DLL '%s' nao encontrada (procurei ao lado do injetor, build\\Release_x64 e caminho dev).", kDllFile);
        Msg("ERRO", "Compile a DLL antes (kiero-dx11-base.vcxproj Release|x64).");
        Sleep(2500);
        return 2;
    }
    char dllA[MAX_PATH] = { 0 };
    WideCharToMultiByte(CP_ACP, 0, dll.c_str(), -1, dllA, MAX_PATH, nullptr, nullptr);
    Msg("INFO", "DLL localizada: %s (%llu bytes).", dllA, size);
    if (!DllIs64(dll)) {
        Msg("ERRO", "DLL nao e x64 (ou PE invalido) - injecao abortada p/ nao crashar o jogo.");
        Sleep(2500);
        return 3;
    }
    ReportDllState(size, mt);

    // 3. Jogo aberto?
    DWORD pid = FindGamePid();
    if (!pid) {
        Msg("ERRO", "Processo ZumbiBlocks2.exe nao encontrado. Abra o jogo antes.");
        Sleep(2500);
        return 4;
    }
    Msg("INFO", "Jogo encontrado (PID %lu).", pid);
    if (!TargetIs64(pid)) {
        Msg("ERRO", "Alvo nao e x64 - abortando (incompativel com a DLL).");
        Sleep(2500);
        return 5;
    }

    // 4. Injecao automatica (sem clique)
    Msg("INFO", "Injetando em 1s (automatico)...");
    Sleep(1000);
    bool ok = Inject(pid, dll);

    if (ok) {
        Msg("OK", "SUCESSO: pressione INSERT no jogo p/ abrir o menu.");
        Msg("INFO", "Log da DLL em Documents\\%s\\logs\\debug_log.txt.", kAppFolder);
        return 0;
    }
    Msg("ERRO", "FALHA na injecao - veja as mensagens acima.");
    Sleep(3000);
    return 1;
}
