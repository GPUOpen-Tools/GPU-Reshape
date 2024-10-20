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
#include "FeatureActivationStage.h"
#include "FeatureHookTable.h"
#include "FeatureInfo.h"

// Common
#include <Common/IComponent.h>

// Forward declarations
class IMessageStorage;

class IFeature : public TComponent<IFeature> {
public:
    COMPONENT(IFeature);

    /// Install this feature
    /// \return success code
    virtual bool Install() = 0;

    /// Post install this feature
    /// Useful for streaming reliant operations
    /// \return success code
    virtual bool PostInstall() { return true; }

    /// Get general information about this feature
    /// \return feature info
    virtual FeatureInfo GetInfo() = 0;

    /// Get the hook table of this feature
    /// \return constructed hook table
    virtual FeatureHookTable GetHookTable() { return {}; }

    /// Activate this feature
    /// \param stage stage being activated
    virtual void Activate(FeatureActivationStage stage) { }

    /// Deactivate this feature
    /// i.e. the feature is no longer in us
    virtual void Deactivate() { }
    
    /// Collect all produced messages
    /// \param storage the output storage
    virtual void CollectMessages(IMessageStorage* storage) { }
};
