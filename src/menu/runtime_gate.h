#pragma once
#include <Windows.h>

// UI-only serialization (Present, ResizeBuffers, WndProc).
// Unity updates exchange snapshots and must never acquire this gate.
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
