#pragma once

// Common
#include <Common/Allocators.h>

/// Common allocation tags
static constexpr AllocationTag kAllocModule = "Module"_AllocTag;
static constexpr AllocationTag kAllocModuleDXStream = "Module.DXStream"_AllocTag;
static constexpr AllocationTag kAllocModuleDXBC = "Module.DXBC"_AllocTag;
static constexpr AllocationTag kAllocModuleDXIL = "Module.DXIL"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILTypeMap = "Module.DXIL.TypeMap"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILRecOps = "Module.DXIL.RecOps"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILIDMap = "Module.DXIL.IDMap"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILIDRemapper = "Module.DXIL.IDRemapper"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILConstants = "Module.DXIL.Constants"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILLLVMBlock = "Module.DXIL.LLVM.Block"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILLLVMBlockMetadata = "Module.DXIL.LLVM.BlockMetadata"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILLLVMBlockBlocks = "Module.DXIL.LLVM.Block.Blocks"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILLLVMBlockRecords = "Module.DXIL.LLVM.Block.Records"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILLLVMBlockAbbreviations = "Module.DXIL.LLVM.Block.Abbreviations"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILLLVMBlockElements = "Module.DXIL.LLVM.Block.Elements"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILSymbols = "Module.DXIL.Symbols"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILMetadata = "Module.DXIL.Metadata"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILFunctionAttributes = "Module.DXIL.FunctionAttributes"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILDebug = "Module.DXIL.Debug"_AllocTag;
static constexpr AllocationTag kAllocModuleILProgram = "Module.IL.Program"_AllocTag;
