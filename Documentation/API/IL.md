# IL - Intermediate Language

The intermediate language featured in this project serves as an abstraction of the backend shader byte-code. The language encapsulates
as much as it deems necessary, such as relevant instruction operands, however does not expose all implementation details. Encapsulating
all details would be beyond the scope of this project, and would be too difficult to maintain.

---

Code quick start

- Implementation
    - [Library](../../Source/Libraries/Backend) </br>
    - [Emitter](../../Source/Libraries/Backend/Include/Backend/IL/Emitter.h)
- Examples
    - [Resource Bounds Injection](../../Source/Features/ResourceBounds/Backend/Source/Feature.cpp)

---

### Program Structure

The intermediate language loosely follows the LLVM model, and most importantly follows the **SSA** (single-static-assignment) model. A program
is split up into three containers, the master program, functions and basic blocks.

All structure and operations are bi-directional, and must preserve the original code structure unless exceptions are specified.

#### Program

The program is responsible for hosting the functions present in the shader, and shared system such as type maps and identifier mappings.
Types are shared across the program, and ensures no duplicate types are allocated for the given program.
Equally, the (SSA) identifier mappings ensures unique and valid SSA allocations for instructions, and hosts all users for a particular block, such as
branching instructions.

#### Function

Functions are responsible for hosting the function signature, basic blocks and, optionally, restructuring basic blocks for control flow.

#### BasicBlock

Basic blocks host all instructions present in the shader, including branching information. Basic blocks store all instruction data
contiguously and also allow for instruction references `IL::InstructionRef<>` during manipulation, this ensures fast iteration times and safe instrumentation.

Instructions can be added, removed, replaced as per any other container, however also features basic block splitting `IL::BasicBlock::Split`. That is, moving instructions post an iterator to another basic block.
Splitting ensures instruction mappings and branch users are updated accordingly, and updates all control flow dependent instructions such as phi instructions for moved instructions.

### Instructions

Instructions hold the abstraction for a particular operation present in the shader byte-code, and attempt to closely map relevant operands. However, it
is not the purpose of this project to create a full encapsulation of all instructions, therefore many are partially mapped to a common sub set.

As instructions must be bi-directional, translation back to the backend shader byte-code operate on a "template" of the original instruction. Exposed operands
are replaced to the abstractions value, all other operands are preserved. If no template is available, i.e., a new instruction, a common template is used.

All exposed operation codes can be found in the [OpCode enum](../../Source/Libraries/Backend/Include/Backend/IL/OpCode.h). It is not recommended to directly
instantiate and populate instructions, instead it is recommended to use emitters.

### Emitters

Emitters are your best friend in instruction instantiation and instrumentation. Emitters handle instruction initialization, SSA identifier allocation, populates type mappings,
and performs basic validation. The emitter mode `Append` | `Replace` | `Instrument` defines how instructions are inserted into the basic block.

- Append inserts the instruction after the given insertion point, defaults to end of block
- Replace replaces a given instruction
- Instrument replaces a given instruction, however preserves all metadata associated to ensure successful templating

### Type System

All exposed types are tracked and uniquely allocated, this is to ensure compliance with backend generators. All exposed types can be found in [the type declarations](../../Source/Libraries/Backend/Include/Backend/IL/Type.h),
which are used through [the type map](../../Source/Libraries/Backend/Include/Backend/IL/TypeMap.h) to retrieve unique handles.

### Control Flow

Some backends require the use of structured control flow, as such the abstraction must account for this. All conditional branching 
supply a [control flow declaration](../../Source/Libraries/Backend/Include/Backend/IL/ControlFlow.h), either as **selection** or **loop**.
- Selection control flow supplies a merge block, essentially the shared block to be branched to
- Loop control flow supplies both a merge and continue block. Merge block remains the shared block which is ultimately branched to,
  the continue block dictates which block branches back to the loop header.

#### SPIRV-Exceptions

Source loop continue blocks cannot host selection merges, this essentially means that the block cannot be safely instrumented. Loop continue
blocks may host instructions of interest that requires block segmentation, such as resource operations. To work around this issue, all loop source
loop continue blocks are patched through a proxy block. That is, the original continue block is demoted to the loop body, and then proxies to the
new loop continue block. All branch users, such as phi operations, are patched to ensure program validity. The alternative is to reject segmentation of 
continue blocks, which is unacceptable.
