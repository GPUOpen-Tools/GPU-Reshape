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
#include "ResourceToken.h"
#include "BufferDescriptor.h"
#include "TextureDescriptor.h"

struct ResourceInfo {
    ResourceInfo() : token{} {
        textureDescriptor = {};
    }
    
    /// Create a texture info
    /// \param token given token
    /// \param isVolumetric true if volumetric
    /// \return info
    static ResourceInfo Texture(const ResourceToken& token, bool isVolumetric) {
        ResourceInfo info;
        info.token = token;
        info.textureDescriptor = TextureDescriptor {
            .region = {
                .width = token.width,
                .height = token.height,
                .depth = token.depthOrSliceCount
            }
        };
        info.isVolumetric = isVolumetric;
        return info;
    }
    
    /// Create a texture info
    /// \param token given token
    /// \param isVolumetric true if volumetric
    /// \param texture given texture
    /// \return info
    static ResourceInfo Texture(const ResourceToken& token, bool isVolumetric, const TextureDescriptor& texture) {
        ResourceInfo info;
        info.token = token;
        info.textureDescriptor = texture;
        info.isVolumetric = isVolumetric;
        return info;
    }

    /// Create a buffer info
    /// \param token given token
    /// \return info
    static ResourceInfo Buffer(const ResourceToken& token) {
        ResourceInfo info;
        info.token = token;
        info.bufferDescriptor = BufferDescriptor {
            .width = token.width
        };
        return info;
    }

    /// Create a buffer info
    /// \param token given token
    /// \param buffer given buffer
    /// \return info
    static ResourceInfo Buffer(const ResourceToken& token, const BufferDescriptor& buffer) {
        ResourceInfo info;
        info.token = token;
        info.bufferDescriptor = buffer;
        return info;
    }

    /// PRMT token
    ResourceToken token;

    /// Is this resource volumetric? i.e. we assume depth, otherwise sliced
    bool isVolumetric{false};

    /// Descriptor data
    union {
        TextureDescriptor textureDescriptor;
        BufferDescriptor bufferDescriptor;
    };
};
