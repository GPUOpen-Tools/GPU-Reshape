#pragma once

// Backend
#include <Backend/IL/Device.h>
#include <Backend/IL/ExtendedOp.h>

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

        /// Check for equality between two values
        /// \param lhs lhs to equate
        /// \param rhs lhs to equate
        /// \return true if equal
        template<typename T>
        bool Equal(T lhs, T rhs) {
            return lhs == rhs;
        }

        /// Select between two values
        /// \param condition conditional value
        /// \param passed value selected if true
        /// \param failed value selected if false
        /// \return selected value
        template<typename T>
        T Select(bool condition, T passed, T failed) {
            return condition ? passed : failed;
        }

        /// Perform an extended instruction
        /// \param op given op code
        /// \param a first argument
        /// \param ix remaining arguments
        /// \return result value
        template<typename T, typename... IX>
        T Extended(Backend::IL::ExtendedOp op, T a, IX... ix) {
            // Fold it down
            T ops[] = {a, ix...};

            switch (op) {
                default:
                    ASSERT(false, "Invalid extended op-code");
                    return {};
                case Backend::IL::ExtendedOp::Min: {
                    return std::min<T>(ops[0], ops[1]);
                }
                case Backend::IL::ExtendedOp::Max: {
                    return std::max<T>(ops[0], ops[1]);
                }
                case Backend::IL::ExtendedOp::Abs: {
                    if constexpr (std::is_unsigned_v<T>) {
                        return ops[0];
                    } else {
                        return std::abs(ops[0]);
                    }
                }
                case Backend::IL::ExtendedOp::Floor: {
                    return static_cast<T>(std::floor(ops[0]));
                }
                case Backend::IL::ExtendedOp::Ceil: {
                    return static_cast<T>(std::ceil(ops[0]));
                }
                case Backend::IL::ExtendedOp::Round: {
                    return static_cast<T>(std::round(ops[0]));
                }
                case Backend::IL::ExtendedOp::Pow: {
                    return static_cast<T>(std::pow(ops[0], ops[1]));
                }
                case Backend::IL::ExtendedOp::Exp: {
                    return static_cast<T>(std::exp(ops[0]));
                }
                case Backend::IL::ExtendedOp::Sqrt: {
                    return static_cast<T>(std::sqrt(ops[0]));
                }
                case Backend::IL::ExtendedOp::FirstBitLow: {
                    return 1u << std::countr_zero(ops[0]);
                }
                case Backend::IL::ExtendedOp::FirstBitHigh: {
                    return std::bit_width(ops[0]) - 1;
                }
            }
        }

        /// Literal emitter
        uint32_t UInt32(uint32_t value) {
            return value;
        }
    };
}
