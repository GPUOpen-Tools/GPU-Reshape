#pragma once

// Backend
#include <Backend/IL/Device.h>

// Common
#include <Common/Assert.h>

// Std
#include <cstdint>

namespace IL {
    struct CPUEmitter {
        /// Generic handle type
        template<typename T>
        using Handle = T;

        /// Specify device
        static constexpr Device kDevice = Device::kCPU;

        /// Assertion
        /// \param condition expected to be true 
        /// \param message message on failure
        void Assert(bool condition, const char* message) {
            ASSERT(condition, message);
        }

        /// Add two values
        /// \param lhs let hand side
        /// \param rhs right hand side
        /// \return resulting value
        template<typename T>
        T Add(T lhs, T rhs) {
            return lhs + rhs;
        }

        /// Subtract two values
        /// \param lhs let hand side
        /// \param rhs right hand side
        /// \return resulting value
        template<typename T>
        T Sub(T lhs, T rhs) {
            return lhs - rhs;
        }

        /// Multiply two values
        /// \param lhs let hand side
        /// \param rhs right hand side
        /// \return resulting value
        template<typename T>
        T Mul(T lhs, T rhs) {
            return lhs * rhs;
        }

        /// Divide two values
        /// \param lhs let hand side
        /// \param rhs right hand side
        /// \return resulting value
        template<typename T>
        T Div(T lhs, T rhs) {
            return lhs / rhs;
        }

        /// Perform a bit-shift left
        /// \param lhs the value to shift
        /// \param rhs the number of places to shift
        /// \return shifted value
        template<typename T>
        T BitShiftLeft(T lhs, T rhs) {
            return lhs << rhs;
        }

        /// Perform a bit-shift right
        /// \param lhs the value to shift
        /// \param rhs the number of places to shift
        /// \return shifted value
        template<typename T>
        T BitShiftRight(T lhs, T rhs) {
            return lhs >> rhs;
        }

        /// Literal emitter
        uint32_t UInt32(uint32_t value) {
            return value;
        }
    };
}
