// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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
#include <Common/FileSystem.h>

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
                case IndexPDPathsMessage::kID: {
                    OnMessage(*it.Get<IndexPDPathsMessage>());
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

void PDBController::IndexPathCandidates(const std::string_view& base, const std::filesystem::path &path) {
    // May create a copy
    std::string filename = path.string();
    
    // Local view
    std::string_view view = filename;

    // Remove up to the base directory
    view = view.substr(base.length());
    IndexPath(view, path);

    // Strip directory information
    if (auto delim = view.find_last_of("\\/"); delim != std::string::npos) {
        view = view.substr(delim + 1u);
        IndexPath(view, path);
    }

    // Strip extension information
    if (auto ext = view.find_last_of("."); ext != std::string::npos) {
        view = view.substr(0, ext);
        IndexPath(view, path);
    }
}

void PDBController::IndexPath(const std::string_view& view, const std::filesystem::path &path) {
    // Insert to crc64 of the filename
    uint64_t crc64 = BufferCRC32Short(view.data(), view.size() * sizeof(char));
    indexed[crc64].push_back(path.string());
}

void PDBController::AppendCandidates(const std::string_view &path, PDBCandidateList &candidates) {
    // Check crc64 of the filename
    uint64_t crc64 = BufferCRC32Short(path.data(), path.size() * sizeof(char));
    for (const std::string& candidate : indexed[crc64]) {
        candidates.Add(candidate);
    }
}

void PDBController::GetCandidateList(const char* path, PDBCandidateList& candidates) {
    /** TODO: Is there some universally accepted way to index debug files? */
    
    // The path may exist relative to the executable, or be an absolute path (f.x. local iteration)
    if (PathExists(path)) {
        candidates.Add(path);
    }

    // Serial
    std::lock_guard guard(mutex);

    // Local view
    std::string_view view = path;

    // Relative to pdb root candidates
    AppendCandidates(view, candidates);
    
    // Strip directory information
    if (auto delim = view.find_last_of("\\/"); delim != std::string::npos) {
        view = view.substr(delim + 1u);
        AppendCandidates(view, candidates);
    }

    // Strip extension information
    if (auto ext = view.find_last_of("."); ext != std::string::npos) {
        view = view.substr(0, ext);
        AppendCandidates(view, candidates);
    }
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

                IndexPathCandidates(path, entry.path());
            }
        } else {
            for (auto &&entry: std::filesystem::directory_iterator(path)) {
                if (entry.is_directory()) {
                    continue;
                }

                IndexPathCandidates(path, entry.path());
            }
        }
    }
}
