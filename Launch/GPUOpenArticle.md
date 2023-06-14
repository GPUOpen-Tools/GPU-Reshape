
# GPU Open - GPU Reshape - Article

---

Meet GPU Reshape, a toolset that leverages on-the-fly instrumentation of GPU operations 
with instruction level validation. Supporting DX12 & Vulkan, no integration required.

---

![Cover.png](Cover.png)

Now that I have your attention, my name is Miguel Petersen, Senior Rendering Engineer at Striking Distance Studios, author of GPU Reshape.
The toolset was developed in collaboration with AMD and Avalanche Studios Group, initially as a proof-of-concept Vulkan layer at Avalanche, after which
development was continued externally.

Development supported by Lou Kramer, Jonas Gustavsson, Marek Machlinski, Daniel Isheden, Alexander Polya, and Wiliam Hjelm. Thank you all.

### Making the GPU a First Class Citizen

Modern graphics units are powerful, but highly complex. Debugging issues, much less knowing where or even if there are issues,
remains a consistent challenge. While this does translate to job security, it can prove tedious and occupy significant development times.

GPU Reshape aims to change this, by bringing powerful features typical of CPU tooling to the GPU, such as:
- **Resource Bounds** </br> Validation of resource read / write coordinates against its bounds.
- **Export Stability** </br> Numeric stability validation of floating point exports (unordered writes, render targets) 
- **Descriptor Validation** </br> Validation of descriptors, potentially dynamically indexed. This includes undefined, mismatched (compile-time to runtime), and out of bounds descriptor indexing.
- **Concurrency Validation** </br> Validation of resource concurrency, i.e. single-producer or multiple-consumer, between queues and events.
- **Resource Initialization** </br> Validation of resource initialization, ensures any read was preceded by a write.
- **Infinite Loops** </br> Detection of infinite loops.

Validation errors are reported on the exact line of code of the instruction, with, for example, the resource, dimensions, and coordinates
accessed. Letting the developer know exactly what went wrong, and where it is to be addressed. Additionally, certain features, such as
descriptor validation, can safeguard potentially erroneous operation, preventing undefined behaviour during instrumentation.

![FirstClassSectionStart.png](FirstClassSectionStart.png)

Future feature scopes includes profiling and debugging functionality, such as branch hot spots, live breakpoints, and assertions.
For further information, please see the public repository.

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
techniques, optimizations, etc..., can be implemented. It is my hope, with time, it matures and evolves into a general purpose
tool that can be of aid to the industry.

### Ending notes 

[[Miguel P: .. things to add here, can't end with the dry stuff above!]]

[[Miguel P: Do I add these here? Not sure, I don't know much about legalities]]

Copyright © 2023 Miguel Petersen </br>
Copyright © 2023 Advanced Micro Devices </br>
Copyright © 2023 Avalanche Studios Group