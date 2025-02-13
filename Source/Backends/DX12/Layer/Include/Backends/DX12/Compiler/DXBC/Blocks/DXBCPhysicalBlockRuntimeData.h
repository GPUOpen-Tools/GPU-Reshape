// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
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

#pragma once

// Layer
#include <Backends/DX12/Compiler/DXBC/Blocks/DXBCPhysicalBlockSection.h>
#include <Backends/DX12/Compiler/DXBC/DXBCParseContext.h>
#include <Backends/DX12/Compiler/DXIL/DXILHeader.h>
#include <Backends/DX12/Compiler/DXBC/DXBCHeader.h>
#include <Backends/DX12/Compiler/DXStream.h>

// Forward declaration
struct DXBCPhysicalBlock;
struct DXCompileJob;

/// Runtime data block
struct DXBCPhysicalBlockRuntimeData : public DXBCPhysicalBlockSection {
    DXBCPhysicalBlockRuntimeData(const Allocators &allocators, Backend::IL::Program &program, DXBCPhysicalBlockTable &table);

    /// Parse signature
    void Parse();

    /// Compile runtime data
    void Compile(const DXCompileJob &job);

    /// Copy this block
    void CopyTo(DXBCPhysicalBlockRuntimeData &out);

private:
    struct Part {
        Part(const Allocators& allocators) : stream(allocators) {
            
        }

        /// Type if this part
        DXBCRuntimeDataPartType type;
        
        /// Block starting address
        const uint8_t* ptr{nullptr};

        /// Size of this block
        uint32_t length{0};

        /// Final stream
        DXStream stream;
    };

    template<typename T>
    struct TablePart {
        /// Header for this table
        DXBCRuntimeDataTableHeader header{};

        /// All contained records
        std::vector<T> records;
    };
    
    struct ResourceEntry {
        /// Serialized record
        DXBCRuntimeDataResourceRecord record;

        /// From the local root signature?
        bool isLocal = false;

        /// Extracted name
        std::string name;
    };

    struct FunctionEntry {
        /// Serialized record
        DXBCRuntimeDataFunctionRecord2 record;

        /// Has a local root signature?
        bool hasLocalRootSignature = false;

        /// Extracted name
        std::string name;

        /// Extracted unmangled name, exports work on this
        std::string unmangledName;

        /// All resource indices
        std::vector<uint32_t> resources;

        /// All dependency indices
        std::vector<uint32_t> dependencies;
    };

    struct SubObjectEntry {
        /// Serialized record
        DXBCRuntimeDataSubObjectRecord record;

        /// Extracted name
        std::string name;
    };

    template<typename T>
    struct RecordEntry {
        /// Serialized record
        T record;
    };

private:
    /// Plain part parsers
    void ParseStringPart(DXBCParseContext& partCtx);
    void ParseIndexArraysPart(DXBCParseContext& partCtx);
    void ParseRawBytesPart(DXBCParseContext& partCtx);

    /// Parse a common table part
    /// \tparam T record type
    /// \param partCtx context
    /// \param out destination table
    template<typename T>
    void ParseTablePart(DXBCParseContext& partCtx, TablePart<T>& out);

private:
    /// Add a resource to all functions
    /// \param job the parent job
    /// \param index resource index to add
    void AddResourceVisibility(const DXCompileJob& job, uint32_t index);

    /// Compile all resources
    /// \param job parent job
    void CompileResources(const DXCompileJob &job);

    /// Compile the visibility of all resources
    /// Ordering according to the expected layout
    /// \param job the parent job
    void CompileResourceVisibility(const DXCompileJob& job);

private:
    struct StringSet {
        std::vector<char>                         buffer;
        std::unordered_map<std::string, uint32_t> lookup;
    };
    
    struct IndexSet {
        std::vector<uint32_t> buffer;
        std::vector<uint32_t> offsets;
    };
    
private:
    /// Get a string from a record offset
    /// \param offset given offset
    /// \return null terminated
    std::string_view GetString(uint32_t offset);

    /// Insert a new string
    /// \param str string to add
    /// \param out destination part data
    /// \return offset
    uint32_t InsertString(const std::string_view& str, StringSet& out);

    /// Insert a set of indices
    /// \param begin index start
    /// \param end index end
    /// \param out destination part data
    /// \param set deduplication set
    /// \return offset
    uint32_t InsertIndices(uint32_t* begin, uint32_t* end, IndexSet& out);

    /// Insert an unsorted set of bytes
    /// \param start blob data
    /// \param length number of bytes
    /// \param out destination part data
    /// \return offset
    uint32_t InsertRaw(const void* start, uint32_t length, std::vector<uint8_t>& out);

    /// Insert a set of signature elements
    /// @param signaturesOffset index offset
    /// @param patchedStrings destination strings
    /// @param patchedIndices destination indices
    /// @return 
    uint32_t InsertSignatureElements(uint32_t signaturesOffset, StringSet& patchedStrings, IndexSet &patchedIndices);

private:
    /// Insert a table part
    /// \param block block to append to
    /// \param table table to insert
    /// \param partType type of the part
    template<typename T>
    void InsertTablePart(DXBCPhysicalBlock* block, const TablePart<T>& table, DXBCRuntimeDataPartType partType);

    /// Insert a record table part
    /// \param block block to append to
    /// \param table table to insert
    /// \param partOffset the header part offset
    /// \param partIndex the current part index, incremented
    template<typename T>
    void InsertRecordTablePart(DXBCPhysicalBlock * block, const TablePart<RecordEntry<T>>& table, uint64_t partOffset, uint32_t& partIndex);

private:
    /// All tables
    TablePart<ResourceEntry> resourceRecords;
    TablePart<FunctionEntry> functionRecords;
    TablePart<SubObjectEntry> subObjectRecords;
    TablePart<RecordEntry<DXBCRuntimeDataNodeID>> nodeIDRecords;
    TablePart<RecordEntry<DXBCRuntimeDataNodeShaderIOAttrib>> nodeShaderIOAttribRecords;
    TablePart<RecordEntry<DXBCRuntimeDataNodeShaderFuncAttrib>> nodeShaderFuncAttribRecords;
    TablePart<RecordEntry<DXBCRuntimeDataIONode>> ioNodeRecords;
    TablePart<RecordEntry<DXBCRuntimeDataNodeShaderInfo>> nodeShaderInfoRecords;
    TablePart<RecordEntry<DXBCRuntimeDataSignatureElement>> signatureElementRecords;
    TablePart<RecordEntry<DXBCRuntimeDataVSInfo>> vsInfoRecords;
    TablePart<RecordEntry<DXBCRuntimeDataPSInfo>> psInfoRecords;
    TablePart<RecordEntry<DXBCRuntimeDataHSInfo>> hsInfoRecords;
    TablePart<RecordEntry<DXBCRuntimeDataDSInfo>> dsInfoRecords;
    TablePart<RecordEntry<DXBCRuntimeDataGSInfo>> gsInfoRecords;
    TablePart<RecordEntry<DXBCRuntimeDataCSInfo>> csInfoRecords;
    TablePart<RecordEntry<DXBCRuntimeDataMSInfo>> msInfoRecords;
    TablePart<RecordEntry<DXBCRuntimeDataASInfo>> asInfoRecords;

private:
    struct ResourceBucket {
        /// All indices of this bucket
        std::vector<uint32_t> indices;
    };

    /// All resource buckets, not indexed from 0-count
    ResourceBucket resourceBuckets[static_cast<uint32_t>(DXILShaderResourceClass::Count)];

private:
    /// Parsed index sets
    Vector<uint32_t> indexBuffer;

    /// Parsed raw data
    Vector<uint8_t> rawBuffer;

    /// Parsed strings
    Vector<char> stringBuffer;

    /// End index of the user resources
    uint64_t userResourcesEnd = 0;

    /// RDAT header
    DXBCRuntimeDataHeader header;

    /// All parts
    Vector<Part> parts;
};

