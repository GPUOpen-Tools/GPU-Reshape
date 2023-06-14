
# GPU Open Article

---

Meet GPU Reshape, a toolset that leverages on-the-fly instrumentation of GPU operations 
with instruction level validation. Supporting DX12 & Vulkan, no integration required.

[[IMAGE]]

---

Now that I have your attention, my name is Miguel Petersen, Senior Rendering Engineer at Striking Distance Studios, author of GPU Reshape.
The toolset was developed in collaboration with AMD and Avalanche Studios Group, initially as a proof-of-concept Vulkan layer at Avalanche, whose
development was continued externally.

### The GPU as a First Class Citizen

Modern graphics units are powerful, but highly complex. Debugging issues, much less knowing where or even if there are issues,
remains a consistent challenge. While this does translate to job security, it can prove tedious and occupy significant develoment times.

GPU Reshape aims to bring features of CPU tooling to the GPU, with powerful features such as:
- Resource Bounds
- Numeric Stability on Exports
- Descriptor Validity
- Resource Initialization
- Infinite Loops

[[IMAGE]]

Validation errors are reported on the exact line of code of the instruction, with, for example, the resource, dimensions, and coordinates
accessed. Letting the developer know exactly what went wrong, and where it is to be addressed.

The future feature scope includes profiling and debugging functionality, such as branch hot spots, live breakpoints, and assertions.

### Instrumentation as a Framework

GPU Reshape, at its core, is a modular API-agnostic instrumentation framework. 
The toolset provides a generalized SSA-based intermediate language from which all instrumentation is done, 
bi-directionally translated to the backend language (namely SPIRV and DXIL). Each feature, such as the validation of out-of-bounds reads, 
operates solely on the intermediate language and has no visibility on the backend language nor API.

Each feature can alter the program as it sees fit, such as adding basic-blocks, modifying instructions, 
and even removing instructions. The feature is given a Program, which act as the abstraction for the active backend, 
from which the user has access to all functions, basic-blocks, instructions, types, etc..., and is able to modify as 
necessary. After modification, the backend then performs just-in-time recompilation of the modified program back to 
the backend language.

Features do not need to concern themselves with backend specifics, such as vectorized versus scalarized execution, 
control-flow requirements, and other implementation details. Given compliance, each feature will translate seamlessly 
to the backend language.

GPU Reshape aims to serve as a framework for instrumentation, acting as a modular base from which any number of tools, 
techniques, optimizations, etc..., can be implemented.
