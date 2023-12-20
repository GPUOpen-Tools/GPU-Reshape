// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

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
