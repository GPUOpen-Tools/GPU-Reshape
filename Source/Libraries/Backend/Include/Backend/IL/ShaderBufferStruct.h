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
#include <Backend/IL/ID.h>

// Common
#include <Common/Assert.h>

namespace IL {
    template<typename T>
    struct ShaderBufferStruct {
        ShaderBufferStruct() = default;

        /// Constructor
        /// @param buffer buffer to r/w 
        /// @param offset optional, base offset
        ShaderBufferStruct(IL::ID buffer, IL::ID offset = InvalidID) : buffer(buffer), offset(offset) {
        
        }

        /// Get a member value
        template<auto M, typename E>
        IL::ID Get(E& emitter) {
            return emitter.Extract(emitter.LoadBuffer(emitter.Load(buffer), GetDWordOffset<M>(emitter)), emitter.GetProgram()->GetConstants().UInt(0)->id);
        }

        /// Set a member value
        template<auto M, typename E>
        void Set(E& emitter, IL::ID value, uint32_t dwordOffset = 0) {
            emitter.StoreBuffer(emitter.Load(buffer), GetDWordOffset<M>(emitter, dwordOffset), value);
        }

        /// Perform an atomic CAS on a member
        template<auto M, typename E>
        IL::ID AtomicCompareExchange(E& emitter, IL::ID comparator, IL::ID value) {
            return emitter.AtomicCompareExchange(emitter.AddressOf(buffer, GetDWordOffset<M>(emitter)), comparator, value);
        }

        /// Perform an atomic exchange on a member
        template<auto M, typename E>
        IL::ID AtomicExchange(E& emitter, IL::ID value) {
            return emitter.AtomicExchange(emitter.AddressOf(buffer, GetDWordOffset<M>(emitter)), value);
        }

        /// Perform an atomic add on a member
        template<auto M, typename E>
        IL::ID AtomicAdd(E& emitter, IL::ID value) {
            return emitter.AtomicAdd(emitter.AddressOf(buffer, GetDWordOffset<M>(emitter)), value);
        }

        /// Get the address of a member
        template<auto M, typename E>
        IL::ID AddressOf(E& emitter) {
            return emitter.AddressOf(buffer, GetDWordOffset<M>(emitter));
        }

        /// Get the dword offset of a member
        template<auto M, typename E>
        IL::ID GetDWordOffset(E& emitter, uint32_t dwordOffset = 0) {
            IL::ID dwordOffsetId = emitter.GetProgram()->GetConstants().UInt(GetStaticDWordOffset<M>() + dwordOffset)->id;

            // Has base offset?
            if (offset != InvalidID) {
                dwordOffsetId = emitter.Add(dwordOffsetId, offset);
            }

            return dwordOffsetId;
        }

        /// Get the static dword offset of a member
        template<auto M>
        static uint32_t GetStaticDWordOffset() {
            static T dummy;
            size_t offset = reinterpret_cast<size_t>(&(dummy.*M)) - reinterpret_cast<size_t>(&dummy);
            ASSERT(offset % sizeof(uint32_t) == 0, "Non-dword aligned offset");
            return static_cast<uint32_t>(offset / sizeof(uint32_t));
        }

    private:
        IL::ID buffer{InvalidID};
        IL::ID offset{InvalidID};
    };
}
