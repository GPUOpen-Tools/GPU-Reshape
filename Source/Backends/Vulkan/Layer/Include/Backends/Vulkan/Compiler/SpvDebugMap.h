#pragma once

// Layer
#include <Backends/Vulkan/Compiler/Spv.h>

// Std
#include <vector>
#include <string_view>

struct SpvDebugMap {
    /// Set the id bound
    /// \param id
    void SetBound(SpvId id) {
        entries.resize(id);
    }

    /// Add a new debug entry
    /// \param id spv identifier
    /// \param op original opcode
    /// \param str assigned string
    void Add(SpvId id, SpvOp op, const std::string_view& str) {
        Entry& entry = entries.at(id);
        ASSERT(entry.op == SpvOpNop, "Double debug additional");

        entry.op = op;
        entry.value = str;
    }

    /// Get a debug string and verify the original opcode
    /// \param id spv identifier
    /// \param op insertion opcode
    /// \return the debug string
    const std::string_view& Get(SpvId id, SpvOp op) const {
        const Entry& entry = entries.at(id);
        ASSERT(entry.op == op, "Unexpected op code");
        return entry.value;
    }

    /// Get a debug string
    /// \param id spv identifier
    /// \return the debug string
    const std::string_view& GetValue(SpvId id) const {
        return entries.at(id).value;
    }

    /// Get the opcode for a given debug identifier
    /// \param id spv identifier
    /// \return opcode, Nop if not inserted
    SpvOp GetOpCode(SpvId id) const {
        return entries.at(id).op;
    }

private:
    struct Entry {
        SpvOp op{SpvOpNop};
        std::string_view value;
    };

    /// All entries
    std::vector<Entry> entries;
};
