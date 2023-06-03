#include <Backends/DX12/Controllers/PDBController.h>
#include <Backends/DX12/States/ResourceState.h>
#include <Backends/DX12/States/DeviceState.h>

// Bridge
#include <Bridge/IBridge.h>

// Message
#include <Message/IMessageStorage.h>

// Schemas
#include <Schemas/PDB.h>

// Common
#include <Common/CRC.h>

// Std
#include <filesystem>

PDBController::PDBController(DeviceState *device) : device(device), pdbPaths(device->allocators) {

}

bool PDBController::Install() {
    // Install bridge
    bridge = registry->Get<IBridge>().GetUnsafe();
    if (!bridge) {
        return false;
    }

    // Install this listener
    bridge->Register(this);

    // OK
    return true;
}

void PDBController::Uninstall() {
    // Uninstall this listener
    bridge->Deregister(this);
}

void PDBController::Handle(const MessageStream *streams, uint32_t count) {
    std::lock_guard guard(mutex);

    for (uint32_t i = 0; i < count; i++) {
        ConstMessageStreamView view(streams[i]);

        // Visit all ordered messages
        for (ConstMessageStreamView<>::ConstIterator it = view.GetIterator(); it; ++it) {
            switch (it.GetID()) {
                case SetPDBConfigMessage::kID: {
                    OnMessage(*it.Get<SetPDBConfigMessage>());
                    break;
                }
                case SetPDBPathMessage::kID: {
                    OnMessage(*it.Get<SetPDBPathMessage>());
                    break;
                }
            }
        }
    }
}

void PDBController::OnMessage(const struct SetPDBConfigMessage &message) {
    pdbPaths.clear();
    pdbPaths.resize(message.pathCount);
    recursive = message.recursive;
}

void PDBController::IndexPath(const std::filesystem::path &path) {
    std::string filenameView = path.filename().string();

    // Insert to crc64 of the filename
    uint64_t crc64 = BufferCRC64(filenameView.data(), filenameView.size() * sizeof(char));
    indexed[crc64].push_back(path.string());
}

const PDBCandidateList &PDBController::GetCandidateList(const std::string_view &view) {
    std::lock_guard guard(mutex);
    std::string_view path = view;

    // This is a secondary lookup, cut all directory information
    if (auto delim = path.find_last_of("\\/"); delim != std::string::npos) {
        path = path.substr(delim + 1u);
    }

    // Get candidates for hash
    uint64_t crc64 = BufferCRC64(path.data(), path.size() * sizeof(char));
    return indexed[crc64];
}

void PDBController::OnMessage(const struct SetPDBPathMessage &message) {
    pdbPaths[message.index] = std::string(message.path.View());
}

void PDBController::OnMessage(const struct IndexPDPathsMessage &message) {
    indexed.clear();

    // Visit all paths
    for (const std::string &path: pdbPaths) {
        // Recursive?
        if (recursive) {
            for (auto &&entry: std::filesystem::recursive_directory_iterator(path)) {
                if (entry.is_directory()) {
                    continue;
                }

                IndexPath(entry.path());
            }
        } else {
            for (auto &&entry: std::filesystem::directory_iterator(path)) {
                if (entry.is_directory()) {
                    continue;
                }

                IndexPath(entry.path());
            }
        }
    }
}
