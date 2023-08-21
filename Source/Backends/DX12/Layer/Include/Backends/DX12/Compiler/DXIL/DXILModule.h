// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

// Layer
#include <Backends/DX12/Compiler/IDXModule.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>

// Backend
#include <Backend/IL/Program.h>

/// DXIL bytecode module
class DXILModule final : public IDXModule {
public:
    ~DXILModule();

    /// Construct
    /// \param allocators shared allocators
    DXILModule(const Allocators &allocators);

    /// Construct from top level program
    /// \param allocators shared allocators
    /// \param program the top level program
    DXILModule(const Allocators &allocators, IL::Program* program);

    /// Copy this module
    /// \param module destination module
    void CopyTo(DXILModule* module);

    /// Overrides
    IDXModule* Copy() override;
    bool Parse(const DXParseJob& job) override;
    IL::Program *GetProgram() override;
    GlobalUID GetInstrumentationGUID() override;
    bool Compile(const DXCompileJob& job, DXStream& out) override;
    IDXDebugModule *GetDebug() override;
    DXCodeOffsetTraceback GetCodeOffsetTraceback(uint32_t codeOffset) override;
    const char* GetLanguage() override;

    /// Get the binding info
    /// \return
    const DXILBindingInfo& GetBindingInfo() {
        return table.bindingInfo;
    }

private:
    /// All physical blocks
    DXILPhysicalBlockTable table;

    /// Program
    IL::Program* program{nullptr};

    /// Nested module?
    bool nested{true};

private:
    Allocators allocators;
};
