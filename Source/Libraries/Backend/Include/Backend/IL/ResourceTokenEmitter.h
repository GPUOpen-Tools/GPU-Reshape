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

        /// Get the token
        ::IL::ID GetToken() const {
            return token;
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
