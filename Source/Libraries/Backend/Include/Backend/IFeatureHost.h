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

// Forward declarations
class IComponentTemplate;
class IFeature;

class IFeatureHost : public TComponent<IFeatureHost> {
public:
    COMPONENT(IFeatureHost);

    /// Register a feature
    /// \param feature
    virtual void Register(const ComRef<IComponentTemplate>& feature) = 0;

    /// Deregister a feature
    /// \param feature
    virtual void Deregister(const ComRef<IComponentTemplate>& feature) = 0;

    /// Enumerate all features
    /// \param count if [features] is null, filled with the number of features
    /// \param features if not null, [count] elements are written to
    virtual void Enumerate(uint32_t* count, ComRef<IComponentTemplate>* features) = 0;

    /// Install all features
    /// \param count if [features] is null, filled with the number of features
    /// \param features if not null, [count] elements are written to
    /// \param registry registry to be installed on
    /// \return success state
    virtual bool Install(uint32_t* count, ComRef<IFeature>* features, Registry* registry) = 0;
};
