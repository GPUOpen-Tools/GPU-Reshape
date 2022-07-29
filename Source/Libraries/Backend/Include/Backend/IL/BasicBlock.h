#pragma once

// Std
#include <vector>

// Backend
#include "RelocationAllocator.h"
#include "Instruction.h"
#include "IdentifierMap.h"
#include "BasicBlockFlags.h"

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
        T *GetMutable() const;

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
            using iterator_category = std::forward_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type        = const Instruction*;
            using pointer           = value_type*;
            using reference         = value_type&;

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

            /// Reinterpret instruction
            template<typename T>
            const T* As() const {
                ASSERT(Get()->opCode == T::kOpCode, "Invalid cast");
                return static_cast<const T*>(Get());
            }

            /// Get an instruction reference
            template<typename T>
            InstructionRef<T> Ref() const {
                Validate();

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
                Validate();

                OpaqueInstructionRef ref;
                ref.basicBlock = block;
                ref.relocationOffset = block->GetRelocationOffset(relocationIndex);
                return ref;
            }

            /// To opaque instruction ref
            operator OpaqueInstructionRef() const {
                return Ref();
            }

            /// To const opaque instruction ref
            operator ConstOpaqueInstructionRef() const {
                return Ref();
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
                ptr = ptr + GetSize(Get());
                relocationIndex++;
                return *this;
            }

            /// Validate the iterator
            void Validate() const {
#ifndef NDEBUG
                ASSERT(debugRevision == block->GetDebugRevision(), "Basic block modified during iteration");
#endif
            }

            /// Valid iterator?
            bool IsValid() const {
                return ptr != nullptr;
            }

            /// Current offset
            const uint8_t *ptr{nullptr};

            /// Relocation index for references
            uint32_t relocationIndex{0};

            /// Parent block
            BasicBlock *block{nullptr};

            /// Debug revision for iterate after modified validation
#ifndef NDEBUG
            uint32_t debugRevision{};
#endif
        };

        /// Typed iterator for sugar syntax
        template<typename T>
        struct TypedIterator : public Iterator {
            using iterator_category = std::forward_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type        = const T*;
            using pointer           = value_type*;
            using reference         = value_type&;

            TypedIterator(const Iterator& it) : Iterator(it) {
                ASSERT(it.GetOpCode() == T::kOpCode, "Invalid cast");
            }

            /// Ref cast
            operator InstructionRef<T>() const {
                return Ref<T>();
            }

            /// Dereference
            const T *operator*() const {
                return As<T>();
            }

            /// Accessor
            const T *operator->() const {
                return As<T>();
            }

            /// Implicit to instruction
            operator const T *() const {
                return As<T>();
            }
        };

        /// Immutable iterator
        struct ConstIterator {
            using iterator_category = std::forward_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type        = const Instruction*;
            using pointer           = value_type*;
            using reference         = value_type&;

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
                Validate();

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
                Validate();

                ConstOpaqueInstructionRef ref;
                ref.basicBlock = block;
                ref.relocationOffset = block->GetRelocationOffset(relocationIndex);
                return ref;
            }

            /// Reinterpret instruction
            template<typename T>
            const T* As() const {
                ASSERT(Get()->opCode == T::kOpCode, "Invalid cast");
                return static_cast<const T*>(Get());
            }

            /// To const opaque instruction ref
            operator ConstOpaqueInstructionRef() const {
                return Ref();
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
                ptr = ptr + GetSize(Get());
                relocationIndex++;
                return *this;
            }

            /// Validate this iterator
            void Validate() const {
#ifndef NDEBUG
                ASSERT(debugRevision == block->GetDebugRevision(), "Basic block modified during iteration");
#endif
            }

            /// Is this iterator valid?
            bool IsValid() const {
                return ptr != nullptr;
            }

            /// Current offset
            const uint8_t *ptr{nullptr};

            /// Current relocation index for references
            uint32_t relocationIndex{0};

            /// Parent block
            const BasicBlock *block{nullptr};

            /// Debug revision for iterate after modified validation
#ifndef NDEBUG
            uint32_t debugRevision{};
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
        void CopyTo(BasicBlock* out) const;

        /// Reindex all users
        void IndexUsers();

        /// Add a new flag to this block
        /// \param value flag to be added
        void AddFlag(BasicBlockFlagSet value) {
            flags |= value;
        }

        /// Remove a flag from this block
        /// \param value flag to be removed
        void RemoveFlag(BasicBlockFlagSet value) {
            flags &= ~value;
        }

        /// Check if this block has a flag
        /// \param value flag to be checked
        /// \return true if present
        bool HasFlag(BasicBlockFlag value) {
            return flags & value;
        }

        /// Append an instruction
        /// \param instruction the instruction to be inserted
        /// \return inserted reference
        Iterator Append(const Instruction* instruction, uint32_t size) {
            MarkAsDirty();

            size_t offset = data.size();
            data.resize(data.size() + size);
            std::memcpy(&data[offset], instruction, size);

            InstructionRef<> ref;
            ref.basicBlock = this;
            ref.relocationOffset = relocationAllocator.Allocate();
            ref.relocationOffset->offset = static_cast<uint32_t>(offset);

            relocationTable.push_back(ref.relocationOffset);

            AddInstructionReferences(instruction, ref);

            count++;

#ifndef NDEBUG
            debugRevision++;
#endif

            return Offset(ref.relocationOffset, static_cast<uint32_t>(relocationTable.size()) - 1);
        }

        /// Append an instruction
        /// \param instruction the instruction to be inserted
        /// \return inserted reference
        Iterator Append(const Instruction* instruction) {
            return Append(instruction, static_cast<uint32_t>(IL::GetSize(instruction)));
        }

        /// Append an instruction
        /// \param instruction the instruction to be inserted
        /// \return inserted reference
        template<typename T, typename = std::enable_if_t<!std::is_pointer_v<T>>>
        TypedIterator<T> Append(const T &instruction) {
            return Append(static_cast<const Instruction*>(&instruction), sizeof(T));
        }

        /// Append an instruction at a given point
        /// \param insertion the insertion point, inserted before this iterator
        /// \param instr the instruction to be inserted
        /// \return the inserted reference
        template<typename T>
        TypedIterator<T> Insert(const ConstOpaqueInstructionRef &insertion, const T &instr) {
            ASSERT(insertion.basicBlock == this, "Instruction offset not from the same basic block");

            MarkAsDirty();

            size_t offset = insertion.relocationOffset->offset;

            auto *newPtr = reinterpret_cast<const uint8_t *>(&instr);
            data.insert(data.begin() + offset, newPtr, newPtr + sizeof(T));

            InstructionRef<T> ref;
            ref.basicBlock = this;
            ref.relocationOffset = relocationAllocator.Allocate();
            ref.relocationOffset->offset = static_cast<uint32_t>(offset);

            if (instr.result != InvalidID) {
                map.AddInstruction(ref, instr.result);
            }

#ifndef NDEBUG
            debugRevision++;
#endif

            return InsertRelocationOffset(insertion.relocationOffset, ref.relocationOffset);
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
            data.erase(data.begin() + offset, data.begin() + offset + GetSize(ptr));

            uint64_t relocationIndex = std::find(relocationTable.begin(), relocationTable.end(), instruction.relocationOffset) - relocationTable.begin();

            // Remove relocation offset
            RemoveRelocationOffset(instruction.relocationOffset);

            ResummarizeRelocationTable(instruction.relocationOffset, relocationIndex);

#ifndef NDEBUG
            debugRevision++;
#endif
        }

        /// Replace an instruction with another, size may differ
        /// \param instruction the instruction reference to replace
        /// \param replacement the replacement
        /// \return the replaced reference
        template<typename T>
        TypedIterator<T> Replace(const OpaqueInstructionRef &instruction, const T &replacement) {
            ASSERT(instruction.basicBlock == this, "Instruction offset not from the same basic block");

            MarkAsDirty();

            const Instruction *ptr = GetRelocationInstruction(instruction.relocationOffset);

            if (ptr->result != InvalidID) {
                map.RemoveInstruction(ptr->result);
            }

            size_t size = GetSize(ptr);
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

#ifndef NDEBUG
            debugRevision++;
#endif

            return ResummarizeRelocationTable(instruction.relocationOffset);
        }

        /// Split this basic block from an iterator onwards
        /// \param destBlock the destination basic block in which all [splitIterator, end) will be inserted
        /// \param splitIterator the iterator from which on the block is splitted, inclusive
        /// \return the first iterator in the new basic block
        Iterator Split(BasicBlock* destBlock, const Iterator& splitIterator, BasicBlockSplitFlagSet splitFlags = BasicBlockSplitFlag::All);

        /// Split this basic block from an iterator onwards
        /// \param destBlock the destination basic block in which all [splitIterator, end) will be inserted
        /// \param splitIterator the iterator from which on the block is splitted, inclusive
        /// \return the first iterator in the new basic block
        template<typename T>
        TypedIterator<T> Split(BasicBlock* destBlock, const Iterator& splitIterator, BasicBlockSplitFlagSet splitFlags = BasicBlockSplitFlag::All) {
            return Split(destBlock, splitIterator, splitFlags);
        }

        /// Split this basic block from an iterator onwards
        /// \param destBlock the destination basic block in which all [splitIterator, end) will be inserted
        /// \param splitIterator the iterator from which on the block is splitted, inclusive
        /// \return the first iterator in the new basic block
        template<typename T>
        TypedIterator<T> Split(BasicBlock* destBlock, const TypedIterator<T>& splitIterator, BasicBlockSplitFlagSet splitFlags = BasicBlockSplitFlag::All) {
            return Split(destBlock, static_cast<Iterator>(splitIterator), splitFlags);
        }

        /// Get the terminator instruction
        /// \return the terminator instruction
        Iterator GetTerminator() {
            ASSERT(count, "No instructions");
            return Offset(relocationTable.back(), static_cast<uint32_t>(relocationTable.size()) - 1);
        }

        /// Get the terminator instruction
        /// \return the terminator instruction
        ConstIterator GetTerminator() const {
            ASSERT(count, "No instructions");
            return Offset(relocationTable.back(), static_cast<uint32_t>(relocationTable.size()) - 1);
        }

        /// Get an iterator from a reference
        Iterator GetIterator(const ConstOpaqueInstructionRef& ref) {
            ASSERT(ref.basicBlock == this, "Invalid reference");

            // Find location
            auto relocationIt = std::find(relocationTable.begin(), relocationTable.end(), ref.relocationOffset);
            ASSERT(relocationIt != relocationTable.end(), "Missing relocation offset");

            // Get the relocation index
            uint32_t index = static_cast<uint32_t>(relocationIt - relocationTable.begin());
            return Offset(ref.relocationOffset, index);
        }

        /// Get an iterator from a reference
        ConstIterator GetIterator(const ConstOpaqueInstructionRef& ref) const {
            ASSERT(ref.basicBlock == this, "Invalid reference");

            // Find location
            auto relocationIt = std::find(relocationTable.begin(), relocationTable.end(), ref.relocationOffset);
            ASSERT(relocationIt != relocationTable.end(), "Missing relocation offset");

            // Get the relocation index
            uint32_t index = static_cast<uint32_t>(relocationIt - relocationTable.begin());
            return Offset(ref.relocationOffset, index);
        }

        /// Mark this basic block as dirty
        void MarkAsDirty() {
            dirty = true;
        }

        /// Check if this basic block has been modified
        bool IsModified() const {
            return dirty;
        }

        /// Check if this basic block is empty
        bool IsEmpty() const {
            return data.empty();
        }

        /// Set the source span
        void SetSourceSpan(SourceSpan span) {
            sourceSpan = span;
        }

        /// Set the source span
        void SetID(IL::ID value) {
            id = value;
        }

        /// Immortalize this basic block
        void Immortalize(SourceSpan span) {
            dirty = false;
            sourceSpan = span;
        }

        /// Get all flags
        BasicBlockFlagSet GetFlags() const {
            return flags;
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

        /// Get the source span of this basic block
        SourceSpan GetSourceSpan() const {
            return sourceSpan;
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
        T *GetRelocationInstruction(const RelocationOffset *relocationOffset) {
            auto* instruction = reinterpret_cast<Instruction *>(data.data() + relocationOffset->offset);

            // Validation
            if constexpr(!std::is_same_v<T, Instruction>) {
                ASSERT(instruction->opCode == T::kOpCode, "Invalid cast");
            }

            return static_cast<T*>(instruction);
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
        /// Add all instruction references
        /// \param instruction instruction to be referenced
        /// \param ref appended reference
        void AddInstructionReferences(const Instruction* instruction, const OpaqueInstructionRef& ref);

        /// Insert a new relocation offset
        /// \param insertion the insertion relocation offset
        /// \param offset the new offset to be inserted
        /// \return the new iterator
        Iterator InsertRelocationOffset(const RelocationOffset* insertion, RelocationOffset* offset) {
            auto relocationIt = std::find(relocationTable.begin(), relocationTable.end(), insertion);
            ASSERT(relocationIt != relocationTable.end(), "Missing relocation offset");

            // Get the relocation index
            auto it = relocationTable.insert(relocationIt, offset);
            uint32_t index = static_cast<uint32_t>(it - relocationTable.begin());

            // Resumarrize from the index
            ResummarizeRelocationTable(offset, index);
            return Offset(offset, index);
        }

        /// Resummarize the relocation table for references
        /// \param offset the starting offset
        /// \return the iterator for a given offset
        Iterator ResummarizeRelocationTable(const RelocationOffset* offset) {
            auto relocationIt = std::find(relocationTable.begin(), relocationTable.end(), offset);
            ASSERT(relocationIt != relocationTable.end(), "Missing relocation offset");

            // Get the relocation index
            uint32_t index = static_cast<uint32_t>(relocationIt - relocationTable.begin());

            // Resumarrize from the index
            ResummarizeRelocationTable(offset, index);
            return Offset(offset, index);
        }

        /// Resummarize the relocation table for references
        /// \param offset the offset to start from
        /// \param relocationIndex the relocation index to start from
        void ResummarizeRelocationTable(const RelocationOffset* offset, uint32_t relocationIndex) {
            for (auto it = Offset(offset, relocationIndex); it != end(); it++) {
                // TODO: Stop when the offset is the same! (Not guaranteed atm, for the future)
                relocationTable[it.relocationIndex]->offset = static_cast<uint32_t>(it.ptr - data.data());
            }
        }

        /// Resummarize the relocation table for references
        void ResummarizeRelocationTable() {
            for (auto it = begin(); it != end(); it++) {
                // TODO: Stop when the offset is the same! (Not guaranteed atm, for the future)
                relocationTable[it.relocationIndex]->offset = static_cast<uint32_t>(it.ptr - data.data());
            }
        }

        /// Get an iterator from a relocation offset
        /// \param offset the relocation offset
        /// \param relocationIndex the index of the relocation offset
        /// \return
        Iterator Offset(const RelocationOffset* offset, uint32_t relocationIndex) {
            Iterator it;
            it.block = this;
            it.ptr = data.data() + offset->offset;
            it.relocationIndex = relocationIndex;
#ifndef NDEBUG
            it.debugRevision = debugRevision;
#endif
            return it;
        }

        /// Get an iterator from a relocation offset
        /// \param offset the relocation offset
        /// \param relocationIndex the index of the relocation offset
        /// \return
        ConstIterator Offset(const RelocationOffset* offset, uint32_t relocationIndex) const {
            ConstIterator it;
            it.block = this;
            it.ptr = data.data() + offset->offset;
            it.relocationIndex = relocationIndex;
#ifndef NDEBUG
            it.debugRevision = debugRevision;
#endif
            return it;
        }

        /// Remove a relocation offset
        void RemoveRelocationOffset(RelocationOffset *offset) {
            auto it = std::find(relocationTable.begin(), relocationTable.end(), offset);
            ASSERT(it != relocationTable.end(), "Dangling relocation offset");
            relocationTable.erase(it);
            relocationAllocator.Free(offset);
        }

    private:
        Allocators allocators;

        /// Label id
        ID id;

        /// Number of instructions
        uint32_t count{0};

        /// Source spain
        SourceSpan sourceSpan;

        /// The shared identifier map
        IdentifierMap &map;

        /// Instruction stream
        std::vector<uint8_t> data;

        /// The current relocation table for resummarization
        std::vector<RelocationOffset *> relocationTable;

        /// Relocation block allocator
        RelocationAllocator relocationAllocator;

        /// Block flags
        BasicBlockFlagSet flags{0};

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

    /// Getter implementation
    template<typename T, typename OPAQUE>
    T *TInstructionRef<T, OPAQUE>::GetMutable() const {
        return this->basicBlock->template GetRelocationInstruction<T>(this->relocationOffset);
    }
}
