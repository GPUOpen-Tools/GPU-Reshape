// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
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

// Backend
#include "ExtendedOp.h"
#include "ID.h"

namespace IL {
    template<typename E>
    struct ExtendedEmitter {
        ExtendedEmitter(E& emitter) : emitter(emitter) {
            
        }

        /// Min two values
        /// \param a left hand value
        /// \param b right hand value 
        /// \return result
        template<typename T>
        T Min(T a, T b) {
            return emitter.Extended(Backend::IL::ExtendedOp::Min, a, b);
        }

        /// Max two values
        /// \param a left hand value
        /// \param b right hand value 
        /// \return result
        template<typename T>
        T Max(T a, T b) {
            return emitter.Extended(Backend::IL::ExtendedOp::Max, a, b);
        }

        /// Clamp a value between two bounds
        /// \param x the value to clamp
        /// \param min the minimum bound
        /// \param max the maximum bound
        /// \return result
        template<typename T>
        T Clamp(T x, T _min, T _max) {
            return Min(_max, Max(_min, x));
        }

        /// Pow two values
        /// May compile to exp(log(a) * b)
        /// \param a left hand value
        /// \param b right hand value 
        /// \return result
        template<typename T>
        T Pow(T a, T b) {
            return emitter.Extended(Backend::IL::ExtendedOp::Pow, a, b);
        }

        /// Get the absolute of a value
        /// \param x value
        /// \return result
        template<typename T>
        T Abs(T x) {
            return emitter.Extended(Backend::IL::ExtendedOp::Abs, x);
        }

        /// Floor a value
        /// \param x value
        /// \return result
        template<typename T>
        T Floor(T x) {
            return emitter.Extended(Backend::IL::ExtendedOp::Floor, x);
        }

        /// Ceil a value
        /// \param x value
        /// \return result
        template<typename T>
        T Ceil(T x) {
            return emitter.Extended(Backend::IL::ExtendedOp::Ceil, x);
        }

        /// Round a value
        /// \param x value
        /// \return result
        template<typename T>
        T Round(T x) {
            return emitter.Extended(Backend::IL::ExtendedOp::Round, x);
        }

        /// Exp a value
        /// \param x value
        /// \return result
        template<typename T>
        T Exp(T x) {
            return emitter.Extended(Backend::IL::ExtendedOp::Exp, x);
        }

        /// Compute the sqrt of a value
        /// \param x value
        /// \return result
        template<typename T>
        T Sqrt(T x) {
            return emitter.Extended(Backend::IL::ExtendedOp::Sqrt, x);
        }

    private:
        /// Current emitter
        E& emitter;
    };
}
