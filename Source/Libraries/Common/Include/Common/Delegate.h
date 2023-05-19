#pragma once

template<typename>
struct Delegate;

/// Simple delegate, provides a single indirection for function pointers
template<typename R, typename... A>
struct Delegate<R(A...)> {
    using Handle = R(*)(void*, A...);

    /// Default
    Delegate() = default;

    /// Frame constructor
    Delegate(void* frame, Handle handle) : frame(frame), handle(handle) {

    }

    /// Invoke the delegate
    R Invoke(A... args) const {
        return handle(frame, args...);
    }

    /// Invoke the delegate
    void TryInvoke(A... args) const {
        if (!IsValid()) {
            return;
        }

        handle(frame, args...);
    }

    /// Is this delegate valid?
    bool IsValid() const {
        return handle != nullptr;
    }

private:
    void* frame{nullptr};
    Handle handle{nullptr};
};

/// Delegate creators
namespace Detail {
    template<typename>
    struct DelegateCreator;

    /// Member creator, single indirection as the fptr is known at compile time
    template<typename C, typename R, typename... A>
    struct DelegateCreator<R(C::*)(A...)> {
        template<R(C::*handle)(A...)>
        static Delegate<R(A...)> MakeFrameProxy(C* frame) {
            return Delegate<R(A...)>(frame, [](void* frame, A... args) {
                return (static_cast<C*>(frame)->*handle)(args...);
            });
        }
    };

    /// Const member creator, single indirection as the fptr is known at compile time
    template<typename C, typename R, typename... A>
    struct DelegateCreator<R(C::*)(A...) const> {
        template<R(C::*handle)(A...) const>
        static Delegate<R(A...)> MakeFrameProxy(C* frame) {
            return Delegate<R(A...)>(frame, [](void* frame, A... args) {
                return (static_cast<const C*>(frame)->*handle)(args...);
            });
        }
    };
}

/// Bind a delegate
#define BindDelegate(INSTANCE, DELEGATE) Detail::DelegateCreator<decltype(&DELEGATE)>::template MakeFrameProxy<&DELEGATE>(INSTANCE)
