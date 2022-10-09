#pragma once

// Backend
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

    /// Get general information about this feature
    /// \return feature info
    virtual FeatureInfo GetInfo() = 0;

    /// Get the hook table of this feature
    /// \return constructed hook table
    virtual FeatureHookTable GetHookTable() { return {}; }

    /// Collect all produced messages
    /// \param storage the output storage
    virtual void CollectMessages(IMessageStorage* storage) { }
};
