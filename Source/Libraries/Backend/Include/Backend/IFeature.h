#pragma once

// Backend
#include "FeatureHookTable.h"

class IMessageStorage;

class IFeature {
public:
    /// Get the hook table of this feature
    /// \return constructed hook table
    virtual FeatureHookTable GetHookTable() = 0;

    /// Collect all produced messages
    /// \param storage the output storage
    virtual void CollectMessages(IMessageStorage* storage) = 0;
};
