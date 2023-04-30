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
        ::IL::ID GetPUID() {
            if (puid != IL::InvalidID) {
                return puid;
            }

            return puid = emitter.BitAnd(emitter.BitShiftRight(token, emitter.UInt32(kResourceTokenPUIDShift)), emitter.UInt32(kResourceTokenPUIDMask));
        }

        /// Get the resource type
        ::IL::ID GetType() {
            if (type != IL::InvalidID) {
                return type;
            }

            return type = emitter.BitAnd(emitter.BitShiftRight(token, emitter.UInt32(kResourceTokenTypeShift)), emitter.UInt32(kResourceTokenTypeMask));
        }

        /// Get the resource sub-resource base
        ::IL::ID GetSRB() {
            if (srb != IL::InvalidID) {
                return srb;
            }

            return srb = emitter.BitAnd(emitter.BitShiftRight(token, emitter.UInt32(kResourceTokenSRBShift)), emitter.UInt32(kResourceTokenSRBMask));
        }

    private:
        /// Underlying token
        ::IL::ID token;

        /// Cache
        ::IL::ID puid{IL::InvalidID};
        ::IL::ID type{IL::InvalidID};
        ::IL::ID srb{IL::InvalidID};

        /// Current emitter
        E& emitter;
    };
}
