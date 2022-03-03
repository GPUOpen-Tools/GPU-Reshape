#include <Backends/Vulkan/Compiler/SpvPhysicalBlockTable.h>

SpvPhysicalBlockTable::SpvPhysicalBlockTable(const Allocators &allocators, IL::Program &program) :
    capability(allocators, program, *this),
    annotation(allocators, program, *this),
    debugStringSource(allocators, program, *this),
    typeConstantVariable(allocators, program, *this),
    function(allocators, program, *this),
    shaderExport(allocators, program, *this),
    program(program),
    scan(program) {
    /* */
}

bool SpvPhysicalBlockTable::Parse(const uint32_t *code, uint32_t count) {
    // Attempt to scan the blocks
    if (!scan.Scan(code, count)) {
        return false;
    }

    // Parse all blocks
    capability.Parse();
    annotation.Parse();
    debugStringSource.Parse();
    typeConstantVariable.Parse();
    function.Parse();

    // OK
    return true;
}

bool SpvPhysicalBlockTable::Compile(const SpvJob &job) {
    // Set the new bound
    scan.header.bound = program.GetIdentifierMap().GetMaxID();

    // Create id map
    SpvIdMap idMap;
    idMap.SetBound(&scan.header.bound);

    // Compile pre blocks
    typeConstantVariable.Compile(idMap);

    // Compile the shader export record
    shaderExport.CompileRecords(job);

    // Compile all functions
    if (!function.Compile(idMap)) {
        return false;
    }

    // OK
    return true;
}

void SpvPhysicalBlockTable::Stitch(SpvStream &out) {
    // Write all blocks
    scan.Stitch(out);
}

void SpvPhysicalBlockTable::CopyTo(SpvPhysicalBlockTable &out) {
    scan.CopyTo(out.scan);
    capability.CopyTo(out, out.capability);
    annotation.CopyTo(out, out.annotation);
    debugStringSource.CopyTo(out, out.debugStringSource);
    typeConstantVariable.CopyTo(out, out.typeConstantVariable);
    function.CopyTo(out, out.function);
    shaderExport.CopyTo(out, out.shaderExport);
}
