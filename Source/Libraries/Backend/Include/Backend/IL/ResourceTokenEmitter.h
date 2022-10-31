#pragma once

// Backend
#include "ID.h"
#include "ResourceTokenPacking.h"

namespace IL {
    template<typename E>
    struct ResourceTokenEmitter {
        ResourceTokenEmitter(E& emitter, ::IL::ID resourceID) : emitter(emitter) {
            token = emitter.ResourceToken(resourceID);
        }

        /// Get the resource physical UID
        ::IL::ID GetPUID() const {
            return emitter.BitAnd(emitter.BitShiftRight(token, emitter.UInt32(kResourceTokenPUIDShift)), emitter.UInt32(kResourceTokenPUIDMask));
        }

        /// Get the resource type
        ::IL::ID GetType() const {
            return emitter.BitAnd(emitter.BitShiftRight(token, emitter.UInt32(kResourceTokenTypeShift)), emitter.UInt32(kResourceTokenTypeMask));
        }

        /// Get the resource sub-resource base
        ::IL::ID GetSRB() const {
            return emitter.BitAnd(emitter.BitShiftRight(token, emitter.UInt32(kResourceTokenSRBShift)), emitter.UInt32(kResourceTokenSRBMask));
        }

    private:
        /// Underlying token
        ::IL::ID token;

        /// Current emitter
        E& emitter;
    };
}
