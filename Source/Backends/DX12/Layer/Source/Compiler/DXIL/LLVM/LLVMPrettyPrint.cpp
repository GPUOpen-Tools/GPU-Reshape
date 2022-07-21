#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMPrettyPrint.h>

void PrettyPrint(LLVMPrettyPrintContext out, const LLVMRecord& record) {
    std::ostream& ss = out.Line();

    // Header
    switch (record.abbreviation.type) {
        case LLVMRecordAbbreviationType::None:
            ss << "[RECORD unabbrev";
            break;
        case LLVMRecordAbbreviationType::BlockMetadata:
            ss << "[RECORD metadata:" << record.abbreviation.abbreviationId;
            break;
        case LLVMRecordAbbreviationType::BlockLocal:
            ss << "[RECORD local:" << record.abbreviation.abbreviationId;
            break;
    }

    // Id
    ss << " id:" << record.id;

    // Ops
    for (uint32_t i = 0; i < record.opCount; i++) {
        ss << " op" << i << ":" << record.ops[i];
    }

    ss << "]\n";
}

void PrettyPrint(LLVMPrettyPrintContext out, const LLVMAbbreviation& abbreviation) {
    std::ostream& ss = out.Line();

    // Header
    ss << "[ABBREV";

    // Parameters
    for (uint32_t i = 0; i < abbreviation.parameters.Size(); i++) {
        const LLVMAbbreviationParameter& parameter = abbreviation.parameters[i];

        switch (parameter.encoding) {
            case LLVMAbbreviationEncoding::Literal:
                ss << " literal:" << parameter.value;
                break;
            case LLVMAbbreviationEncoding::Fixed:
                ss << " fixed:" << parameter.value;
                break;
            case LLVMAbbreviationEncoding::VBR:
                ss << " vbr:" << parameter.value;
                break;
            case LLVMAbbreviationEncoding::Array:
                ss << " array:" << parameter.value;
                break;
            case LLVMAbbreviationEncoding::Char6:
                ss << " ch6:" << parameter.value;
                break;
            case LLVMAbbreviationEncoding::Blob:
                ss << " blob:" << parameter.value;
                break;
        }
    }

    ss << "]\n";
}

void PrettyPrint(LLVMPrettyPrintContext out, const LLVMBlock* block) {
    std::ostream& ss = out.Line();

    // Header
    if (block->id == 0) {
        ss << "[BLOCK_INFO";
    } else {
        ss << "[BLOCK " << block->id;
    }

    // Abbreviation size
    ss << " abbrevSize:" << block->abbreviationSize;

    // Optional metadata
    if (block->metadata) {
        ss << " metadata:" << block->metadata;
    }

    ss << "\n";

    // All sub blocks
    for (const LLVMBlock* child : block->blocks) {
        PrettyPrint(out.Tab(), child);
    }

    // All abbreviations within
    for (const LLVMAbbreviation& abbreviation : block->abbreviations) {
        PrettyPrint(out.Tab(), abbreviation);
    }

    // All records within
    for (const LLVMRecord& record : block->records) {
        PrettyPrint(out.Tab(), record);
    }

    out.Line() << "]\n";
}
