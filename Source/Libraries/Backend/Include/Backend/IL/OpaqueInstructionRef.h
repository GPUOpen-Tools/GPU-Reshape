#pragma once

#include <cstdint>

namespace IL {
    struct BasicBlock;
    struct RelocationOffset;

    /// Immutable opaque instruction reference
    struct ConstOpaqueInstructionRef {
        /// Valid?
        bool IsValid() const {
            return relocationOffset != nullptr;
        }

        /// Valid?
        operator bool() const {
            return IsValid();
        }

        /// Parent basic block
        const BasicBlock* basicBlock{nullptr};

        /// The relocation offset within the basic block
        const RelocationOffset* relocationOffset{nullptr};
    };

    /// Mutable opaque instruction reference
    struct OpaqueInstructionRef {
        /// Valid?
        bool IsValid() const {
            return relocationOffset != nullptr;
        }

        /// Valid?
        operator bool() const {
            return IsValid();
        }

        /// Downcast to an immutable reference
        operator ConstOpaqueInstructionRef() const {
            ConstOpaqueInstructionRef ref;
            ref.basicBlock = basicBlock;
            ref.relocationOffset = relocationOffset;
            return ref;
        }

        /// Parent basic block
        BasicBlock* basicBlock{nullptr};

        /// The relocation offset within the basic block
        RelocationOffset* relocationOffset{nullptr};
    };
}
