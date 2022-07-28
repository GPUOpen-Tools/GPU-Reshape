#pragma once

// Layer
#include "LLVM/LLVMRecord.h"
#include "LLVM/LLVMBitStreamReader.h"
#include "LLVM/LLVMBitStreamWriter.h"
#include "DXILIDMap.h"

// Backend
#include <Backend/IL/ID.h>

// Common
#include <Common/Containers/TrivialStackVector.h>

// Std
#include <vector>

struct DXILIDRemapper {
    struct Anchor {
        uint32_t stitchAnchor;
    };

    /// Encode an operand as user
    static uint64_t EncodeUserOperand(IL::ID id) {
        return static_cast<uint64_t>(id) | (1ull << 32u);
    }

    /// Check if an operand is user derived
    static bool IsUserOperand(uint64_t id) {
        return id & (1ull << 32u);
    }

    /// Check if an operand is source derived
    static bool IsSourceOperand(uint64_t id) {
        return !(id & (1ull << 32u));
    }

    /// Decode a user operand
    static IL::ID DecodeUserOperand(uint64_t id) {
        return id & ~(1ull << 32u);
    }

    DXILIDRemapper(DXILIDMap& idMap) : idMap(idMap) {

    }

    /// Set the remap bound
    /// \param source source program bound
    /// \param user user modified program bound
    void SetBound(uint32_t source, uint32_t user) {
        sourceMappings.resize(source, ~0u);
        userMappings.resize(user, ~0u);
    }

    /// Allocate a source record value
    uint32_t AllocSourceMapping(uint32_t sourceResult) {
        uint32_t valueId = allocationIndex++;
        sourceMappings.at(sourceResult) = valueId;
        return valueId;
    }

    /// Allocate a user mapping
    /// \param id the IL id
    uint32_t AllocUserMapping(IL::ID id) {
        if (userMappings.size() <= id) {
            userMappings.resize(id + 1);
        }

        uint32_t valueId = allocationIndex++;
        userMappings.at(id) = valueId;
        return valueId;
    }

    /// Set a user mapping
    /// \param user source user operand
    /// \param source destination LLVM value index
    void SetUserMapping(IL::ID user, uint64_t source) {
        if (userMappings.size() <= user) {
            userMappings.resize(user + 1);
        }

        userMappings.at(user) = source;
    }

    /// Allocate a user or source record mapping
    /// \param record given record
    void AllocRecordMapping(const LLVMRecord &record) {
        if (record.userRecord) {
            // Strictly a user record, no source references to this
            AllocUserMapping(record.result);
        } else {
            // Source record, create source wise mapping
            uint32_t valueId = AllocSourceMapping(record.sourceAnchor);

            // Create IL mapping, source records can be referenced by both other source records and user records
            if (idMap.IsMapped(record.sourceAnchor)) {
                SetUserMapping(idMap.GetMapped(record.sourceAnchor), valueId);
            }
        }
    }

    /// Remap a DXIL value
    /// \param source source DXIL value
    void Remap(uint64_t &source) {
        // Original source mappings are allocated at a given range
        if (IsSourceOperand(source)) {
            uint32_t mapping = sourceMappings.at(source);
            if (mapping == ~0u) {
                // Mapping doesn't exist yet, add as unresolved
                unresolvedReferences.Add(UnresolvedReferenceEntry{
                    .source = &source,
                    .absolute = source
                });

                // Sanity
#ifndef NDEBUG
                source = ~0u;
#endif // NDEBUG

                return;
            }

            // Assign source to new mapping
            source = mapping;
        } else {
            uint32_t mapping = userMappings.at(DecodeUserOperand(source));
            if (mapping == ~0u) {
                // Mapping doesn't exist yet, add as unresolved
                unresolvedReferences.Add(UnresolvedReferenceEntry{
                    .source = &source,
                    .absolute = source
                });

                // Sanity
#ifndef NDEBUG
                source = ~0u;
#endif // NDEBUG

                return;
            }

            // Assign source to new mapping
            source = mapping;
        }
    }

    /// Remap a DXIL value
    /// \param source source DXIL value
    void RemapRelative(const Anchor &anchor, const LLVMRecord &record, uint64_t &source) {
        uint32_t absoluteRemap;

        // Original source mappings are allocated at a given range
        if (IsSourceOperand(source)) {
            ASSERT(record.sourceAnchor != ~0u, "Source operand on a user record");

            uint32_t mapping = sourceMappings.at(record.sourceAnchor - source);
            ASSERT(mapping != ~0u, "Remapped not found on source operand");

            // Assign absolute
            absoluteRemap = mapping;
        } else {
            // Get absolute mapping
            uint32_t mapping = TryGetUserMapping(DecodeUserOperand(source));
            ASSERT(mapping != ~0u, "Remapped not found on user operand");

            // Assign absolute
            absoluteRemap = mapping;
        }

#ifndef NDEBUG
        if (absoluteRemap == ~0u)
        {
            source = ~0u;
            return;
        }
#endif // NDEBUG

        // Re-encode relative
        source = anchor.stitchAnchor - absoluteRemap;
    }

    /// Remap a DXIL value
    /// \param source source DXIL value
    void RemapUnresolvedReference(const Anchor &anchor, const LLVMRecord &record, uint64_t &source) {
        uint32_t absoluteRemap;

        // Original source mappings are allocated at a given range
        if (IsSourceOperand(source)) {
            int64_t ref = LLVMBitStreamReader::DecodeSigned(source);

            // Backwards or forwards?
            if (ref < 0) {
                absoluteRemap = record.sourceAnchor + static_cast<uint32_t>(-ref);
            } else {
                absoluteRemap = record.sourceAnchor - ref;
            }

            // Must be within the source range
            ASSERT(absoluteRemap < sourceMappings.size(), "Unmapped source operand beyond source range");

            // Add as unresolved
            unresolvedForwardReferences.Add(UnresolvedForwardReferenceEntry{
                .source = &source,
                .anchor = anchor.stitchAnchor,
                .absolute = absoluteRemap
            });

            // Sanity
#ifndef NDEBUG
            source = ~0u;
#endif // NDEBUG
        } else {
            // Add as unresolved
            unresolvedForwardReferences.Add(UnresolvedForwardReferenceEntry{
                .source = &source,
                .anchor = anchor.stitchAnchor,
                .absolute = source
            });

            // Sanity
#ifndef NDEBUG
            source = ~0u;
#endif // NDEBUG
        }
    }

    /// Resolve all unresolved references
    void ResolveForwardReferences() {
        for (const UnresolvedReferenceEntry &entry: unresolvedReferences) {
            uint32_t absoluteRemap;

            // Original source mappings are allocated at a given range
            if (IsSourceOperand(entry.absolute)) {
                uint32_t mapping = sourceMappings.at(entry.absolute);
                ASSERT(mapping != ~0u, "Remapped not found on source operand");
                absoluteRemap = mapping;
            } else {
                uint32_t mapping = userMappings.at(DecodeUserOperand(entry.absolute));
                ASSERT(mapping != ~0u, "Remapped not found on user operand");
                absoluteRemap = mapping;
            }

            // Re-encode relative
            *entry.source = absoluteRemap;
        }

        for (const UnresolvedForwardReferenceEntry &entry: unresolvedForwardReferences) {
            uint32_t absoluteRemap;

            // Original source mappings are allocated at a given range
            if (IsSourceOperand(entry.absolute)) {
                uint32_t mapping = sourceMappings.at(entry.absolute);
                ASSERT(mapping != ~0u, "Remapped not found on source operand");
                absoluteRemap = mapping;
            } else {
                uint32_t mapping = userMappings.at(DecodeUserOperand(entry.absolute));
                ASSERT(mapping != ~0u, "Remapped not found on user operand");
                absoluteRemap = mapping;
            }

#ifndef NDEBUG
            if (absoluteRemap == ~0u)
            {
                *entry.source = ~0u;
                continue;
            }
#endif // NDEBUG

            // Forward reference capable resolves are signed
            int64_t relative = static_cast<int64_t>(entry.anchor) - absoluteRemap;

            // Re-encode relative
            *entry.source = LLVMBitStreamWriter::EncodeSigned(relative);
        }
    }

    /// Try to remap a DXIL value
    /// \param source source DXIL value
    /// \return remapped value, UINT32_MAX if not found
    uint32_t TryGetSourceMapping(uint32_t source) {
        return source < sourceMappings.size() ? sourceMappings.at(source) : ~0u;
    }

    /// Try to remap a DXIL value
    /// \param source source DXIL value
    /// \return remapped value, UINT32_MAX if not found
    uint32_t TryGetUserMapping(uint32_t user) {
        return user < userMappings.size() ? userMappings.at(user) : ~0u;
    }

    /// Set a redirected value
    /// \param user the given user id
    /// \param operand the redirected operand
    void SetUserRedirect(IL::ID user, uint64_t operand) {
        if (userRedirects.size() <= user) {
            userRedirects.resize(user + 1, ~0u);
        }

        userRedirects.at(user) = operand;
    }

    /// Try to get a redirect
    /// \param id user id
    /// \return UINT32_MAX if invalid
    uint64_t TryGetUserRedirect(IL::ID id) {
        return id < userRedirects.size() ? userRedirects.at(id) : ~0u;
    }

    /// Encode a potentially redirected user operand
    /// \param id user id
    /// \return encoded operand
    uint64_t EncodeRedirectedUserOperand(IL::ID id) {
        // Try to get redirect
        uint64_t redirect = TryGetUserRedirect(id);
        if (redirect != ~0u) {
            return redirect;
        }

        // Encode regular user
        return EncodeUserOperand(id);
    }

    /// Get the current record anchor
    Anchor GetAnchor() const {
        Anchor anchor;
        anchor.stitchAnchor = allocationIndex;
        return anchor;
    }

private:
    struct UnresolvedReferenceEntry {
        uint64_t *source{nullptr};
        uint64_t absolute;
    };

    struct UnresolvedForwardReferenceEntry {
        uint64_t *source{nullptr};
        uint32_t anchor;
        uint64_t absolute;
    };

    /// All unresolved references
    TrivialStackVector<UnresolvedReferenceEntry, 64> unresolvedReferences;
    TrivialStackVector<UnresolvedForwardReferenceEntry, 64> unresolvedForwardReferences;

private:
    /// Current user allocation index
    uint32_t allocationIndex{0};

    /// All present source mappings
    std::vector<uint32_t> sourceMappings;

    /// All present user mappings
    std::vector<uint32_t> userMappings;

    /// All present user redirects
    std::vector<uint64_t> userRedirects;

    /// Shared id map
    DXILIDMap& idMap;
};
