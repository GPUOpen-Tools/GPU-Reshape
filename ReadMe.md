<img style="float: left;" width="175" src="/Source/UIX/Studio/Resources/Icons/Icon_Frame.png">

# GPU Reshape

An open collaboration between **Miguel Petersen** (author), **Advanced Micro Devices** and **Avalanche Studios Group**.

---

[Build](Documentation/Build.md) -
[UIX](Documentation/UIX.md) -
[API](Documentation/API.md) -
[Motivation](Documentation/Motivation.md) -
[Design](Documentation/Design.md) -
[Features](Documentation/Features.md) -
[Proof of concept](Avalanche/ReadMe.md)

---

<p float="left">
  <img src="/Documentation/Resources/Images/StudioC.png" width="450" /> 
  <img src="/Documentation/Resources/Images/StudioB.png" width="450" />
</p>

**GPU Reshape** offers API agnostic instrumentation of GPU side operations to perform, e.g., validation of potentially undefined behaviour, supporting both Vulkan and D3D12 (DXIL).
No application side integration is required.

Current feature scope provides instrumentation on operations which are either undefined behaviour, or typically indicative of user fault such as:

- Validation on out-of-bounds resource reads / writes
- Validation on out-of-bounds descriptor reads, and descriptor compatability testing
- Validation of floating point exports (fragment, unordered access view) against NaN / Inf
- Detection of potentially hazardous concurrent resource read / writes
- Validation of resource initialization during reads

The future feature scope includes profiling and debugging functionality, such as branch hot spots, live breakpoints, and assertions. 
For the full planned feature set, see [Features](Documentation/Features.md).

## Instrumentation as a framework

The toolset provides a generalized SSA-based intermediate language from which all instrumentation is done, bi-directionally translated to SPIRV and DXIL.
Each feature, such as the validation of out-of-bounds reads, operates solely on the intermediate language and has no visibility on the backend language nor API.

Each feature can alter the program as it sees fit, such as adding basic-blocks, modifying instructions, and even removing instructions. The feature is given a [Program](Documentation/API/IL.md), which
act as the abstraction for the active backend, from which the user has access to all functions, basic-blocks, instructions, types, etc..., and is able to modify as necessary.
After modification, the backend then performs just-in-time recompilation of the modified program back to the backend language.

The toolset additionally provides a set of building blocks needed for instrumentation:

- [Message Streams](Documentation/API/Message.md) help facilitate GPU -> CPU communication, and also serves as the base for inter-process/endpoint communication.
- Persistent data such as buffers, textures and push / root constants, visible to all instrumentation features. Certain features require state management.
- User programs, entirely user driven compute kernels written through the intermediate language. All persistent data visible.
- User side command hooking, certain features may wish to modify state, invoke kernels, before the pending command.
- Resource tokens, abstracting away differences in binding models by providing a token from shader resource handles. Each token provides a physical UID, resource type and sub-resource base.
- (*, there's more, need to expand, and write documentation...)

Features do not need to concern themselves with backend specifics, such as vectorized versus scalarized execution, control-flow requirements, and other implementation details. Given compliance, each feature
will translate seamlessly to the backend language.

This toolset aims to serve as a **framework** for instrumentation, acting as a modular base from which any number of tools, techniques, optimizations, etc..., can be implemented. 

## Credit

GPU Reshape was initially developed as a prototype tool by Miguel Petersen at Avalanche Studios Group, extending validation tools to shader side operations.
It was then requested to continue development externally through an open collaboration on GPUOpen.

Development is lead by Miguel Petersen, with support by: (readme todo, support? sounds vague, collaborators? need to figure out the terminology for proper crediting)

- Lou Kramer (AMD)
- Jonas Gustavsson (AMD)
- Daniel Isheden (Avalanche Studios)
- Alexander Polya (Avalanche Studios)
- Wiliam Hjelm (Avalanche Studios)

Copyright © 2023 Miguel Petersen
</br>
Copyright © 2023 Advanced Micro Devices
</br>
Copyright © 2023 Avalanche Studios Group
