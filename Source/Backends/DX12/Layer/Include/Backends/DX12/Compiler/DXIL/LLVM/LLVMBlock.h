#pragma once

// Layer
#include "LLVMRecord.h"
#include "LLVMAbbreviation.h"
#include "LLVMBlockElement.h"
#include <Backends/DX12/Compiler/Tags.h>

// Std
#include <cstdint>

// Common
#include <Common/Containers/TrivialStackVector.h>
#include <Common/Allocator/PolyAllocator.h>

// Forward declarations
struct LLVMBlockMetadata;

struct LLVMBlock {
    LLVMBlock(const Allocators& allocators, LLVMReservedBlock id = {}) :
        polyAllocator(allocators, kAllocModuleLLVMBlockElements),
        id(static_cast<uint32_t>(id)),
        blocks(&polyAllocator),
        records(&polyAllocator),
        abbreviations(&polyAllocator),
        elements(&polyAllocator) {
        /** */
    }

    /// Check if this block is of reserved id
    template<typename T>
    bool Is(T value) const {
        return value == static_cast<T>(id);
    }

    /// Interpret the id as a reserved type
    template<typename T>
    T As() const {
        return static_cast<T>(id);
    }

    /// Get a sub-block
    /// \param id reserved identifier
    /// \return nullptr if not found
    LLVMBlock* GetBlock(LLVMReservedBlock _id) {
        for (LLVMBlock* block : blocks) {
            if (block->Is(_id)) {
                return block;
            }
        }

        return nullptr;
    }

    /// Get a sub-block
    /// \param uid unique id
    /// \return nullptr if not found
    LLVMBlock* GetBlockWithUID(uint32_t _uid) {
        for (LLVMBlock* block : blocks) {
            if (block->uid == _uid) {
                return block;
            }
        }

        return nullptr;
    }

    /// Find a block placement
    /// \param rid record id
    /// \return nullptr if none found
    template<typename T>
    const LLVMBlockElement* FindPlacement(LLVMBlockElementType type, const T& rid) {
        for (const LLVMBlockElement& element : elements) {
            if (!element.Is(type)) {
                continue;
            }

            bool match = false;

            switch (type) {
                case LLVMBlockElementType::Abbreviation:
                    match = true;
                    break;
                case LLVMBlockElementType::Record:
                    match = records[element.id].Is(rid);
                    break;
                case LLVMBlockElementType::Block:
                    match = blocks[element.id]->Is(rid);
                    break;
            }

            if (match) {
                return &element;
            }
        }

        return nullptr;
    }

    /// Find a block placement
    /// \param rid record id
    /// \return nullptr if none found
    const LLVMBlockElement* FindPlacement(LLVMBlockElementType type) {
        for (const LLVMBlockElement& element : elements) {
            if (element.Is(type)) {
                return &element;
            }
        }

        return nullptr;
    }

    /// Find a block placement, reverse search
    /// \param rid record id
    /// \return nullptr if none found
    template<typename T>
    const LLVMBlockElement* FindPlacementReverse(LLVMBlockElementType type, const T& rid) {
        for (int64_t i = elements.size() - 1; i >= 0; i--) {
            const LLVMBlockElement& element = elements[i];

            if (!element.Is(type)) {
                continue;
            }

            bool match = false;

            switch (type) {
                case LLVMBlockElementType::Abbreviation:
                    match = true;
                    break;
                case LLVMBlockElementType::Record:
                    match = records[element.id].Is(rid);
                    break;
                case LLVMBlockElementType::Block:
                    match = blocks[element.id]->Is(rid);
                    break;
            }

            if (match) {
                return &element;
            }
        }

        return nullptr;
    }

    /// Add a record to the end of this block
    /// \param record record to be added
    void AddRecord(const LLVMRecord& record) {
        elements.push_back(LLVMBlockElement(LLVMBlockElementType::Record, static_cast<uint32_t>(records.size())));
        records.push_back(record);
    }

    /// Add a block to the end of this block
    /// \param record block to be added
    void AddBlock(LLVMBlock* block) {
        elements.push_back(LLVMBlockElement(LLVMBlockElementType::Block, static_cast<uint32_t>(blocks.size())));
        blocks.push_back(block);
    }

    /// Add a record at a location
    /// \param record record to be added
    void InsertRecord(const LLVMBlockElement* location, const LLVMRecord& record) {
        elements.insert(AsIterator(location), LLVMBlockElement(LLVMBlockElementType::Record, static_cast<uint32_t>(records.size())));
        records.push_back(record);
    }

    /// Add a block at a location
    /// \param record block to be added
    void InsertBlock(const LLVMBlockElement* location, LLVMBlock* block) {
        elements.insert(AsIterator(location), LLVMBlockElement(LLVMBlockElementType::Block, static_cast<uint32_t>(blocks.size())));
        blocks.push_back(block);
    }

    /// Shared allocator
    PolyAllocator polyAllocator;

    /// Identifier of this block, may be reserved
    uint32_t id{~0u};
    
    /// Unique identifier of this block
    uint32_t uid{~0u};

    /// Abbreviation size
    uint32_t abbreviationSize{~0u};

    /// First scan block length
    uint32_t blockLength{~0u};

    /// All child blocks
    std::pmr::vector<LLVMBlock*> blocks;

    /// All records within this block
    std::pmr::vector<LLVMRecord> records;

    /// All abbreviations local to this block
    std::pmr::vector<LLVMAbbreviation> abbreviations;

    /// Elements in declaration order
    std::pmr::vector<LLVMBlockElement> elements;

    /// Optional metadata
    LLVMBlockMetadata* metadata{nullptr};

private:
    /// Element to iterator
    std::pmr::vector<LLVMBlockElement>::const_iterator AsIterator(const LLVMBlockElement* element) const {
        return elements.begin() + std::distance(elements.data(), element);
    }
};
