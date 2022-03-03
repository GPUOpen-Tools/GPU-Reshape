#include <Backends/Vulkan/Compiler/Utils/SpvUtilShaderExport.h>
#include <Backends/Vulkan/Compiler/SpvPhysicalBlockTable.h>
#include <Backends/Vulkan/Compiler/SpvJob.h>

SpvUtilShaderExport::SpvUtilShaderExport(const Allocators &allocators, IL::Program &program, SpvPhysicalBlockTable &table) :
    allocators(allocators),
    program(program),
    table(table) {

}

void SpvUtilShaderExport::CompileRecords(const SpvJob &job) {
    // Note: This is quite ugly, will be changed

    // Capability set
    table.capability.Add(SpvCapabilityImageBuffer);

    // IL type map
    Backend::IL::TypeMap &ilTypeMap = program.GetTypeMap();

    // UInt32
    const Backend::IL::Type *intType = ilTypeMap.FindTypeOrAdd(Backend::IL::IntType{
        .bitWidth = 32,
        .signedness = false
    });

    // RWBuffer<uint>
    buffer32UIRW = ilTypeMap.FindTypeOrAdd(Backend::IL::BufferType{
        .elementType = intType,
        .texelType = Backend::IL::Format::R32UInt
    });

    // RWBuffer<uint>*
    buffer32UIRWPtr = ilTypeMap.FindTypeOrAdd(Backend::IL::PointerType{
        .pointee = buffer32UIRW,
        .addressSpace = Backend::IL::AddressSpace::Resource,
    });

    // RWBuffer<uint>[N]
    const Backend::IL::Type *buffer32UIWWArray = ilTypeMap.FindTypeOrAdd(Backend::IL::ArrayType{
        .elementType = buffer32UIRW,
        .count = std::max(1u, job.streamCount)
    });

    // RWBuffer<uint>[N]*
    buffer32UIRWArrayPtr = ilTypeMap.FindTypeOrAdd(Backend::IL::PointerType{
        .pointee = buffer32UIWWArray,
        .addressSpace = Backend::IL::AddressSpace::Resource,
    });

    // Id allocations
    counterId = table.scan.header.bound++;
    streamId = table.scan.header.bound++;

    // SpvIds
    SpvId buffer32UIRWPtrId = table.typeConstantVariable.typeMap.GetSpvTypeId(buffer32UIRWPtr);
    SpvId buffer32UIRWArrayPtrId = table.typeConstantVariable.typeMap.GetSpvTypeId(buffer32UIRWArrayPtr);

    // Counter
    SpvInstruction &spvCounterVar = table.typeConstantVariable.block->stream.Allocate(SpvOpVariable, 4);
    spvCounterVar[1] = buffer32UIRWPtrId;
    spvCounterVar[2] = counterId;
    spvCounterVar[3] = SpvStorageClassUniformConstant;

    // Streams
    SpvInstruction &spvStreamVar = table.typeConstantVariable.block->stream.Allocate(SpvOpVariable, 4);
    spvStreamVar[1] = buffer32UIRWArrayPtrId;
    spvStreamVar[2] = streamId;
    spvStreamVar[3] = SpvStorageClassUniformConstant;

    // Descriptor set
    SpvInstruction &spvCounterSet = table.annotation.block->stream.Allocate(SpvOpDecorate, 4);
    spvCounterSet[1] = counterId;
    spvCounterSet[2] = SpvDecorationDescriptorSet;
    spvCounterSet[3] = job.instrumentationKey.pipelineLayoutUserSlots;

    // Binding
    SpvInstruction &spvCounterBinding = table.annotation.block->stream.Allocate(SpvOpDecorate, 4);
    spvCounterBinding[1] = counterId;
    spvCounterBinding[2] = SpvDecorationBinding;
    spvCounterBinding[3] = 0;

    // Descriptor set
    SpvInstruction &spvStreamSet = table.annotation.block->stream.Allocate(SpvOpDecorate, 4);
    spvStreamSet[1] = streamId;
    spvStreamSet[2] = SpvDecorationDescriptorSet;
    spvStreamSet[3] = job.instrumentationKey.pipelineLayoutUserSlots;

    // Binding
    SpvInstruction &spvStreamBinding = table.annotation.block->stream.Allocate(SpvOpDecorate, 4);
    spvStreamBinding[1] = streamId;
    spvStreamBinding[2] = SpvDecorationBinding;
    spvStreamBinding[3] = 1;
}

void SpvUtilShaderExport::Export(SpvStream &stream, uint32_t exportID, IL::ID value) {
    Backend::IL::TypeMap &ilTypeMap = program.GetTypeMap();

    // Note: This is quite ugly, will be changed

    // UInt32
    Backend::IL::IntType typeInt;
    typeInt.bitWidth = 32;
    typeInt.signedness = false;
    const Backend::IL::Type *uintType = ilTypeMap.FindTypeOrAdd(typeInt);

    // Uint32*
    Backend::IL::PointerType typeUintImagePtr;
    typeUintImagePtr.pointee = uintType;
    typeUintImagePtr.addressSpace = Backend::IL::AddressSpace::Texture;
    const Backend::IL::Type *uintImagePtrType = ilTypeMap.FindTypeOrAdd(typeUintImagePtr);

    // Constant identifiers
    uint32_t zeroUintId = table.scan.header.bound++;
    uint32_t streamOffsetId = table.scan.header.bound++;
    uint32_t scopeId = table.scan.header.bound++;
    uint32_t memSemanticId = table.scan.header.bound++;
    uint32_t offsetAdditionId = table.scan.header.bound++;

    // 0
    SpvInstruction &spv = table.typeConstantVariable.block->stream.Allocate(SpvOpConstant, 4);
    spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(uintType);
    spv[2] = zeroUintId;
    spv[3] = 0;

    // Index of the stream
    SpvInstruction &spvOffset = table.typeConstantVariable.block->stream.Allocate(SpvOpConstant, 4);
    spvOffset[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(uintType);
    spvOffset[2] = streamOffsetId;
    spvOffset[3] = exportID;

    // Device scope
    SpvInstruction &spvScope = table.typeConstantVariable.block->stream.Allocate(SpvOpConstant, 4);
    spvScope[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(uintType);
    spvScope[2] = scopeId;
    spvScope[3] = SpvScopeDevice;

    // No memory mask
    SpvInstruction &spvMemSem = table.typeConstantVariable.block->stream.Allocate(SpvOpConstant, 4);
    spvMemSem[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(uintType);
    spvMemSem[2] = memSemanticId;
    spvMemSem[3] = SpvMemorySemanticsMaskNone;

    // The offset addition (will change for dynamic types in the future)
    SpvInstruction &spvSize = table.typeConstantVariable.block->stream.Allocate(SpvOpConstant, 4);
    spvSize[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(uintType);
    spvSize[2] = offsetAdditionId;
    spvSize[3] = 1u;

    uint32_t texelPtrId = table.scan.header.bound++;

    // Get the address of the texel to be atomically incremented
    SpvInstruction &texelPtr = stream.Allocate(SpvOpImageTexelPointer, 6);
    texelPtr[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(uintImagePtrType);
    texelPtr[2] = texelPtrId;
    texelPtr[3] = counterId;
    texelPtr[4] = streamOffsetId;
    texelPtr[5] = zeroUintId;

    uint32_t atomicPositionId = table.scan.header.bound++;

    // Atomically increment the texel
    SpvInstruction &atom = stream.Allocate(SpvOpAtomicIAdd, 7);
    atom[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(uintType);
    atom[2] = atomicPositionId;
    atom[3] = texelPtrId;
    atom[4] = scopeId;
    atom[5] = memSemanticId;
    atom[6] = offsetAdditionId;

    uint32_t accessId = table.scan.header.bound++;

    // Get the destination stream
    SpvInstruction &chain = stream.Allocate(SpvOpAccessChain, 5);
    chain[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(buffer32UIRWPtr);
    chain[2] = accessId;
    chain[3] = streamId;
    chain[4] = streamOffsetId;

    uint32_t accessLoadId = table.scan.header.bound++;

    // Load the stream
    SpvInstruction &load = stream.Allocate(SpvOpLoad, 4);
    load[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(buffer32UIRW);
    load[2] = accessLoadId;
    load[3] = accessId;

    // Write to the stream
    SpvInstruction &write = stream.Allocate(SpvOpImageWrite, 5);
    write[1] = accessLoadId;
    write[2] = atomicPositionId;
    write[3] = value;
    write[4] = SpvImageOperandsMaskNone;
}

void SpvUtilShaderExport::CopyTo(SpvPhysicalBlockTable &remote, SpvUtilShaderExport &out) {
    out.counterId = counterId;
    out.streamId = streamId;
    out.buffer32UIRWArrayPtr = buffer32UIRWArrayPtr;
    out.buffer32UIRWPtr = buffer32UIRWPtr;
    out.buffer32UIRW = buffer32UIRW;
}
