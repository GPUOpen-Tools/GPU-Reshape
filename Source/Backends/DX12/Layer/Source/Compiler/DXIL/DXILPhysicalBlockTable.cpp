#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMHeader.h>

DXILPhysicalBlockTable::DXILPhysicalBlockTable(const Allocators &allocators, IL::Program &program) :
    scan(allocators),
    program(program),
    idMap(program),
    idRemapper(idMap),
    function(allocators, program, *this),
    functionAttribute(allocators, program, *this),
    type(allocators, program, *this),
    global(allocators, program, *this),
    string(allocators, program, *this),
    symbol(allocators, program, *this),
    metadata(allocators, program, *this),
    intrinsics(allocators, program, *this),
    recordAllocator(allocators) {

}

bool DXILPhysicalBlockTable::Parse(const void *byteCode, uint64_t byteLength) {
    if (!scan.Scan(byteCode, byteLength)) {
        return false;
    }

    /*
     * The DXIL scanner works slightly different than the Spv scanner, each physical block cannot be
     * parsed separately, as the identifier mapping is allocated when a particular record is present.
     * While this is clever from LLVMs side, as it reduces the effective binary size, it is a little
     * more involved to parse.
     * */

    LLVMBlock &root = scan.GetRoot();

    // Pre-parse all types for local fetching
    for (const LLVMBlock *block: root.blocks) {
        switch (static_cast<LLVMReservedBlock>(block->id)) {
            default:
                // Handled later
                break;
            case LLVMReservedBlock::Type:
                type.ParseType(block);
                break;
        }
    }

    // Visit all records
    for (LLVMRecord &record: root.records) {
        switch (static_cast<LLVMModuleRecord>(record.id)) {
            default: {
                ASSERT(false, "Unexpected block id");
                break;
            }
            case LLVMModuleRecord::Version: {
                if (record.opCount != 1 || record.ops[0] != 1) {
                    ASSERT(false, "Unsupported LLVM version");
                    return false;
                }
                break;
            }
            case LLVMModuleRecord::Triple: {
                triple.resize(record.opCount);
                record.FillOperands(triple.data());
                break;
            }
            case LLVMModuleRecord::DataLayout: {
                dataLayout.resize(record.opCount);
                record.FillOperands(dataLayout.data());
                break;
            }
            case LLVMModuleRecord::ASM:
                break;
            case LLVMModuleRecord::SectionName:
                break;
            case LLVMModuleRecord::DepLib:
                break;
            case LLVMModuleRecord::GlobalVar:
                global.ParseGlobalVar(record);
                break;
            case LLVMModuleRecord::Function:
                function.ParseModuleFunction(record);
                break;
            case LLVMModuleRecord::Alias:
                global.ParseAlias(record);
                break;
            case LLVMModuleRecord::GCName:
                break;
        }
    }


    // Visit all blocks
    for (LLVMBlock *block: root.blocks) {
        switch (static_cast<LLVMReservedBlock>(block->id)) {
            default:
                ASSERT(false, "Unexpected block id");
                break;
            case LLVMReservedBlock::Info:
                break;
            case LLVMReservedBlock::Module:
                break;
            case LLVMReservedBlock::Parameter:
                functionAttribute.ParseParameterBlock(block);
                break;
            case LLVMReservedBlock::ParameterGroup:
                functionAttribute.ParseParameterAttributeGroup(block);
                break;
            case LLVMReservedBlock::Constants:
                global.ParseConstants(block);
                break;
            case LLVMReservedBlock::Function:
                function.ParseFunction(block);
                break;
            case LLVMReservedBlock::ValueSymTab:
                symbol.ParseSymTab(block);
                break;
            case LLVMReservedBlock::Metadata:
                metadata.ParseMetadata(block);
                break;
            case LLVMReservedBlock::MetadataAttachment:
                break;
            case LLVMReservedBlock::Type:
                // Types already parsed
                break;
            case LLVMReservedBlock::StrTab:
                string.ParseStrTab(block);
                break;
        }
    }

    // OK
    return true;
}

bool DXILPhysicalBlockTable::Compile(const DXJob &job) {
    LLVMBlock &root = scan.GetRoot();

    // Set the remap bound
    idRemapper.SetBound(idMap.GetBound(), program.GetIdentifierMap().GetMaxID());

    // Set declaration blocks for on-demand records
    metadata.SetDeclarationBlock(&root);
    functionAttribute.SetDeclarationBlock(&root);
    type.typeMap.SetDeclarationBlock(root.GetBlock(LLVMReservedBlock::Type));
    global.constantMap.SetDeclarationBlock(root.GetBlock(LLVMReservedBlock::Constants));

    // Compile utilities
    intrinsics.Compile();

    // Insert all SE metadata
    metadata.CompileShaderExportResources(job);

    // Pre-parse all types for local fetching
    for (LLVMBlock *block: root.blocks) {
        switch (static_cast<LLVMReservedBlock>(block->id)) {
            default:
                // Handled later
                break;
            case LLVMReservedBlock::Type:
                type.CompileType(block);
                break;
        }
    }

    // Visit all records
    for (LLVMRecord &record: root.records) {
        switch (static_cast<LLVMModuleRecord>(record.id)) {
            default: {
                ASSERT(false, "Unexpected block id");
                break;
            }
            case LLVMModuleRecord::Version:
                break;
            case LLVMModuleRecord::Triple:
                break;
            case LLVMModuleRecord::DataLayout:
                break;
            case LLVMModuleRecord::ASM:
                break;
            case LLVMModuleRecord::SectionName:
                break;
            case LLVMModuleRecord::DepLib:
                break;
            case LLVMModuleRecord::GlobalVar:
                global.CompileGlobalVar(record);
                break;
            case LLVMModuleRecord::Function:
                function.CompileModuleFunction(record);
                break;
            case LLVMModuleRecord::Alias:
                global.CompileAlias(record);
                break;
            case LLVMModuleRecord::GCName:
                break;
        }
    }

    // Visit all blocks
    for (LLVMBlock *block: root.blocks) {
        switch (static_cast<LLVMReservedBlock>(block->id)) {
            default:
            ASSERT(false, "Unexpected block id");
                break;
            case LLVMReservedBlock::Info:
                break;
            case LLVMReservedBlock::Module:
                break;
            case LLVMReservedBlock::Parameter:
                break;
            case LLVMReservedBlock::ParameterGroup:
                break;
            case LLVMReservedBlock::Constants:
                global.CompileConstants(block);
                break;
            case LLVMReservedBlock::Function:
                function.CompileFunction(job, block);
                break;
            case LLVMReservedBlock::ValueSymTab:
                symbol.CompileSymTab(block);
                break;
            case LLVMReservedBlock::Metadata:
                break;
            case LLVMReservedBlock::MetadataAttachment:
                break;
            case LLVMReservedBlock::Type:
                break;
            case LLVMReservedBlock::StrTab:
                string.CompileStrTab(block);
                break;
        }
    }

    // Compile dynamic blocks last
    for (LLVMBlock *block: root.blocks) {
        switch (static_cast<LLVMReservedBlock>(block->id)) {
            default:
                break;
            case LLVMReservedBlock::Constants:
                global.CompileConstants(block);
                break;
            case LLVMReservedBlock::Metadata:
                metadata.CompileMetadata(block);
                break;
            case LLVMReservedBlock::Type:
                type.CompileType(block);
                break;
        }
    }

    // Compile dynamic global metadata
    metadata.CompileMetadata();

    // OK
    return true;
}

void DXILPhysicalBlockTable::Stitch(DXStream &out) {
    LLVMBlock &root = scan.GetRoot();

    // Set the remap bound
    idRemapper.SetBound(idMap.GetBound(), program.GetIdentifierMap().GetMaxID());

    // Visit in declaration order
    for (const LLVMBlockElement& element : root.elements) {
        if (element.Is(LLVMBlockElementType::Record)) {
            LLVMRecord& record = root.records[element.id];
            switch (static_cast<LLVMModuleRecord>(record.id)) {
                default:
                    ASSERT(false, "Unexpected block id");
                    break;
                case LLVMModuleRecord::Version:
                    break;
                case LLVMModuleRecord::Triple:
                    break;
                case LLVMModuleRecord::DataLayout:
                    break;
                case LLVMModuleRecord::ASM:
                    break;
                case LLVMModuleRecord::SectionName:
                    break;
                case LLVMModuleRecord::DepLib:
                    break;
                case LLVMModuleRecord::GlobalVar:
                    global.StitchGlobalVar(record);
                    break;
                case LLVMModuleRecord::Function:
                    function.StitchModuleFunction(record);
                    break;
                case LLVMModuleRecord::Alias:
                    global.StitchAlias(record);
                    break;
                case LLVMModuleRecord::GCName:
                    break;
            }
        } else if (element.Is(LLVMBlockElementType::Block)) {
            LLVMBlock* block = root.blocks[element.id];
            switch (static_cast<LLVMReservedBlock>(block->id)) {
                default:
                    ASSERT(false, "Unexpected block id");
                    break;
                case LLVMReservedBlock::Info:
                    break;
                case LLVMReservedBlock::Module:
                    break;
                case LLVMReservedBlock::Parameter:
                    break;
                case LLVMReservedBlock::ParameterGroup:
                    break;
                case LLVMReservedBlock::Constants:
                    global.StitchConstants(block);
                    break;
                case LLVMReservedBlock::Function:
                    function.StitchFunction(block);
                    break;
                case LLVMReservedBlock::ValueSymTab:
                    symbol.StitchSymTab(block);
                    break;
                case LLVMReservedBlock::Metadata:
                    metadata.StitchMetadata(block);
                    break;
                case LLVMReservedBlock::MetadataAttachment:
                    break;
                case LLVMReservedBlock::Type:
                    type.StitchType(block);
                    break;
                case LLVMReservedBlock::StrTab:
                    string.StitchStrTab(block);
                    break;
            }
        }
    }

    // Fixup all forward references to their new value indices
    idRemapper.ResolveForwardReferences();

    // Stitch final block
    scan.Stitch(out);
}

void DXILPhysicalBlockTable::CopyTo(DXILPhysicalBlockTable &out) {
    scan.CopyTo(out.scan);

    // Table maps
    idMap.CopyTo(out.idMap);
    idRemapper.CopyTo(out.idRemapper);

    // Blocks
    type.CopyTo(out.type);
    global.CopyTo(out.global);
    symbol.CopyTo(out.symbol);
    metadata.CopyTo(out.metadata);
    function.CopyTo(out.function);
    functionAttribute.CopyTo(out.functionAttribute);
}
