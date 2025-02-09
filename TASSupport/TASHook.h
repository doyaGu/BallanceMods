#pragma once

#include <mutex>

#include "Hook.h"

#include "CKTimeManager.h"
#include "CKInputManager.h"

template <typename T>
T ForceReinterpretCast(void *base, size_t offset) {
    void *p = static_cast<char *>(base) + offset;
    return *reinterpret_cast<T *>(&p);
}

template <typename Class>
class PreProcessHook : public Class {
public:
    using MethodType = decltype(&Class::PreProcess);
    using HookType = Hook<MethodType>;
    using InterceptorType = HookInterceptor<MethodType>;
    using CallbackType = typename InterceptorType::CallbackType;

    static bool Enable(CKBaseManager *manager) {
        static std::once_flag flag;
        std::call_once(flag, [&]() {
            // Extract function pointer from vtable
            void **vtable = *reinterpret_cast<void ***>(manager);
            auto preProcessFunc = *reinterpret_cast<MethodType *>(&vtable[5]);

            // Enable hook and interceptor
            if (s_PreProcessHook.Enable(preProcessFunc, &PreProcessHook::PreProcessDetour)) {
                s_PreProcessInterceptor = new HookInterceptor<MethodType>(s_PreProcessHook);
            }
        });

        return s_PreProcessHook.GetOriginal() != nullptr;
    }

    static void Disable() {
        if (s_PreProcessHook.GetOriginal()) {
            s_PreProcessHook.Disable();
            delete s_PreProcessInterceptor;
            s_PreProcessInterceptor = nullptr;
        }
    }

    // Get the original (unhooked) function pointer
    static MethodType GetOriginal() {
        return s_PreProcessHook.GetOriginal();
    }

    // Set a pre-execution callback
    static void AddPreCallback(CallbackType callback) {
        if (s_PreProcessInterceptor) {
            s_PreProcessInterceptor->AddPreCallback(std::move(callback));
        }
    }

    // Remove all pre-execution callbacks
    static void ClearPreCallbacks() {
        if (s_PreProcessInterceptor) {
            s_PreProcessInterceptor->ClearPreCallbacks();
        }
    }

    // Set a post-execution callback
    static void AddPostCallback(CallbackType callback) {
        if (s_PreProcessInterceptor) {
            s_PreProcessInterceptor->AddPostCallback(std::move(callback));
        }
    }

    // Remove all post-execution callbacks
    static void ClearPostCallbacks() {
        if (s_PreProcessInterceptor) {
            s_PreProcessInterceptor->ClearPostCallbacks();
        }
    }

private:
    CKERROR PreProcessDetour() {
        if (s_PreProcessInterceptor) {
            return s_PreProcessInterceptor->InvokeMethod(this);
        }
        return s_PreProcessHook.InvokeMethodOriginal(this);
    }

    static HookType s_PreProcessHook;
    static HookInterceptor<MethodType> *s_PreProcessInterceptor;
};

template <typename Class>
typename PreProcessHook<Class>::HookType PreProcessHook<Class>::s_PreProcessHook;

template <typename Class>
HookInterceptor<typename PreProcessHook<Class>::MethodType> *PreProcessHook<Class>::s_PreProcessInterceptor = nullptr;

using CKTimeManagerHook = PreProcessHook<CKTimeManager>;
using CKInputManagerHook = PreProcessHook<CKInputManager>;

// Make physics engine deterministic
bool HookPhysicsRT();
void UnhookPhysicsRT();

// Make random building block deterministic
bool HookRandom();
bool UnhookRandom();
