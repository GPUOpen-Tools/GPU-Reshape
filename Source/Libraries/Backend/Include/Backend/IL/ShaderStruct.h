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

namespace IL {
    template<typename T>
    struct ShaderStruct {
        ShaderStruct(IL::ID data) : data(data) {
        
        }

        /// Get a value within the struct
        /// Must be dword aligned
        /// \param emitter instruction emitter to use
        /// \return dword value
        template<auto M, typename E>
        IL::ID Get(E& emitter) {
            static T dummy;
            size_t offset = reinterpret_cast<size_t>(&(dummy.*M)) - reinterpret_cast<size_t>(&dummy);
            ASSERT(offset % sizeof(uint32_t) == 0, "Non-dword aligned offset");
            return emitter.Extract(data, emitter.GetProgram()->GetConstants().UInt(offset / sizeof(uint32_t))->id);
        }

    private:
        /// Underlying data
        IL::ID data;
    };
}
