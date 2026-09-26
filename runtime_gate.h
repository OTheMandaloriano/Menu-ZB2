#pragma once
#include <Windows.h>

// Never wait for the game/render thread: skip the update or frame on contention.
namespace RuntimeGate {
inline SRWLOCK mutex = SRWLOCK_INIT;
class TryScope {
    bool owned;
public:
    TryScope() : owned(TryAcquireSRWLockExclusive(&mutex) != FALSE) {}
    ~TryScope() { Release(); }
    explicit operator bool() const { return owned; }
    void Release() { if (owned) { ReleaseSRWLockExclusive(&mutex); owned = false; } }
    TryScope(const TryScope&) = delete;
    TryScope& operator=(const TryScope&) = delete;
};
}
