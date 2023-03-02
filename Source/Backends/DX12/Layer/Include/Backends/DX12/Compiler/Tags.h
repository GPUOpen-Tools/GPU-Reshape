#pragma once

// Common
#include <Common/Allocators.h>

/// Common allocation tags
static constexpr AllocationTag kAllocModule = "DX12.Module"_AllocTag;
static constexpr AllocationTag kAllocModuleDXStream = "DX12.Module.DXStream"_AllocTag;
static constexpr AllocationTag kAllocModuleDXBC = "DX12.Module.DXBC"_AllocTag;
static constexpr AllocationTag kAllocModuleDXIL = "DX12.Module.DXIL"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILTypeMap = "DX12.Module.DXIL.TypeMap"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILRecOps = "DX12.Module.DXIL.RecOps"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILIDMap = "DX12.Module.DXIL.IDMap"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILIDRemapper = "DX12.Module.DXIL.IDRemapper"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILConstants = "DX12.Module.DXIL.Constants"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILLLVMBlock = "DX12.Module.DXIL.LLVM.Block"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILLLVMBlockMetadata = "DX12.Module.DXIL.LLVM.BlockMetadata"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILLLVMBlockBlocks = "DX12.Module.DXIL.LLVM.Block.Blocks"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILLLVMBlockRecords = "DX12.Module.DXIL.LLVM.Block.Records"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILLLVMBlockAbbreviations = "DX12.Module.DXIL.LLVM.Block.Abbreviations"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILLLVMBlockElements = "DX12.Module.DXIL.LLVM.Block.Elements"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILSymbols = "DX12.Module.DXIL.Symbols"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILMetadata = "DX12.Module.DXIL.Metadata"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILFunctionAttributes = "DX12.Module.DXIL.FunctionAttributes"_AllocTag;
static constexpr AllocationTag kAllocModuleDXILDebug = "DX12.Module.DXIL.Debug"_AllocTag;
static constexpr AllocationTag kAllocModuleILProgram = "DX12.Module.IL.Program"_AllocTag;
