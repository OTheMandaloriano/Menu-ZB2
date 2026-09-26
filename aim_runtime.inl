// Internal Mono adapter, included inside namespace Mono after projection helpers.
// All functions run on EspThread. Pure selection rules live in aim_logic.h.
static Aim::Target s_aimTargets[Aim::MaxTargets];
static int s_aimCount = 0;
static Aim::Lock s_aimLock;
static Aim::Activation s_aimActivation;
static void ClearAimBridge();
static void ConfigureAimRange(bool enabled);
static Aim::Point s_aimForward = {};
static bool s_aimForwardValid = false;
static int s_visibilityCursor = 0;

static bool AimRequested() {
    return Config::bAimbot || Config::bAutoAim || Config::bSilentAim || Config::bAutoFire || Config::bTriggerbot;
}
static void ResetAim() {
    s_aimCount = 0;
    s_aimLock.Clear();
    s_aimActivation.Reset();
    s_visibilityCursor = 0;
    ClearAimBridge();
    ConfigureAimRange(false);
}
static Aim::Policy AimPolicy() {
    Aim::Policy policy;
    policy.distance = Fin(Config::fAimDistance) ? Config::fAimDistance : 120.0f;
    policy.distance = (std::max)(10.0f, (std::min)(500.0f, policy.distance));
    policy.radius = Config::bLimitFov && !Config::b360Mode ? Config::fFovAngle * 4.0f : 0;
    if (!Fin(policy.radius) || policy.radius < 0) policy.radius = 360;
    policy.priority = Config::iAimPriority;
    policy.fullCircle = Config::b360Mode;
    return policy;
}
static bool ReadAimBone(void* zo, Aim::Point& point, std::uintptr_t& boneIdentity) {
    static const int joints[] = { SK_HEAD, SK_NECK, SK_SP2, SK_HL };
    static const char* names[] = { "head", "neck", "sp2", "hl" };
    const int selection = Config::iAimBone >= 0 && Config::iAimBone < 4 ? Config::iAimBone : 0;
    void* array = ReadP(zo, Off::ZO_armature);
    if (!array) return false;
    long long count = 0;
    __try { memcpy(&count, (char*)array + Off::A_len, sizeof(count)); }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    if (count <= 0 || count > 64) return false;
    for (int i = 0; i < count; ++i) {
        const int index = count == 19 ? kBoneIdx[joints[selection]] : i;
        void* bone = ReadP(array, Off::A_data + index * sizeof(void*));
        if (bone) {
            char name[64] = {};
            if (count != 19) GetName(bone, name, sizeof(name));
            if (count == 19 || !strcmp(name, names[selection])) {
                Vec3 world;
                if (!GetPos(bone, world) || !Fin(world.x) || !Fin(world.y) || !Fin(world.z)) return false;
                point = { world.x, world.y, world.z };
                boneIdentity = reinterpret_cast<std::uintptr_t>(bone);
                return true;
            }
        }
        if (count == 19) break;
    }
    return false;
}
static void CollectAimTarget(void* entity, void* zo, void* camera, float hp) {
    if (!AimRequested() || !s_camWok || !s_aimForwardValid) return;
    Aim::Target target;
    if (!ReadAimBone(zo, target.position, target.bone)) return;
    Aim::Point direction;
    if (!Aim::Ray({s_camW.x,s_camW.y,s_camW.z}, target.position, direction, target.distance)) return;
    Vec3 projected, world = { target.position.x, target.position.y, target.position.z };
    target.angle = Aim::Angle(s_aimForward, direction);
    target.projected = W2S(camera, world, projected);
    target.pixels = 0;
    if (target.projected) {
        const float dx = projected.x - s_vpW * 0.5f, dy = projected.y - s_vpH * 0.5f;
        target.pixels = sqrtf(dx * dx + dy * dy);
    }
    target.entity = reinterpret_cast<std::uintptr_t>(entity);
    target.hp = hp;
    Aim::KeepBest(s_aimTargets, s_aimCount, target, AimPolicy());
}
static bool AimTargetAlive(const Aim::Target& target) {
    void* health = ReadP(reinterpret_cast<void*>(target.entity), Off::Z_health);
    unsigned char alive = 0;
    __try { if (health) memcpy(&alive, (char*)health + Off::ZH_alive, 1); }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    return alive && ReadF(health, Off::ZH_amount) > 0;
}

struct AimPhysics {
    MonoMethod* raycast = nullptr;
    MonoMethod* collider = nullptr;
    MonoMethod* isChild = nullptr;
    int hitSize = 0;
    long long retryAt = 0;
};
static AimPhysics s_aimPhysics;
static void AimVisibilityUnavailable(const char* reason) {
    static long long nextLog = 0;
    const long long now = PiNow();
    if (now < nextLog) return;
    nextLog = now + 5000000;
    Log::Warnf("[AIM] visibilidade indisponivel: %s; mira/disparo suspensos.", reason);
}
static bool HasParameterTypes(MonoMethod* method, const char* const* expected, int count) {
    if (!pSigOf || !pSigCount || !pSigParam || !pTypeName || !pFree) return false;
    void* signature = pSigOf(method);
    if (!signature || pSigCount(signature) != count) return false;
    void* iter = nullptr;
    for (int i = 0; i < count; ++i) {
        void* type = pSigParam(signature, &iter);
        const char* name = type ? pTypeName(type) : nullptr;
        const bool matches = name && !strcmp(name, expected[i]);
        if (name) pFree(const_cast<char*>(name));
        if (!matches) return false;
    }
    return true;
}
static bool ResolveAimPhysics() {
    if (s_aimPhysics.raycast) return true;
    const long long now = PiNow();
    if (now < s_aimPhysics.retryAt) return false;
    s_aimPhysics.retryAt = now + 2000000;
    __try {
        cPhys = FindUnityClass("Physics");
        MonoClass* hit = FindUnityClass("RaycastHit");
        if (!cPhys || !hit || !cTrans || !pClassValueSize || !pClassMethods || !pMethodGetName) return false;
        unsigned alignment = 0;
        const int size = pClassValueSize(hit, &alignment);
        if (size <= 0 || size > 128 || alignment > 16) return false;
        MonoMethod* collider = pMethodFrom(hit, "get_collider", 0);
        MonoMethod* child = pMethodFrom(cTrans, "IsChildOf", 1);
        if (!collider || !child) return false;
        const char* types[] = { "UnityEngine.Vector3", "UnityEngine.Vector3", "UnityEngine.RaycastHit&",
                               "System.Single", "System.Int32", "UnityEngine.QueryTriggerInteraction" };
        void* iter = nullptr;
        while (MonoMethod* method = static_cast<MonoMethod*>(pClassMethods(cPhys, &iter))) {
            const char* name = pMethodGetName(method);
            if (!name || strcmp(name, "Raycast") || !HasParameterTypes(method, types, 6)) continue;
            s_aimPhysics.collider = collider;
            s_aimPhysics.isChild = child;
            s_aimPhysics.hitSize = size;
            s_aimPhysics.raycast = method;
            Log::Infof("[AIM] Physics.Raycast(out RaycastHit) pronto; tamanho runtime=%d.", size);
            return true;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
    return false;
}
static bool InvokeAimBool(MonoMethod* method, void* object, void** args, bool& value) {
    __try {
        MonoObject* exception = nullptr;
        MonoObject* result = pInvoke(method, object, args, &exception);
        if (exception || !result) return false;
        void* data = pUnbox(result);
        if (!data) return false;
        value = *static_cast<unsigned char*>(data) != 0;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
static Aim::Visibility AimVisibility(const Aim::Target& target) {
    if (!ResolveAimPhysics()) { AimVisibilityUnavailable("binding Physics/RaycastHit"); return Aim::Visibility::Unknown; }
    Aim::Point origin = {s_camW.x,s_camW.y,s_camW.z}, direction;
    float distance;
    if (!Aim::Ray(origin, target.position, direction, distance)) return Aim::Visibility::Unknown;
    // QueryTriggerInteraction.Ignore=1, verified in the installed PhysicsModule.
    // All layers: a shot mask must not silently exclude walls from visibility.
    int mask = -1, ignoreTriggers = 1;
    alignas(16) unsigned char hit[128] = {};
    void* args[] = { &origin, &direction, hit, &distance, &mask, &ignoreTriggers };
    bool collided = false;
    if (!InvokeAimBool(s_aimPhysics.raycast, nullptr, args, collided)) {
        AimVisibilityUnavailable("consulta Raycast"); return Aim::Visibility::Unknown;
    }
    if (!collided) return Aim::Visibility::Clear;
    __try {
        void* collider = InvokeObj(s_aimPhysics.collider, hit, nullptr);
        void* hitTransform = collider ? InvokeObj(mGetTrans, collider, nullptr) : nullptr;
        void* zo = ReadP(reinterpret_cast<void*>(target.entity), Off::Z_obj);
        void* targetTransform = zo ? InvokeObj(mGetTrans, zo, nullptr) : nullptr;
        if (!hitTransform || !targetTransform) {
            AimVisibilityUnavailable("collider/entidade destruida"); return Aim::Visibility::Unknown;
        }
        bool belongsToTarget = hitTransform == targetTransform;
        void* childArgs[] = { targetTransform };
        if (!belongsToTarget && !InvokeAimBool(s_aimPhysics.isChild, hitTransform, childArgs, belongsToTarget)) {
            AimVisibilityUnavailable("hierarquia do collider"); return Aim::Visibility::Unknown;
        }
        return belongsToTarget ? Aim::Visibility::TargetHit : Aim::Visibility::Blocked;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        AimVisibilityUnavailable("acesso ao collider"); return Aim::Visibility::Unknown;
    }
}

static MonoClassField* s_aimCameraField = nullptr;
static MonoMethod* s_aimGetAngle = nullptr;
static MonoMethod* s_aimSetAngle = nullptr;
static MonoMethod* s_aimRotate = nullptr;
static bool ResolveAimCamera() {
    if (s_aimCameraField && s_aimGetAngle && s_aimSetAngle && s_aimRotate) return true;
    MonoClass* camera = pClassFrom(s_img, "", "PlayerCamera");
    if (!camera || !cPlayer || !cTrans) return false;
    s_aimCameraField = pFieldFrom(cPlayer, "cam");
    s_aimGetAngle = pMethodFrom(camera, "get_Angle", 0);
    s_aimSetAngle = pMethodFrom(camera, "set_Angle", 1);
    s_aimRotate = pMethodFrom(cTrans, "Rotate", 1); // unique Vector3 overload; matches AxialInput
    return s_aimCameraField && s_aimGetAngle && s_aimSetAngle && s_aimRotate;
}
static bool MoveAimCamera(void* local, const Aim::Target& target) {
    Aim::Point direction; float distance;
    if (!ResolveAimCamera() || !Aim::Ray({s_camW.x,s_camW.y,s_camW.z}, target.position, direction, distance)) return false;
    void* camera = ReadP(local, FieldOff(s_aimCameraField));
    if (!camera) return false;
    __try {
        MonoObject* exception = nullptr;
        MonoObject* result = pInvoke(s_aimGetAngle, camera, nullptr, &exception);
        if (exception || !result) return false;
        float current = 0;
        memcpy(&current, pUnbox(result), sizeof(current));
        void* transform = InvokeObj(mGetTrans, camera, nullptr);
        Vec3 euler;
        if (!transform || !GetEulerY(transform, euler) || !Fin(current) || !Fin(euler.y)) return false;
        float smoothing = Config::fSmoothing;
        if (!(smoothing >= 1 && smoothing <= 8)) smoothing = 1;
        float pitch = asinf((std::max)(-1.0f, (std::min)(1.0f, direction.y))) * 57.29577951f;
        pitch = current + (pitch - current) / smoothing;
        pitch = (std::max)(-80.0f, (std::min)(80.0f, pitch));
        float yaw = std::remainder(atan2f(direction.x, direction.z) * 57.29577951f - euler.y, 360.0f) / smoothing;
        void* pitchArgs[] = { &pitch };
        pInvoke(s_aimSetAngle, camera, pitchArgs, &exception);
        if (exception) return false;
        Vec3 turn = {0, yaw, 0}; void* yawArgs[] = { &turn };
        pInvoke(s_aimRotate, transform, yawArgs, &exception);
        return !exception;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
static bool AimWindowActive() {
    HWND window = GetForegroundWindow(); DWORD process = 0;
    if (window) GetWindowThreadProcessId(window, &process);
    return process == GetCurrentProcessId() && !Config::bMenuOpen;
}
#include "aim_bridge.inl"
static void WohaxAim(void* local) {
    const bool enabled = local && s_camWok && AimRequested() && AimWindowActive();
    // The managed adapter updates the ShootGun local consumed by both simulation
    // and SyncShotOnline. A connected role never silently turns into camera aim.
    const bool allowSilent = Config::bSilentAim &&
        Aim::CanRedirectShot(static_cast<Aim::GameMode>(s.coopMode));
    const int key = Config::iAimKey;
    const bool down = key > 0 && key <= 255 && (GetAsyncKeyState(key) & 0x8000);
    const bool automatic = Config::bAutoAim || Config::bAutoFire;
    const bool firing = s_aimActivation.Update(enabled, automatic, key, Config::iAimMode, down);
    ConfigureAimRange(enabled && (firing || Config::bTriggerbot));
    if (!enabled) { s_aimLock.Clear(); ClearAimBridge(); return; }
    const auto candidates = Aim::Rank(s_aimTargets, s_aimCount, AimPolicy(), s_aimLock, PiNow());
    if (!firing && !Config::bTriggerbot) { ClearAimBridge(); return; }
    int candidateCount = 0;
    while (candidateCount < Aim::MaxTargets && candidates[candidateCount] >= 0) ++candidateCount;
    static long long nextDiagnostic = 0;
    const bool diagnostic = PiNow() >= nextDiagnostic;
    if (diagnostic) {
        nextDiagnostic = PiNow() + 5000000;
        int behind = 0;
        for (int i=0; i<s_aimCount; ++i) if (s_aimTargets[i].angle > 90) ++behind;
        Log::Infof("[AIM-SELECT] mode=%d 360=%d silent=%d firing=%d candidates=%d rear=%d",
            s.coopMode, Config::b360Mode ? 1 : 0, allowSilent ? 1 : 0, firing ? 1 : 0, candidateCount, behind);
    }
    // Bounded raycast work without starving targets beyond the first three.
    for (int checked = 0; checked < (std::min)(candidateCount, Aim::VisibilityBudget); ++checked) {
        const int index = candidates[(s_visibilityCursor + checked) % candidateCount];
        const auto& target = s_aimTargets[index];
        if (!AimTargetAlive(target)) continue;
        const auto visibility = ManagedAimVisibility(local, target);
        if (!Aim::CanAim(visibility) || !AimTargetAlive(target)) continue;
        s_aimLock.Select(target.entity, PiNow());
        if (diagnostic) Log::Infof("[AIM-SELECT] escolhido: angulo=%.1f distancia=%.1f", target.angle, target.distance);
        s_visibilityCursor = 0;
        const bool move = Aim::ShouldMoveVisible(firing, Config::bAimbot, automatic,
                                                 Config::bSilentAim, allowSilent);
        if (move && !allowSilent && !MoveAimCamera(local, target)) { ClearAimBridge(); return; }
        const int flags = (allowSilent && firing ? 1 : 0) |
            (Config::bAutoFire ? 2 : 0) | (Config::bTriggerbot ? 4 : 0);
        PublishAimBridge(local, &target, flags);
        return;
    }
    s_aimLock.Clear();
    s_visibilityCursor = candidateCount ? (s_visibilityCursor + Aim::VisibilityBudget) % candidateCount : 0;
    PublishAimBridge(local, nullptr, Config::bTriggerbot ? 4 : 0);
}
