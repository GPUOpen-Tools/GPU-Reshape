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

// Common
#include <Common/ComRef.h>

// Backend
#include <Backend/ShaderProgram/IShaderProgram.h>
#include <Backend/ShaderData/ShaderData.h>

class SRBMaskingShaderProgram final : public IShaderProgram {
public:
    /// Constructor
    /// \param initializationMaskBufferID
    SRBMaskingShaderProgram(ShaderDataID initializationMaskBufferID);

    /// Install the masking program
    /// \return
    bool Install();

    /// IShaderProgram
    void Inject(IL::Program &program) override;

    /// Interface querying
    void *QueryInterface(ComponentID id) override {
        switch (id) {
            case IComponent::kID:
                return static_cast<IComponent*>(this);
            case IShaderProgram::kID:
                return static_cast<IShaderProgram*>(this);
        }

        return nullptr;
    }

    /// Get the mask ID
    ShaderDataID GetMaskEventID() const {
        return maskEventID;
    }

    /// Get the PUID ID
    ShaderDataID GetPUIDEventID() const {
        return puidEventID;
    }

private:
    /// Shared data host
    ComRef<IShaderDataHost> shaderDataHost{nullptr};

    /// Shader data
    ShaderDataID initializationMaskBufferID{InvalidShaderDataID};
    ShaderDataID maskEventID{InvalidShaderDataID};
    ShaderDataID puidEventID{InvalidShaderDataID};
};
