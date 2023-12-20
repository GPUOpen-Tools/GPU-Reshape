#pragma once

// Common
#include <Common/IComponent.h>

class ApplicationData : public TComponent<ApplicationData> {
public:
    COMPONENT(ApplicationData);

    /// Check if the application is still alive
    bool IsAlive() const;

    /// Assigned process id
    uint32_t processID{UINT32_MAX};
};
