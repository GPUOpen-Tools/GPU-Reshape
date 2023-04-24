#pragma once

// Layer
#include "LLVM/LLVMRecord.h"
#include "LLVM/LLVMBitStreamReader.h"
#include "LLVM/LLVMBitStreamWriter.h"
#include "DXILIDMap.h"
#include "DXILIDRemapRule.h"
#include "DXILIDUserType.h"

// Backend
#include <Backend/IL/ID.h>
#include <Backend/IL/Source.h>

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
        return static_cast<IL::ID>(id & ~(1ull << 32u));
    }

    /// Decode a forward encoded value
    static uint32_t DecodeForward(uint32_t id) {
        return static_cast<uint32_t>(-static_cast<int32_t>(id));
    }
 
    DXILIDRemapper(const Allocators& allocators, DXILIDMap &idMap)
        : allocators(allocators.Tag(kAllocModuleDXILIDRemapper)),
          unresolvedReferences(allocators.Tag(kAllocModuleDXILIDRemapper)),
          unresolvedForwardReferences(allocators.Tag(kAllocModuleDXILIDRemapper)),
          compileSegment(allocators.Tag(kAllocModuleDXILIDRemapper)),
          stitchSegment(allocators.Tag(kAllocModuleDXILIDRemapper)),
          idMap(idMap) {

    }

    /// Partial source to instrumented copy
    /// \param out destination map
    void CopyTo(DXILIDRemapper& out) {
        out.compileSegment.userMappings = compileSegment.userMappings;
    }

    /// Set the remap bound
    /// \param source source program bound
    /// \param user user modified program bound
    void SetBound(uint32_t source, uint32_t user) {
        stitchSegment.sourceMappings.resize(source, ~0u);
        compileSegment.userMappings.resize(user, UserMapping {.index = ~0u, .type = DXILIDUserType::Singular});
    }

    /// Allocate a source record value
    uint32_t AllocSourceMapping(uint32_t sourceResult) {
        uint32_t valueId = stitchSegment.allocationIndex++;
        stitchSegment.sourceMappings.at(sourceResult) = valueId;
        return valueId;
    }

    /// Set a source record value
    /// \param sourceResult original source index
    /// \param valueId stitched value index
    void SetSourceMapping(uint32_t sourceResult, uint32_t valueId) {
        stitchSegment.sourceMappings.at(sourceResult) = valueId;
    }

    /// Allocate a user mapping
    /// \param id the IL id
    uint32_t AllocUserMapping(IL::ID id) {
        if (compileSegment.userMappings.size() <= id) {
            compileSegment.userMappings.resize(id + 1);
        }

        uint32_t valueId = stitchSegment.allocationIndex++;
        compileSegment.userMappings.at(id).index = valueId;
        return valueId;
    }

    /// Allocate a source user mapping, copied over to instrumented map during partial copies
    /// \param id destination id
    /// \param type user type
    /// \param index index mapping
    void AllocSourceUserMapping(IL::ID id, DXILIDUserType type, uint32_t index) {
        if (compileSegment.userMappings.size() <= id) {
            compileSegment.userMappings.resize(id + 1);
        }

        UserMapping& mapping = compileSegment.userMappings.at(id);
        mapping.type = type;
        mapping.index = index;
    }

    /// Set a user mapping
    /// \param user source user operand
    /// \param source destination LLVM value index
    void SetUserMapping(IL::ID user, uint32_t source) {
        if (compileSegment.userMappings.size() <= user) {
            compileSegment.userMappings.resize(user + 1);
        }

        compileSegment.userMappings.at(user).index = source;
    }

    /// Allocate a user or source record mapping
    /// \param record given record
    void AllocRecordMapping(const LLVMRecord &record) {
        if (record.sourceAnchor == ~0u) {
            // Must have a user record
            ASSERT(record.userRecord, "Record without source mapping must be user generated");

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

    /// Remove the remapping rule from an operand
    /// \param value source operand
    /// \param rule given rule
    /// \return operand
    uint64_t RemoveRemapRule(uint64_t value, DXILIDRemapRule rule) {
        switch (rule) {
            default:
                ASSERT(false, "Invalid remap rule");
                return ~0ull;
            case DXILIDRemapRule::None:
                return value;
            case DXILIDRemapRule::Nullable:
                ASSERT(value > 0, "Nullable remap with zero value");
                return value - 1;
        }
    }

    /// Apply the remapping rule to an operand
    /// \param value source operand
    /// \param rule given rule
    /// \return operand
    uint64_t ApplyRemapRule(uint64_t value, DXILIDRemapRule rule) {
        switch (rule) {
            default:
                ASSERT(false, "Invalid remap rule");
                return ~0ull;
            case DXILIDRemapRule::None:
                return value;
            case DXILIDRemapRule::Nullable:
                ASSERT(value > 0, "Nullable remap with zero value");
                return value + 1;
        }
    }

    /// Remap a DXIL value
    /// \param source source DXIL value
    void Remap(uint64_t &source, DXILIDRemapRule rule = DXILIDRemapRule::None) {
        // Original source mappings are allocated at a given range
        if (IsSourceOperand(source)) {
            uint64_t unmapped = RemoveRemapRule(source, rule);

            uint32_t mapping = stitchSegment.sourceMappings.at(unmapped);
            if (mapping == ~0u) {
                // Mapping doesn't exist yet, add as unresolved
                unresolvedReferences.Add(UnresolvedReferenceEntry{
                    .source = &source,
                    .absolute = unmapped,
                    .rule = rule
                });

                // Sanity
#ifndef NDEBUG
                source = ~0u;
#endif // NDEBUG

                return;
            }

            // Assign source to new mapping
            source = ApplyRemapRule(mapping, rule);
        } else {
            uint32_t mapping = compileSegment.userMappings.at(DecodeUserOperand(source)).index;
            if (mapping == ~0u) {
                // Mapping doesn't exist yet, add as unresolved
                unresolvedReferences.Add(UnresolvedReferenceEntry{
                    .source = &source,
                    .absolute = source,
                    .rule = rule
                });

                // Sanity
#ifndef NDEBUG
                source = ~0u;
#endif // NDEBUG

                return;
            }

            // Assign source to new mapping
            source = ApplyRemapRule(mapping, rule);
        }
    }

    /// Check if a given value is resolved
    /// \param record source record
    /// \param id relative id
    /// \return true if resolved
    bool IsResolved(const LLVMRecord &record, uint32_t id) {
        if (IsSourceOperand(id)) {
            return id <= record.sourceAnchor;
        } else {
            ASSERT(false, "Non-source resolve states not implemented");
            return false;
        }
    }
    
    ///  Remap a DXIL value
    /// \param anchor source anchor
    /// \param record source record
    /// \param source source DXIL value
    /// \return
    bool RemapRelative(const Anchor &anchor, const LLVMRecord &record, uint64_t &source) {
        uint32_t absoluteRemap;

        // Original source mappings are allocated at a given range
        if (IsSourceOperand(source)) {
            ASSERT(record.sourceAnchor != ~0u, "Source operand on a user record");

            // Forward?
            uint32_t mapping;
            if (source <= record.sourceAnchor) {
                mapping = stitchSegment.sourceMappings.at(record.sourceAnchor - source);
            } else {
                mapping = stitchSegment.sourceMappings.at(record.sourceAnchor + DXILIDRemapper::DecodeForward(static_cast<uint32_t>(source)));
            }

            // If failed, this may be a replaced identifier, try user space
            if (mapping == ~0u) {
                // Get mapped identifier
                IL::ID id = idMap.GetMapped(record.sourceAnchor - source);
                ASSERT(id != IL::InvalidOffset, "Remapped failed to potentially replaced identifier");

                // Get absolute mapping
                mapping = TryGetUserMapping(id);
                ASSERT(mapping != ~0u, "Remapped not found on user operand");
            }

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
        if (absoluteRemap == ~0u) {
            source = ~0u;
            return absoluteRemap > anchor.stitchAnchor;
        }
#endif // NDEBUG

        // Re-encode relative
        source = anchor.stitchAnchor - absoluteRemap;
        return absoluteRemap > anchor.stitchAnchor;
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
                absoluteRemap = record.sourceAnchor - static_cast<uint32_t>(ref);
            }

            // Must be within the source range
            ASSERT(absoluteRemap < stitchSegment.sourceMappings.size(), "Unmapped source operand beyond source range");

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
                uint32_t mapping = stitchSegment.sourceMappings.at(entry.absolute);
                ASSERT(mapping != ~0u, "Remapped not found on source operand");
                absoluteRemap = mapping;
            } else {
                uint32_t mapping = compileSegment.userMappings.at(DecodeUserOperand(entry.absolute)).index;
                ASSERT(mapping != ~0u, "Remapped not found on user operand");
                absoluteRemap = mapping;
            }

            // Re-encode relative
            *entry.source = ApplyRemapRule(absoluteRemap, entry.rule);
        }

        for (const UnresolvedForwardReferenceEntry &entry: unresolvedForwardReferences) {
            uint32_t absoluteRemap;

            // Original source mappings are allocated at a given range
            if (IsSourceOperand(entry.absolute)) {
                uint32_t mapping = stitchSegment.sourceMappings.at(entry.absolute);
                ASSERT(mapping != ~0u, "Remapped not found on source operand");
                absoluteRemap = mapping;
            } else {
                uint32_t mapping = compileSegment.userMappings.at(DecodeUserOperand(entry.absolute)).index;
                ASSERT(mapping != ~0u, "Remapped not found on user operand");
                absoluteRemap = mapping;
            }

#ifndef NDEBUG
            if (absoluteRemap == ~0u) {
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
        return source < stitchSegment.sourceMappings.size() ? stitchSegment.sourceMappings.at(source) : ~0u;
    }

    /// Try to remap a DXIL value
    /// \param source source DXIL value
    /// \return remapped value, UINT32_MAX if not found
    uint32_t TryGetUserMapping(uint32_t user) {
        return user < compileSegment.userMappings.size() ? compileSegment.userMappings.at(user).index : ~0u;
    }

    /// Get the user mapping
    /// \param user given user, must exist
    uint32_t GetUserMapping(uint32_t user) {
        return compileSegment.userMappings.at(user).index;
    }

    /// Set the user mapping type
    /// \param user given user, must exist
    /// \param type final type
    void SetUserMappingType(uint32_t user, DXILIDUserType type) {
        compileSegment.userMappings.at(user).type = type;
    }

    /// Get the user type
    /// \param user given user, must exist
    /// \return type
    DXILIDUserType GetUserMappingType(uint32_t user) {
        return compileSegment.userMappings.at(user).type;
    }

    /// Set a redirected value
    /// \param user the given user id
    /// \param operand the redirected operand
    void SetUserRedirect(IL::ID user, IL::ID redirect) {
        // Ensure space
        if (compileSegment.userRedirects.size() <= user) {
            compileSegment.userRedirects.resize(user + 1, ~0u);
        }

        // Unfold redirects
        while(redirect < compileSegment.userRedirects.size() && compileSegment.userRedirects.at(redirect) != ~0u) {
            redirect = static_cast<IL::ID>(compileSegment.userRedirects.at(redirect));
        }

        // Preserve user mappings
        if (redirect < compileSegment.userMappings.size()) {
            if (compileSegment.userMappings.size() <= user) {
                compileSegment.userMappings.resize(user + 1);
            }

            compileSegment.userMappings.at(user) = compileSegment.userMappings.at(redirect);
        }

        // Set final redirect
        compileSegment.userRedirects.at(user) = redirect;
    }

    /// Try to get a redirect
    /// \param id user id
    /// \return UINT32_MAX if invalid
    uint64_t TryGetUserRedirect(IL::ID id) {
        return id < compileSegment.userRedirects.size() ? compileSegment.userRedirects.at(id) : ~0u;
    }

    /// Encode a potentially redirected user operand
    /// \param id user id
    /// \return encoded operand
    uint64_t EncodeRedirectedUserOperand(IL::ID id) {
        // Try to get redirect
        uint64_t redirect = TryGetUserRedirect(id);
        if (redirect != ~0u) {
            return EncodeUserOperand(static_cast<IL::ID>(redirect));
        }

        // Encode regular user
        return EncodeUserOperand(id);
    }

    /// Get the current record anchor
    Anchor GetAnchor() const {
        Anchor anchor;
        anchor.stitchAnchor = stitchSegment.allocationIndex;
        return anchor;
    }

public:
    /// Single mapping
    struct UserMapping {
        /// Mapped index
        uint32_t index;

        /// Underlying type
        DXILIDUserType type;
    };

    /// Snapshot of the map, compilation data
    struct CompileSnapshot {
        /// Current offset in user mappings
        uint64_t userMappingOffset{0};

        /// Current offset in user redirects
        uint64_t userRedirectsOffset{0};
    };

    /// Snapshot of the map, stitch data
    struct StitchSnapshot {
        /// Value allocation index
        uint32_t allocationIndex{0};

        /// Current offset in source mappings
        uint64_t sourceMappingOffset{0};
    };

    /// Extracted segment of the map, compile data
    struct CompileSegment {
        CompileSegment(const Allocators& allocators) : userMappings(allocators), userRedirects(allocators) {
            
        }
        
        CompileSnapshot head;
        
        /// All present user mappings
        Vector<UserMapping> userMappings;

        /// All present user redirects
        Vector<uint64_t> userRedirects;
    };

    /// Extracted segment of the map, stitch data
    struct StitchSegment {
        StitchSegment(const Allocators& allocators) : sourceMappings(allocators) {
            
        }
        
        StitchSnapshot head;
        
        /// Current user allocation index
        uint32_t allocationIndex{0};

        /// All present source mappings
        Vector<uint32_t> sourceMappings;
    };

    /// Create a new compilation snapshot, signifies a point in time for id mapping
    /// \return snapshot
    CompileSnapshot CreateCompileSnapshot() {
        return CompileSnapshot {
            .userMappingOffset = compileSegment.userMappings.size(),
            .userRedirectsOffset = compileSegment.userRedirects.size()
        };
    }

    /// Create a new stitching snapshot, signifies a point in time for id mapping
    /// \return snapshot
    StitchSnapshot CreateStitchSnapshot() {
        return StitchSnapshot {
            .allocationIndex = stitchSegment.allocationIndex,
            .sourceMappingOffset = stitchSegment.sourceMappings.size()
        };
    }

    /// Branch from a given snapshot
    /// \param from snapshot to be branched from
    /// \return segment
    CompileSegment Branch(const CompileSnapshot& from) {
        CompileSegment remote(allocators);
        remote.head = from;

        // Validate
        ASSERT(compileSegment.userMappings.size() >= from.userMappingOffset, "Remote snapshot ahead of root");
        ASSERT(compileSegment.userRedirects.size() >= from.userRedirectsOffset, "Remote snapshot ahead of root");

        // Move user map
        remote.userMappings.resize(compileSegment.userMappings.size() - from.userMappingOffset);
        std::memcpy(remote.userMappings.data(), compileSegment.userMappings.data() + from.userMappingOffset, sizeof(UserMapping) * remote.userMappings.size());

        // Move redirect map
        remote.userRedirects.resize(compileSegment.userRedirects.size() - from.userRedirectsOffset);
        std::memcpy(remote.userRedirects.data(), compileSegment.userRedirects.data() + from.userRedirectsOffset, sizeof(uint64_t) * remote.userRedirects.size());

        // Set current state
        Revert(from);

        // OK!
        return remote;
    }

    /// Branch from a given snapshot
    /// \param from snapshot to be branched from
    /// \return segment
    StitchSegment Branch(const StitchSnapshot& from) {
        StitchSegment remote(allocators);
        remote.head = from;

        // Base index
        remote.allocationIndex = stitchSegment.allocationIndex;

        // Validate
        ASSERT(stitchSegment.sourceMappings.size() >= from.sourceMappingOffset, "Remote snapshot ahead of root");

        // Move source map
        remote.sourceMappings.resize(stitchSegment.sourceMappings.size() - from.sourceMappingOffset);
        std::memcpy(remote.sourceMappings.data(), stitchSegment.sourceMappings.data() + from.sourceMappingOffset, sizeof(uint32_t) * remote.sourceMappings.size());

        // Set current state
        Revert(from);

        // OK!
        return remote;
    }

    /// Revert to a snapshot
    /// \param from snapshot to be reverted to
    void Revert(const CompileSnapshot& from) {
        compileSegment.userMappings.resize(from.userMappingOffset);
        compileSegment.userRedirects.resize(from.userRedirectsOffset);
    }

    /// Revert to a snapshot
    /// \param from snapshot to be reverted to
    void Revert(const StitchSnapshot& from) {
        stitchSegment.allocationIndex = from.allocationIndex;
        stitchSegment.sourceMappings.resize(from.sourceMappingOffset);
    }

    /// Merge a branch
    /// \param remote branch to be merged from, heads must match
    void Merge(const CompileSegment& remote) {
        ASSERT(compileSegment.userMappings.size() == remote.head.userMappingOffset, "Invalid remote, length mismatch");
        ASSERT(compileSegment.userRedirects.size() == remote.head.userRedirectsOffset, "Invalid remote, length mismatch");

        // Move user
        compileSegment.userMappings.resize(remote.head.userMappingOffset + remote.userMappings.size());
        std::memcpy(compileSegment.userMappings.data() + remote.head.userMappingOffset, remote.userMappings.data(), sizeof(uint32_t) * remote.userMappings.size());

        // Move redirect
        compileSegment.userRedirects.resize(remote.head.userRedirectsOffset + remote.userRedirects.size());
        std::memcpy(compileSegment.userRedirects.data() + remote.head.userRedirectsOffset, remote.userRedirects.data(), sizeof(uint32_t) * remote.userRedirects.size());
    }

    /// Merge a branch
    /// \param remote branch to be merged from, heads must match
    void Merge(const StitchSegment& remote) {
        ASSERT(stitchSegment.allocationIndex == remote.head.allocationIndex, "Invalid remote, allocation offset mismatch");
        ASSERT(stitchSegment.sourceMappings.size() == remote.head.sourceMappingOffset, "Invalid remote, length mismatch");

        // Set new offset
        stitchSegment.allocationIndex = remote.allocationIndex;

        // Move source
        stitchSegment.sourceMappings.resize(remote.head.sourceMappingOffset + remote.sourceMappings.size());
        std::memcpy(stitchSegment.sourceMappings.data() + remote.head.sourceMappingOffset, remote.sourceMappings.data(), sizeof(uint32_t) * remote.sourceMappings.size());
    }

private:
    Allocators allocators;

private:
    struct UnresolvedReferenceEntry {
        uint64_t *source{nullptr};
        uint64_t absolute;
        DXILIDRemapRule rule;
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
    /// Root segments
    CompileSegment compileSegment;
    StitchSegment stitchSegment;

    /// Shared id map
    DXILIDMap& idMap;
};
