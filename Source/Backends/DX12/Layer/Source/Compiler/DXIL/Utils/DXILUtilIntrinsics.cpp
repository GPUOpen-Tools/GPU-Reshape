#include <Backends/DX12/Compiler/DXIL/Utils/DXILUtilIntrinsics.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>

void DXILUtilIntrinsics::Compile() {
    // Get type map
    Backend::IL::TypeMap &typeMap = program.GetTypeMap();

    // Inbuilt
    voidType = typeMap.FindTypeOrAdd(Backend::IL::VoidType{});

    // Numeric
    i1Type = typeMap.FindTypeOrAdd(Backend::IL::BoolType{});
    i8Type = typeMap.FindTypeOrAdd(Backend::IL::IntType{.bitWidth = 8, .signedness = true});
    i32Type = typeMap.FindTypeOrAdd(Backend::IL::IntType{.bitWidth = 32, .signedness = true});
    f32Type = typeMap.FindTypeOrAdd(Backend::IL::FPType{.bitWidth = 32});

    // Structured handle type
    handleType = typeMap.FindTypeOrAdd(Backend::IL::StructType{
        .memberTypes {
            typeMap.FindTypeOrAdd(Backend::IL::PointerType{
                .pointee = i8Type,
                .addressSpace = Backend::IL::AddressSpace::Function
            })
        }
    });

    // Structured size map
    dimensionsType = table.type.typeMap.CompileNamedType(typeMap.FindTypeOrAdd(Backend::IL::StructType {
        .memberTypes {
            i32Type,
            i32Type,
            i32Type,
            i32Type
        }
    }), "dx.types.Dimensions");
}

const DXILFunctionDeclaration *DXILUtilIntrinsics::GetIntrinsic(const DXILIntrinsicSpec &spec) {
    /*
     * LLVM Specification
     *   [FUNCTION, type, callingconv, isproto,
     *    linkage, paramattr, alignment, section, visibility, gc, prologuedata,
     *     dllstorageclass, comdat, prefixdata, personalityfn, preemptionspecifier]
     */

    // Ensure enough space
    if (spec.uid >= intrinsics.size()) {
        intrinsics.resize(spec.uid + 1);
    }

    // Already populated?
    if (intrinsics[spec.uid].declaration) {
        return intrinsics[spec.uid].declaration;
    }

    // Existing declaration?
    if (const DXILFunctionDeclaration* decl = table.function.FindDeclaration(spec.name)) {
        intrinsics[spec.uid].declaration = decl;
        return decl;
    }

    // Root block
    LLVMBlock *global = &table.scan.GetRoot();

    // Get type map
    Backend::IL::TypeMap &typeMap = program.GetTypeMap();

    // Function signature
    Backend::IL::FunctionType funcTy;
    funcTy.returnType = GetType(spec.returnType);

    // Add parameters
    for (size_t i = 0; i < spec.parameterTypes.size(); i++) {
        funcTy.parameterTypes.push_back(GetType(spec.parameterTypes[i]));
    }

    // Compile function type
    const Backend::IL::FunctionType *fnType = typeMap.FindTypeOrAdd(funcTy);

    // TODO: Intrinsic parameter group support
    LLVMParameterGroupValue attributes[] = { LLVMParameterGroupValue::NoUnwind };

    // Module scope function declaration
    LLVMRecord record(LLVMModuleRecord::Function);
    record.SetUser(true, ~0u, program.GetIdentifierMap().AllocID());
    record.opCount = 15;
    record.ops = table.recordAllocator.AllocateArray<uint64_t>(record.opCount);
    record.ops[0] = table.type.typeMap.GetType(fnType);
    record.ops[1] = static_cast<uint32_t>(LLVMCallingConvention::C);
    record.ops[2] = 1;
    record.ops[3] = static_cast<uint64_t>(LLVMLinkage::ExternalLinkage);
    record.ops[4] = table.functionAttribute.FindOrCompileAttributeList(1, attributes);
    record.ops[5] = 0;
    record.ops[6] = 0;
    record.ops[7] = 0;
    record.ops[8] = 0;
    record.ops[9] = 0;
    record.ops[10] = 0;
    record.ops[11] = 0;
    record.ops[12] = 0;
    record.ops[13] = 0;
    record.ops[14] = 0;

    // Insert after previous functions
    const LLVMBlockElement* insertionPoint = global->FindPlacementReverse(LLVMBlockElementType::Record, LLVMModuleRecord::Function) + 1;
    global->InsertRecord(insertionPoint, record);

    // Symbol tab for linking
    LLVMBlock *symTab = global->GetBlock(LLVMReservedBlock::ValueSymTab);

    // Symbol entry
    LLVMRecord syRecord(LLVMSymTabRecord::Entry);
    syRecord.SetUser(false, ~0u, ~0u);
    syRecord.opCount = 1u + spec.name.length();
    syRecord.ops = table.recordAllocator.AllocateArray<uint64_t>(syRecord.opCount);
    syRecord.ops[0] = DXILIDRemapper::EncodeUserOperand(record.result);

    // Copy name
    for (uint32_t i = 0; i < syRecord.opCount - 1; i++) {
        syRecord.ops[1 + i] = spec.name[i];
    }

    // Emit symbol
    symTab->AddRecord(syRecord);

    // Create function declaration
    auto* decl = table.function.AddDeclaration(DXILFunctionDeclaration {
        .id = DXILIDRemapper::EncodeUserOperand(record.result),
        .type = fnType,
        .linkage = LLVMLinkage::CommonLinkage
    });

    // Cache declaration
    intrinsics[spec.uid].declaration = decl;

    // OK
    return decl;
}

const Backend::IL::Type *DXILUtilIntrinsics::GetType(const DXILIntrinsicTypeSpec &type) {
    switch (type) {
        default:
            ASSERT(false, "Not implemented");
        case DXILIntrinsicTypeSpec::Void:
            return voidType;
        case DXILIntrinsicTypeSpec::I32:
            return i32Type;
        case DXILIntrinsicTypeSpec::F32:
            return f32Type;
        case DXILIntrinsicTypeSpec::I8:
            return i8Type;
        case DXILIntrinsicTypeSpec::I1:
            return i1Type;
        case DXILIntrinsicTypeSpec::Handle:
            return handleType;
        case DXILIntrinsicTypeSpec::Dimensions:
            return dimensionsType;
    }
}