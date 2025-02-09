#pragma once

#include <tuple>
#include <functional>
#include <utility>
#include <type_traits>

#include "MinHook.h"

//--------------------------------------------------------------------------
// FunctionTraits: Deduces the return type and argument list of a callable.
//--------------------------------------------------------------------------

// Primary template (unused)
template <typename T>
struct FunctionTraits;

// --- Specialization for free functions ---
template <typename Ret, typename... Args>
struct FunctionTraits<Ret(*)(Args...)> {
    using ReturnType = Ret;
    using ArgumentsTuple = std::tuple<Args...>;
    // For free functions, we want a simple callable: Ret(Args...)
    using FunctionType = std::function<Ret(Args...)>;
    using CallbackType = std::function<void(Args...)>;
};

// --- Specialization for non-const member functions ---
template <typename Ret, typename Class, typename... Args>
struct FunctionTraits<Ret(Class::*)(Args...)> {
    using ReturnType = Ret;
    using ClassType = Class;
    using ArgumentsTuple = std::tuple<Args...>;
    // For member functions, we want a callable that takes an instance pointer first.
    using FunctionType = std::function<Ret(Class *, Args...)>;
    using CallbackType = std::function<void(Class *, Args...)>;
};

// --- Specialization for const member functions ---
template <typename Ret, typename Class, typename... Args>
struct FunctionTraits<Ret(Class::*)(Args...) const> {
    using ReturnType = Ret;
    using ClassType = const Class;
    using ArgumentsTuple = std::tuple<Args...>;
    using FunctionType = std::function<Ret(const Class *, Args...)>;
    using CallbackType = std::function<void(Class *, Args...)>;
};

//--------------------------------------------------------------------------
// Hook<T>: A generic hook class that supports installing a detour (using MinHook)
//          and provides methods to invoke the original function.
//          (It does not manage any callbacks.)
//--------------------------------------------------------------------------

template <typename T>
class Hook {
public:
    using HookType = T;
    using Traits = FunctionTraits<HookType>;
    using ReturnType = typename Traits::ReturnType;
    using ArgsTuple = typename Traits::ArgumentsTuple;

    Hook() = default;

    ~Hook() {
        Disable();
    }

    // Enable the hook. 'target' is the original function pointer,
    // 'detour' is our replacement function.
    // (Both must be convertible to HookType.)
    template <typename TargetType, typename DetourType>
    bool Enable(TargetType target, DetourType detour) {
        if (m_Original)
            return false; // already hooked

        // Use reinterpret_cast since MinHook works with LPVOID.
        LPVOID pTarget = *reinterpret_cast<LPVOID *>(&target);
        LPVOID pDetour = *reinterpret_cast<LPVOID *>(&detour);
        LPVOID pOriginal = nullptr;

        if (MH_CreateHook(pTarget, pDetour, &pOriginal) != MH_OK ||
            MH_EnableHook(pTarget) != MH_OK) {
            m_Target = nullptr;
            m_Detour = nullptr;
            m_Original = nullptr;
            return false;
        }

        m_Target = *reinterpret_cast<HookType *>(&target);
        m_Detour = *reinterpret_cast<HookType *>(&detour);
        m_Original = *reinterpret_cast<HookType *>(&pOriginal);
        return true;
    }

    // Disable the hook.
    void Disable() {
        if (m_Target) {
            LPVOID pTarget = *reinterpret_cast<LPVOID *>(&m_Target);
            MH_DisableHook(pTarget);
            MH_RemoveHook(pTarget);
            m_Target = nullptr;
            m_Detour = nullptr;
            m_Original = nullptr;
        }
    }

    // Get the original (unhooked) function pointer.
    HookType GetOriginal() const {
        return m_Original;
    }

    // --- InvokeOriginal() for free functions ---
    template <typename... Args>
    ReturnType InvokeOriginal(Args &&... args) {
        return m_Original(std::forward<Args>(args)...);
    }

    // --- InvokeMethodOriginal() for member functions ---
    template <typename... Args>
    ReturnType InvokeMethodOriginal(typename Traits::ClassType *instance, Args &&... args) {
        return (instance->*m_Original)(std::forward<Args>(args)...);
    }

private:
    HookType m_Target = nullptr;
    HookType m_Detour = nullptr;
    HookType m_Original = nullptr;
};

//--------------------------------------------------------------------------
// HookInterceptor<T>: A powerful and efficient wrapper around Hook<T> that adds
//                      extensive interception capabilities. It supports multiple
//                      pre- and post-callbacks with ordering, removal, and a fast
//                      execution path when no callbacks are registered.
//--------------------------------------------------------------------------

template <typename T>
class HookInterceptor {
public:
    using HookType = T;
    using Traits = FunctionTraits<HookType>;
    using ReturnType = typename Traits::ReturnType;
    using CallbackType = typename Traits::CallbackType;
    using ClassType = typename Traits::ClassType;

    // Construct the interceptor with a reference to an existing Hook<T>.
    explicit HookInterceptor(Hook<T> &hook) : m_Hook(hook) {}

    // ----------------------------
    // Invocation methods
    // ----------------------------

    // For free functions: Invoke the original function while executing all
    // registered pre- and post-callbacks.
    template <typename... Args>
    ReturnType Invoke(Args &&... args) {
        // Fast-path: if no callbacks are registered, call the original directly.
        if (m_PreCallbacks.empty() && m_PostCallbacks.empty()) {
            return m_Hook.InvokeOriginal(std::forward<Args>(args)...);
        }

        // Execute all pre-callbacks.
        for (auto &callback : m_PreCallbacks) {
            callback(std::forward<Args>(args)...);
        }

        // Invoke the original function.
        ReturnType ret = m_Hook.InvokeOriginal(std::forward<Args>(args)...);

        // Execute all post-callbacks.
        for (auto &callback : m_PostCallbacks) {
            callback(std::forward<Args>(args)...);
        }
        return ret;
    }

    // For member functions: Invoke the original method while executing all
    // registered pre- and post-callbacks.
    template <typename... Args>
    ReturnType InvokeMethod(ClassType *instance, Args &&... args) {
        if (m_PreCallbacks.empty() && m_PostCallbacks.empty()) {
            return m_Hook.InvokeMethodOriginal(instance, std::forward<Args>(args)...);
        }

        for (auto &callback : m_PreCallbacks) {
            callback(instance, std::forward<Args>(args)...);
        }

        ReturnType ret = m_Hook.InvokeMethodOriginal(instance, std::forward<Args>(args)...);

        for (auto &callback : m_PostCallbacks) {
            callback(instance, std::forward<Args>(args)...);
        }
        return ret;
    }

    // ----------------------------
    // Callback management methods
    // ----------------------------

    size_t AddPreCallback(CallbackType callback) {
        m_PreCallbacks.push_back(std::move(callback));
        return m_PreCallbacks.size() - 1;
    }

    size_t AddPostCallback(CallbackType callback) {
        m_PostCallbacks.push_back(std::move(callback));
        return m_PostCallbacks.size() - 1;
    }

    void InsertPreCallback(CallbackType callback) {
        m_PreCallbacks.insert(m_PreCallbacks.begin(), std::move(callback));
    }

    void InsertPostCallback(CallbackType callback) {
        m_PostCallbacks.insert(m_PostCallbacks.begin(), std::move(callback));
    }

    bool RemovePreCallback(size_t index) {
        if (index < m_PreCallbacks.size()) {
            m_PreCallbacks.erase(m_PreCallbacks.begin() + index);
            return true;
        }
        return false;
    }

    bool RemovePostCallback(size_t index) {
        if (index < m_PostCallbacks.size()) {
            m_PostCallbacks.erase(m_PostCallbacks.begin() + index);
            return true;
        }
        return false;
    }

    void ClearPreCallbacks() {
        m_PreCallbacks.clear();
    }

    void ClearPostCallbacks() {
        m_PostCallbacks.clear();
    }

    void Clear() {
        m_PreCallbacks.clear();
        m_PostCallbacks.clear();
    }

private:
    Hook<T> &m_Hook;
    std::vector<CallbackType> m_PreCallbacks;
    std::vector<CallbackType> m_PostCallbacks;
};
