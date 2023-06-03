#pragma once

// Layer
#include <Backends/DX12/Controllers/IController.h>

// Bridge
#include <Bridge/IBridgeListener.h>

// Common
#include <Common/ComRef.h>
#include <Common/Allocator/Vector.h>

// Std
#include <string_view>
#include <vector>
#include <mutex>
#include <filesystem>
#include <unordered_map>

// Forward declarations
class IBridge;
struct DeviceState;
struct ResourceState;

/// Candidate list
using PDBCandidateList = std::vector<std::string>;

class PDBController final : public IController, public IBridgeListener {
public:
    COMPONENT(PDBController);

    PDBController(DeviceState* device);

    /// Install the controller
    bool Install();

    /// Uninstall the controller
    void Uninstall();

    /// Overrides
    void Handle(const MessageStream *streams, uint32_t count) final;

    /// Get the candidates for a given path
    /// \param view path
    /// \return candidate list, may be empty
    const PDBCandidateList& GetCandidateList(const std::string_view& view);

protected:
    /// Message handlers
    void OnMessage(const struct SetPDBConfigMessage& message);
    void OnMessage(const struct SetPDBPathMessage& message);
    void OnMessage(const struct IndexPDPathsMessage& message);

private:
    /// Index a given path
    /// \param path path to index
    void IndexPath(const std::filesystem::path& path);

private:
    DeviceState* device;

    /// Owning bridge, stored as naked pointer for referencing reasons
    IBridge* bridge{nullptr};

    /// All indexed paths
    Vector<std::string> pdbPaths;

    /// Recursive indexing?
    bool recursive{false};

    /// All indexed paths
    std::unordered_map<uint64_t, PDBCandidateList> indexed;

    /// Shared lock
    std::mutex mutex;
};
