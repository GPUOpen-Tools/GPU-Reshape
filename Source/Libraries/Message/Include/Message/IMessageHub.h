#pragma once

// Common
#include <Common/IComponent.h>

// Std
#include <string_view>

class IMessageHub : public TComponent<IMessageHub> {
public:
    COMPONENT(IMessageHub);

    /// Add a new message
    /// \param name name of the message, fx. system name
    /// \param message message contents
    /// \param count number of messages
    virtual void Add(const std::string_view& name, const std::string_view& message, uint32_t count) = 0;
};