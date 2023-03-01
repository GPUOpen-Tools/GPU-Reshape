#pragma once

// Common
#include <Common/Allocators.h>

/// Common allocation tags
static constexpr AllocationTag kAllocModule = "Module"_AllocTag;
static constexpr AllocationTag kAllocModuleDXIL = "Module.DXIL"_AllocTag;
static constexpr AllocationTag kAllocModuleILProgram = "Module.IL.Program"_AllocTag;
static constexpr AllocationTag kAllocModuleLLVMBlock = "Module.LLVM.Block"_AllocTag;
static constexpr AllocationTag kAllocModuleLLVMBlockElements = "Module.LLVM.Block.Elements"_AllocTag;
static constexpr AllocationTag kAllocModuleLLVMRecordAllocator = "Module.LLVM.RecordAllocator"_AllocTag;
static constexpr AllocationTag kAllocModuleLLVMMetadata = "Module.LLVM.Metadata"_AllocTag;
