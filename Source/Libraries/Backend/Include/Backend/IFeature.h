#pragma once

// Backend
#include "FeatureHookTable.h"

class IFeature {
public:
    /// Get the hook table of this feature
    /// \return constructed hook table
    virtual FeatureHookTable GetHookTable() = 0;
};
