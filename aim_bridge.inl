// Mono sidecar loader. No CLR/JIT ABI guesses and no callbacks into this DLL.
static MonoMethod* s_bridgePublish = nullptr;
static MonoMethod* s_bridgeClear = nullptr;
static MonoMethod* s_bridgeStats = nullptr;
static MonoMethod* s_bridgeStart = nullptr;
static MonoMethod* s_bridgeVisible = nullptr;
static MonoMethod* s_modifierApply = nullptr;
static MonoMethod* s_modifierReset = nullptr;
static MonoMethod* s_modifierError = nullptr;
static MonoMethod* s_worldCollect = nullptr;
static MonoMethod* s_worldError = nullptr;
static long long s_bridgeRetryAt = 0;
static bool LoadAimBridge() {
    if (s_bridgePublish) return true;
    const long long now = PiNow();
    if (now < s_bridgeRetryAt) return false;
    s_bridgeRetryAt = now + 5000000;
    using OpenAssembly = void* (__cdecl*)(MonoDomain*, const char*);
    HMODULE mono = GetModuleHandleW(L"mono-2.0-bdwgc.dll");
    auto open = reinterpret_cast<OpenAssembly>(GetProcAddress(mono, "mono_domain_assembly_open"));
    HMODULE self = nullptr;
    wchar_t modulePath[MAX_PATH] = {};
    if (!open || !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&LoadAimBridge), &self) ||
        !GetModuleFileNameW(self, modulePath, MAX_PATH)) return false;
    wchar_t* slash = wcsrchr(modulePath, L'\\');
    if (!slash) return false;
    slash[1] = 0;
    char directory[MAX_PATH*3] = {};
    if (!WideCharToMultiByte(CP_UTF8, 0, modulePath, -1, directory, sizeof(directory), nullptr, nullptr)) return false;
    char path[MAX_PATH*3] = {};
    __try {
        _snprintf_s(path, _TRUNCATE, "%s0Harmony.dll", directory);
        if (!open(s_dom, path)) { Log::Warn("[AIM] 0Harmony.dll ausente/incompativel."); return false; }
        _snprintf_s(path, _TRUNCATE, "%sZb2.AimBridge.dll", directory);
        void* assembly = open(s_dom, path);
        MonoClass* bridge = assembly ? pClassFrom(static_cast<MonoImage*>(pAssmImage(assembly)), "Zb2Menu", "AimBridge") : nullptr;
        if (!bridge) { Log::Warn("[AIM] Zb2.AimBridge.dll ausente/incompativel."); return false; }
        MonoMethod* install = pMethodFrom(bridge, "Install", 0);
        MonoMethod* publish = pMethodFrom(bridge, "Publish", 5);
        MonoMethod* clear = pMethodFrom(bridge, "Clear", 0);
        MonoMethod* visible = pMethodFrom(bridge, "Visible", 4);
        bool installed = false;
        if (!install || !publish || !clear || !visible || !InvokeAimBool(install, nullptr, nullptr, installed) || !installed) {
            Log::Warn("[AIM] adaptador de disparo nao inicializou; Silent/AutoFire/Trigger suspensos.");
            MonoMethod* lastError = pMethodFrom(bridge, "LastError", 0);
            MonoObject* exception = nullptr;
            MonoObject* result = lastError ? pInvoke(lastError, nullptr, nullptr, &exception) : nullptr;
            if (result && !exception) {
                char* error = pStrUtf8(result);
                if (error) { Log::Warnf("[AIM-BRIDGE] %s", error); pFree(error); }
            }
            return false;
        }
        auto image = static_cast<MonoImage*>(pAssmImage(assembly));
        auto modifiers = pClassFrom(image, "Zb2Menu", "ModifierBridge");
        auto world = pClassFrom(image, "Zb2Menu", "WorldEspBridge");
        if (!modifiers || !world) return false;
        s_modifierApply = pMethodFrom(modifiers, "Apply", 7);
        s_modifierReset = pMethodFrom(modifiers, "Reset", 0);
        s_modifierError = pMethodFrom(modifiers, "LastError", 0);
        s_worldCollect = pMethodFrom(world, "Collect", 4);
        s_worldError = pMethodFrom(world, "LastError", 0);
        if (!s_modifierApply || !s_modifierReset || !s_worldCollect) return false;
        s_bridgeStart = pMethodFrom(bridge, "StartLoop", 1);
        if (!s_bridgeStart) return false;
        s_bridgePublish = publish; s_bridgeClear = clear;
        s_bridgeVisible = visible;
        s_bridgeStats = pMethodFrom(bridge, "Statistics", 0);
        Log::Info("[AIM] adaptador pronto: ShotPath compartilhado com SyncShotOnline; Silent / AutoFire / Trigger.");
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Log::Warn("[AIM] falha ao carregar adaptador gerenciado."); return false;
    }
}
static Aim::Visibility ManagedAimVisibility(void* local, const Aim::Target& target) {
    if (!LoadAimBridge()) return Aim::Visibility::Unknown;
    float range = AimPolicy().distance;
    void* args[] = {local, reinterpret_cast<void*>(target.entity), reinterpret_cast<void*>(target.bone), &range};
    bool visible = false;
    if (!InvokeAimBool(s_bridgeVisible, nullptr, args, visible)) return Aim::Visibility::Unknown;
    return visible ? Aim::Visibility::Clear : Aim::Visibility::Blocked;
}
static void ClearAimBridge() {
    if (!s_bridgeClear) return;
    __try { MonoObject* exception = nullptr; pInvoke(s_bridgeClear, nullptr, nullptr, &exception); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}
static void PublishAimBridge(void* local, const Aim::Target* target, int flags) {
    if (!flags) { ClearAimBridge(); return; }
    if (!LoadAimBridge()) return;
    void* entity = target ? reinterpret_cast<void*>(target->entity) : nullptr;
    void* bone = target ? reinterpret_cast<void*>(target->bone) : nullptr;
    float range = AimPolicy().distance;
    void* args[] = {local, entity, bone, &flags, &range};
    __try {
        MonoObject* exception = nullptr;
        pInvoke(s_bridgePublish, nullptr, args, &exception);
        if (exception) { ClearAimBridge(); Log::Warn("[AIM] falha ao publicar pedido de disparo."); }
        static long long nextStats = 0;
        const long long now = PiNow();
        if (s_bridgeStats && now >= nextStats) {
            nextStats = now + 5000000;
            exception = nullptr;
            MonoObject* result = pInvoke(s_bridgeStats, nullptr, nullptr, &exception);
            if (result && !exception) {
                char* stats = pStrUtf8(result);
                if (stats) { Log::Infof("[AIM-BRIDGE] %s", stats); pFree(stats); }
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) { ClearAimBridge(); }
}

#include "world_esp.inl"
