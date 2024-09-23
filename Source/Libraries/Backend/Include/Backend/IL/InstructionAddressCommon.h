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
#include "Program.h"

namespace IL {
    /// Visit the complete address chain in reverse order
    /// \param program given program
    /// \param address top address to decompose and visit
    /// \param functor visitor
    template<typename F>
    void VisitGlobalAddressChainReverse(IL::Program& program, IL::ID address, F&& functor) {
        for (;;) {
            InstructionRef<> ref(program.GetIdentifierMap().Get(address));
            if (!ref) {
                functor(address, false);
                return;
            }

            const IL::Instruction* instr = ref.Get();

            switch (instr->opCode) {
                default: {
                    // Unknown addressing, report it to avoid partial-but-similar cases
                    if (!functor(address, false)) {
                        return;
                    }
                    return;
                }
                case OpCode::AddressChain: {
                    auto _instr = instr->As<AddressChainInstruction>();
                    ASSERT(_instr->chains.count > 0, "Invalid address chain");

                    // Report all chains
                    for (int32_t i = _instr->chains.count - 1; i >= 0; i--) {
                        if (!functor(_instr->chains[static_cast<uint32_t>(i)].index, i == 0)) {
                            return;
                        }
                    }

                    address = _instr->composite;
                    break;
                }
                case OpCode::Alloca: {
                    // End of chain
                    if (!functor(address, false)) {
                        return;
                    }
                    return;
                }
            }
        }
    }

    /// Get the resource from a chain
    /// \param program given program
    /// \param address address to traverse
    /// \return invalid if not found
    inline IL::ID GetResourceFromAddressChain(Program& program, IL::ID address) {
        // Traverse back until we find the resource
        ID resourceId{InvalidID};
        IL::VisitGlobalAddressChainReverse(program, address, [&](ID id, bool) {
            if (IsPointerToResourceType(program.GetTypeMap().GetType(id))) {
                resourceId = id;
                return false;
            }

            // Next!
            return true;
        });
        
        return resourceId;
    }
}
