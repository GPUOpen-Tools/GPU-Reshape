#pragma once

// Automation
#include <Test/Automation/Connection/InstrumentationConfig.h>
#include <Test/Automation/Connection/PooledMessage.h>

// Bridge
#include <Bridge/EndpointConfig.h>

// Common
#include <Common/ComRef.h>

// Schemas
#include <Schemas/Diagnostic.h>
#include <Schemas/HostResolve.h>

// Std
#include <map>
#include <mutex>

// Forward declarations
class RemoteClientBridge;
class PoolingListener;

/// Connections are a convenient way to interface with applications for automation.
/// They are not written for high performance messaging, but instead for ease of use.
/// For high performance messaging, use remote client bridges with listeners.
class Connection : public IComponent {
public:
    /// Constructor
    Connection();

    /// Deconstructor
    ~Connection();

    /** Connection currently handles common pooling, such as instrumentation and features, this will be moved elsewhere */

    /// Install this connection
    /// \param resolve endpoint to resolve to
    /// \param processName process name to connect to
    /// \return success state
    bool Install(const EndpointResolve& resolve, const std::string_view& processName);

    /// Instrument globally
    /// \param config given configuration
    /// \return diagnostic report message
    PooledMessage<InstrumentationDiagnosticMessage> InstrumentGlobal(const InstrumentationConfig& config);

    /// Pool a message
    /// \tparam T given type of the message
    /// \param mode pooling mode, describes the storage mechanism
    /// \return pending message
    template<typename T>
    PooledMessage<T> Pool(PoolingMode mode = PoolingMode::StoreAndRelease) {
        return PooledMessage<T>(listener.GetUnsafe(), mode);
    }

    /// Get a feature bit
    /// \param name name of the feature to query
    /// \return feature bit
    uint64_t GetFeatureBit(std::string_view name) const {
        return featureMap.at(std::string(name));
    }

    /// Get the attached process id
    uint32_t GetProcessID() const {
        return processID;
    }

private:
    /// Create a connection
    /// \param processName target process name
    /// \return success state
    bool CreateConnection(const std::string_view& processName);

    /// Pool all host servers
    /// \param processName target process name
    /// \param pool messaging pool
    /// \return success state
    bool PoolHostServers(const std::string_view& processName, PooledMessage<HostDiscoveryMessage>& pool);

    /// Pool all target features
    bool PoolFeatures();

    /// Commit all messages
    void Commit();

private:
    /// Remote bridge
    ComRef<RemoteClientBridge> bridge;

    /// Pooling listener, handles all asynchronous event and signalling
    ComRef<PoolingListener> listener;

    /// Internal stream
    MessageStream stream;
    MessageStreamView<> view;

    /// Pooled feature map
    std::map<std::string, uint64_t> featureMap;

    /// Attached process id
    uint32_t processID{UINT32_MAX};
};
