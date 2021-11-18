#pragma once

// Std
#include <vector>

// Backend
#include "RelocationAllocator.h"
#include "Instruction.h"
#include "IdentifierMap.h"

namespace IL {
    /// Instruction reference
    /// \tparam T instruction type
    /// \tparam OPAQUE opaque reference type
    template<typename T, typename OPAQUE>
    struct TInstructionRef : public OPAQUE {
        TInstructionRef() = default;

        /// Constructor
        TInstructionRef(const OPAQUE &ref) : OPAQUE(ref) {

        }

        /// Get the instruction
        const T *Get() const;

        /// Accessor
        const T *operator->() const {
            return Get();
        }

        /// Reinterpret the instruction, asserts on validity
        /// \tparam U instruction type
        /// \return casted instruction type
        template<typename U>
        TInstructionRef<U, OPAQUE> As() const {
            ASSERT(Is<U>(), "Bad instruction cast");
            return *this;
        }

        /// Cast the instruction
        /// \tparam U instruction type
        /// \return invalid if failed cast
        template<typename U>
        TInstructionRef<U, OPAQUE> Cast() const {
            return {Is<U>() ? static_cast<OPAQUE>(*this) : OPAQUE{}};
        }

        /// Check if this instruction is of type
        template<typename U>
        bool Is() const {
            return U::kOpCode == Get()->opCode;
        }

        /// To implicit ID
        operator ID() const {
            return Get()->result;
        }
    };

    /// Mutable instruction reference
    template<typename T = Instruction>
    using InstructionRef = TInstructionRef<T, OpaqueInstructionRef>;

    /// Immutable instruction reference
    template<typename T = Instruction>
    using ConstInstructionRef = TInstructionRef<T, ConstOpaqueInstructionRef>;

    /// Basic block, holds a list of instructions
    ///   Instructions are laid out linearly in memory, however still allow for instruction references
    ///   while the block is being modified.
    struct BasicBlock {
        /// Mutable iterator
        struct Iterator {
            /// Comparison
            bool operator!=(const Iterator &other) const {
                Validate();

                return ptr != other.ptr;
            }

            /// Get the instruction
            const Instruction *Get() const {
                Validate();

                return reinterpret_cast<const Instruction *>(ptr);
            }

            /// Dereference
            const Instruction *operator*() const {
                return Get();
            }

            /// Accessor
            const Instruction *operator->() const {
                return Get();
            }

            /// Implicit to instruction
            operator const Instruction *() const {
                return Get();
            }

            /// Get an instruction reference
            template<typename T>
            InstructionRef<T> Ref() const {
                // Validation
                if constexpr(!std::is_same_v<T, Instruction>) {
                    ASSERT(Get()->opCode == T::kOpCode, "Invalid cast");
                }

                InstructionRef<T> ref;
                ref.basicBlock = block;
                ref.relocationOffset = block->GetRelocationOffset(relocationIndex);
                return ref;
            }

            /// Get an opaque instruction reference
            OpaqueInstructionRef Ref() const {
                OpaqueInstructionRef ref;
                ref.basicBlock = block;
                ref.relocationOffset = block->GetRelocationOffset(relocationIndex);
                return ref;
            }

            /// Get the op code
            OpCode GetOpCode() const {
                return Get()->opCode;
            }

            /// Post increment
            Iterator operator++(int) {
                Iterator self = *this;
                ++(*this);
                return self;
            }

            /// Pre increment
            Iterator &operator++() {
                ptr = ptr + GetSize(GetOpCode());
                relocationIndex++;
                return *this;
            }

            /// Validate the iterator
            void Validate() const {
#ifndef NDEBUG
                ASSERT(debugRevision == block->GetDebugRevision(), "Basic block modified during iteration");
#endif
            }

            /// Current offset
            const uint8_t *ptr;

            /// Relocation index for references
            uint32_t relocationIndex{0};

            /// Parent block
            BasicBlock *block{nullptr};

            /// Debug revision for iterate after modified validation
#ifndef NDEBUG
            uint32_t debugRevision;
#endif
        };

        /// Immutable iterator
        struct ConstIterator {
            /// Comparison
            bool operator!=(const ConstIterator &other) const {
                Validate();

                return ptr != other.ptr;
            }

            /// Get the instruction
            const Instruction *Get() const {
                Validate();

                return reinterpret_cast<const Instruction *>(ptr);
            }

            /// Dereference
            const Instruction *operator*() const {
                return Get();
            }

            /// Accessor
            const Instruction *operator->() const {
                return Get();
            }

            /// Implicit instruction
            operator const Instruction *() const {
                return Get();
            }

            /// Get a const instruction reference
            template<typename T>
            ConstInstructionRef<T> Ref() const {
                // Validation
                if constexpr(!std::is_same_v<T, Instruction>) {
                    ASSERT(Get()->opCode == T::kOpCode, "Invalid cast");
                }

                ConstInstructionRef<T> ref;
                ref.basicBlock = block;
                ref.relocationOffset = block->GetRelocationOffset(relocationIndex);
                return ref;
            }

            /// Get a const opaque instruction reference
            ConstOpaqueInstructionRef Ref() const {
                ConstOpaqueInstructionRef ref;
                ref.basicBlock = block;
                ref.relocationOffset = block->GetRelocationOffset(relocationIndex);
                return ref;
            }

            /// Get the op code
            OpCode GetOpCode() const {
                return Get()->opCode;
            }

            /// Post increment
            ConstIterator operator++(int) {
                ConstIterator self = *this;
                ++(*this);
                return self;
            }

            /// Pre increment
            ConstIterator &operator++() {
                ptr = ptr + GetSize(GetOpCode());
                relocationIndex++;
                return *this;
            }

            /// Validate this iterator
            void Validate() const {
#ifndef NDEBUG
                ASSERT(debugRevision == block->GetDebugRevision(), "Basic block modified during iteration");
#endif
            }

            /// Current offset
            const uint8_t *ptr;

            /// Current relocation index for references
            uint32_t relocationIndex{0};

            /// Parent block
            const BasicBlock *block{nullptr};

            /// Debug revision for iterate after modified validation
#ifndef NDEBUG
            uint32_t debugRevision;
#endif
        };

        /// Constructor
        BasicBlock(const Allocators &allocators, IdentifierMap &map, ID id) : relocationAllocator(allocators), allocators(allocators), map(map), id(id) {

        }

        /// Allow move
        BasicBlock(BasicBlock&& other) = default;

        /// No copy
        BasicBlock(const BasicBlock& other) = delete;

        /// No copy assignment
        BasicBlock& operator=(const BasicBlock& other) = delete;
        BasicBlock& operator=(BasicBlock&& other) = delete;

        /// Copy this basic block
        /// \param map the new identifier map
        /// \return the new basic block
        BasicBlock Copy(IdentifierMap& copyMap) const {
            BasicBlock bb(allocators, copyMap, id);
            bb.count = count;
            bb.dirty = dirty;
            bb.data = data;

            // Preallocate
            bb.relocationTable.resize(relocationTable.size());

            // Copy the relocation offsets
            for (size_t i = 0; i < relocationTable.size(); i++) {
                bb.relocationTable[i] = bb.relocationAllocator.Allocate();
                bb.relocationTable[i]->offset = relocationTable[i]->offset;
            }

            // Reindex the map
            for (auto it = bb.begin(); it != bb.end(); it++) {
                auto result = it->result;

                // Anything to index?
                if (result != InvalidID) {
                    copyMap.AddInstruction(it.Ref(), result);
                }
            }

            return bb;
        }

        /// Insert an instruction
        /// \param instruction the instruction to be inserted
        /// \return inserted reference
        template<typename T>
        InstructionRef<T> Insert(const T &instruction) {
            MarkAsDirty();

            InstructionRef<T> ref;
            ref.basicBlock = this;
            ref.relocationOffset = relocationAllocator.Allocate();

            size_t offset = data.size();
            data.resize(data.size() + sizeof(T));
            std::memcpy(&data[offset], &instruction, sizeof(T));

            ref.relocationOffset->offset = offset;

            relocationTable.push_back(ref.relocationOffset);

            if (instruction.result != InvalidID) {
                map.AddInstruction(ref, instruction.result);
            }

            count++;

#ifndef NDEBUG
            debugRevision++;
#endif

            return ref;
        }

        /// Insert an instruction at a given point
        /// \param insertion the insertion point
        /// \param instr the instruction to be inserted
        /// \return the inserted reference
        template<typename T>
        InstructionRef<T> Insert(const ConstOpaqueInstructionRef &insertion, const T &instr) {
            ASSERT(insertion.basicBlock == this, "Instruction offset not from the same basic block");

            MarkAsDirty();

            InstructionRef<T> ref;
            ref.basicBlock = this;
            ref.relocationOffset = relocationAllocator.Allocate();

            size_t size = GetSize(GetRelocationInstruction(insertion.relocationOffset)->opCode);

            size_t offset = insertion.relocationOffset->offset + size;

            auto *newPtr = reinterpret_cast<const uint8_t *>(&instr);
            data.insert(data.begin() + offset, newPtr, newPtr + sizeof(T));

            ref.relocationOffset->offset = offset;

            relocationTable.push_back(ref.relocationOffset);

            if (instr.result != InvalidID) {
                map.AddInstruction(ref, instr.result);
            }

            ResummarizeRelocationTable();

#ifndef NDEBUG
            debugRevision++;
#endif

            return ref;
        }

        /// Remove an instruction
        /// \param instruction the instruction reference
        void Remove(const OpaqueInstructionRef &instruction) {
            ASSERT(instruction.basicBlock == this, "Instruction offset not from the same basic block");

            MarkAsDirty();

            const Instruction *ptr = GetRelocationInstruction(instruction.relocationOffset);

            if (ptr->result != InvalidID) {
                map.RemoveInstruction(ptr->result);
            }

            size_t offset = instruction.relocationOffset->offset;
            data.erase(data.begin() + offset, data.begin() + offset + GetSize(ptr->opCode));

            // Remove relocation offset
            RemoveRelocationOffset(instruction.relocationOffset);

            ResummarizeRelocationTable();

#ifndef NDEBUG
            debugRevision++;
#endif
        }

        /// Replace an instruction with another, size may differ
        /// \param instruction the instruction reference to replace
        /// \param replacement the replacement
        /// \return the replaced reference
        template<typename T>
        InstructionRef<T> Replace(const OpaqueInstructionRef &instruction, const T &replacement) {
            ASSERT(instruction.basicBlock == this, "Instruction offset not from the same basic block");

            MarkAsDirty();

            const Instruction *ptr = GetRelocationInstruction(instruction.relocationOffset);

            if (ptr->result != InvalidID) {
                map.RemoveInstruction(ptr->result);
            }

            size_t size = GetSize(ptr->opCode);
            size_t offset = instruction.relocationOffset->offset;

            // Compare size
            if (size > sizeof(T)) {
                // Current instruction is too large, remove unused space
                data.erase(data.begin() + offset + sizeof(T), data.begin() + offset + size);
            } else if (size < sizeof(T)) {
                // Current instruction is too small, add new space
                data.insert(data.begin() + offset + size, sizeof(T) - size, 0x0);
            }

            // Replace the instruction data
            std::memcpy(data.data() + offset, &replacement, sizeof(T));

            if (replacement.result != InvalidID) {
                map.AddInstruction(instruction, replacement.result);
            }

            ResummarizeRelocationTable();

#ifndef NDEBUG
            debugRevision++;
#endif

            return instruction;
        }

        /// Resummarize the relocation table for references
        /// TODO: Resummarize AFTER the inserted instruction, bit of a waste otherwise
        void ResummarizeRelocationTable() {
            for (auto it = begin(); it != end(); it++) {
                relocationTable[it.relocationIndex]->offset = it.ptr - data.data();
            }
        }

        /// Mark this basic block as dirty
        void MarkAsDirty() {
            dirty = true;
        }

        /// Check if this basic block has been modified
        bool IsModified() const {
            return dirty;
        }

        /// Immortalize this basic block
        void Immortalize() {
            dirty = false;
        }

        /// Get the number of instructions
        uint32_t GetCount() const {
            return count;
        }

        /// Get the id of this basic block
        ID GetID() const {
            return id;
        }

        /// Iterator
        Iterator begin() {
            Iterator it;
            it.block = this;
            it.ptr = data.data();
#ifndef NDEBUG
            it.debugRevision = debugRevision;
#endif
            return it;
        }

        /// Iterator
        ConstIterator begin() const {
            ConstIterator it;
            it.block = this;
            it.ptr = data.data();
#ifndef NDEBUG
            it.debugRevision = debugRevision;
#endif
            return it;
        }

        /// Iterator
        Iterator end() {
            Iterator it;
            it.block = this;
            it.ptr = data.data() + data.size();
#ifndef NDEBUG
            it.debugRevision = debugRevision;
#endif
            return it;
        }

        /// Iterator
        ConstIterator end() const {
            ConstIterator it;
            it.block = this;
            it.ptr = data.data() + data.size();
#ifndef NDEBUG
            it.debugRevision = debugRevision;
#endif
            return it;
        }

        /// Get a relocation offset from an index
        /// \param index the linear index
        /// \return the offset
        RelocationOffset *GetRelocationOffset(uint32_t index) const {
            return relocationTable.at(index);
        }

        /// Get a relocation instruction
        /// \param relocationOffset the offset, must be from this basic block
        /// \return the instruction pointer
        template<typename T = Instruction>
        const T *GetRelocationInstruction(const RelocationOffset *relocationOffset) const {
            auto* instruction = reinterpret_cast<const Instruction *>(data.data() + relocationOffset->offset);

            // Validation
            if constexpr(!std::is_same_v<T, Instruction>) {
                ASSERT(instruction->opCode == T::kOpCode, "Invalid cast");
            }

            return static_cast<const T*>(instruction);
        }

        /// Get the current debug revision
#ifndef NDEBUG
        uint32_t GetDebugRevision() const {
            return debugRevision;
        }
#endif

    private:
        /// Remove a relocation offset
        void RemoveRelocationOffset(RelocationOffset *offset) {
            auto it = std::find(relocationTable.begin(), relocationTable.end(), offset);
            ASSERT(it != relocationTable.end(), "Dangling relocation offset");
            relocationTable.erase(it);
        }

    private:
        Allocators allocators;

        /// Label id
        ID id;

        /// Number of instructions
        uint32_t count{0};

        /// The shared identifier map
        IdentifierMap &map;

        /// Instruction stream
        std::vector<uint8_t> data;

        /// The current relocation table for resummarization
        std::vector<RelocationOffset *> relocationTable;

        /// Relocation block allocator
        RelocationAllocator relocationAllocator;

        /// Dirty flag
        bool dirty{true};

        /// Debug revision for iteration validation
#ifndef NDEBUG
        uint32_t debugRevision{0};
#endif
    };

    /// Getter implementation
    template<typename T, typename OPAQUE>
    inline const T *TInstructionRef<T, OPAQUE>::Get() const {
        return this->basicBlock->template GetRelocationInstruction<T>(this->relocationOffset);
    }
}
