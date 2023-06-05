#pragma once

// Layer
#include <Backends/DX12/Controllers/IController.h>

// Bridge
#include <Bridge/IBridgeListener.h>

// Common
#include <Common/ComRef.h>
#include <Common/Allocator/Vector.h>
#include <Common/Containers/TrivialStackVector.h>

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
using PDBCandidateList = TrivialStackVector<std::string_view, 32u>;

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
    /// \param candidates candidate list, may be empty
    void GetCandidateList(const char* path, PDBCandidateList& candidates);

protected:
    /// Message handlers
    void OnMessage(const struct SetPDBConfigMessage& message);
    void OnMessage(const struct SetPDBPathMessage& message);
    void OnMessage(const struct IndexPDPathsMessage& message);

private:
    /// Index a given path and its candidates
    /// \param base pdb root
    /// \param path candidate path
    void IndexPathCandidates(const std::string_view& base, const std::filesystem::path& path);
    
    /// Index a given path
    /// \param base pdb root
    /// \param path candidate path
    void IndexPath(const std::string_view& base, const std::filesystem::path& path);

    /// Append a given set of candidates
    /// \param view search path
    /// \param candidates all candidates
    void AppendCandidates(const std::string_view& view, PDBCandidateList& candidates);

private:
    DeviceState* device;

    /// Owning bridge, stored as naked pointer for referencing reasons
    IBridge* bridge{nullptr};

    /// All indexed paths
    Vector<std::string> pdbPaths;

    /// Recursive indexing?
    bool recursive{false};

    /// All indexed paths
    std::unordered_map<uint64_t, std::vector<std::string>> indexed;

    /// Shared lock
    std::mutex mutex;
};
