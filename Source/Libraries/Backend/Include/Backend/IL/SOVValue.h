#pragma once

// Backend
#include "ID.h"

// Common
#include <Common/Assert.h>

namespace IL {
    struct SOVValue {
        SOVValue() = default;

        /// Initialize to vector
        SOVValue(IL::ID vector) : isScalarized(false) {
            values[0] = vector;
        }

        /// Initialize to scalar
        SOVValue(IL::ID scalarX, IL::ID scalarY, IL::ID scalarZ, IL::ID scalarW) : isScalarized(true) {
            values[0] = scalarX;
            values[1] = scalarY;
            values[2] = scalarZ;
            values[3] = scalarW;
        }

        /// Get the vectorized value
        IL::ID GetVector() const {
            ASSERT(!isScalarized, "SOVValue mismatch");
            return values[0];
        }

        /// Get the scalarized value at index
        IL::ID GetScalar(uint32_t i) const {
            ASSERT(isScalarized, "SOVValue mismatch");
            return values[i];
        }

        /// Is this a scalar?
        bool IsScalarized() const {
            return isScalarized;
        }

        /// Is this a vector?
        bool IsVectorized() const {
            return !isScalarized;
        }

    private:
        /// Scalar?
        bool isScalarized{false};

        /// Vector or scalar values
        IL::ID values[4]{IL::InvalidID};
    };
}
