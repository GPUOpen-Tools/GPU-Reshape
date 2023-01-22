#pragma once

// Layer
#include "LLVMRecord.h"
#include "LLVMAbbreviation.h"
#include "LLVMBlockElement.h"

// Std
#include <cstdint>

// Common
#include <Common/Containers/TrivialStackVector.h>

// Forward declarations
struct LLVMBlockMetadata;

struct LLVMBlock {
    LLVMBlock(LLVMReservedBlock id = {}) : id(static_cast<uint32_t>(id)) {

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
        for (int64_t i = elements.Size() - 1; i >= 0; i--) {
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
        elements.Add(LLVMBlockElement(LLVMBlockElementType::Record, static_cast<uint32_t>(records.Size())));
        records.Add(record);
    }

    /// Add a block to the end of this block
    /// \param record block to be added
    void AddBlock(LLVMBlock* block) {
        elements.Add(LLVMBlockElement(LLVMBlockElementType::Block, static_cast<uint32_t>(blocks.Size())));
        blocks.Add(block);
    }

    /// Add a record at a location
    /// \param record record to be added
    void InsertRecord(const LLVMBlockElement* location, const LLVMRecord& record) {
        elements.Insert(location, LLVMBlockElement(LLVMBlockElementType::Record, static_cast<uint32_t>(records.Size())));
        records.Add(record);
    }

    /// Add a block at a location
    /// \param record block to be added
    void InsertBlock(const LLVMBlockElement* location, LLVMBlock* block) {
        elements.Insert(location, LLVMBlockElement(LLVMBlockElementType::Block, static_cast<uint32_t>(blocks.Size())));
        blocks.Add(block);
    }

    /// Identifier of this block, may be reserved
    uint32_t id{~0u};

    /// Unique identifier of this block
    uint32_t uid{~0u};

    /// Abbreviation size
    uint32_t abbreviationSize{~0u};

    /// First scan block length
    uint32_t blockLength{~0u};

    /// All child blocks
    TrivialStackVector<LLVMBlock*, 32> blocks;

    /// All records within this block
    TrivialStackVector<LLVMRecord, 32> records;

    /// All abbreviations local to this block
    TrivialStackVector<LLVMAbbreviation, 32> abbreviations;

    /// Elements in declaration order
    TrivialStackVector<LLVMBlockElement, 128> elements;

    /// Optional metadata
    LLVMBlockMetadata* metadata{nullptr};
};
