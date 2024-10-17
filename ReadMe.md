<h1><img align="center" height="75" src="/Source/UIX/Studio/Resources/Icons/Icon_Frame.png"> <a>GPU Reshape</a></h1>

**GPU Reshape** offers API agnostic instrumentation of GPU side operations to perform, e.g., validation of potentially undefined behaviour, supporting both Vulkan and D3D12 (DXIL).
No application side integration is required.

GPU Reshape is an open collaboration between **Miguel Petersen** (author), **Advanced Micro Devices** and **Avalanche Studios Group**.

![Cover.png](Documentation/Resources/Images/Cover.png)

---

<p align="center">
  <a href="Documentation/QuickStart.md">Quick Start</a> -
  <a href="Documentation/Features.md">Features</a> -
  <a href="Documentation/Build.md">Build</a> -
  <a href="Documentation/API.md">API</a> -
  <a href="Documentation/Motivation.md">Motivation</a> -
  <a href="Documentation/UIX.md">UIX</a>
</p>
<p align="center">
  Main is releases only, for the latest changes see the <a href="https://github.com/GPUOpen-Tools/GPU-Reshape/tree/development">development branch</a>.
</p>

---

Current feature scope provides instrumentation on operations which are either undefined behaviour, or typically indicative of user fault such as:

- **AMD|Waterfall** </br> Validation of AMD address scalarization / waterfalling, and detection of missing NonUniformResourceIndex on descriptor indexing.
- **Resource Bounds** </br> Validation of resource read / write coordinates against its bounds.
- **Export Stability** </br> Numeric stability validation of floating point exports (UAV writes, render targets, vertex exports), e.g. NaN / Inf.
- **Descriptor Validation** </br> Validation of descriptors, potentially dynamically indexed. This includes undefined, mismatched (compile-time to runtime), out of bounds descriptor indexing, and missing table bindings.
- **Concurrency Validation** </br> Validation of resource concurrency, i.e. single-producer or multiple-consumer, between queues and events, now on a per-texel/byte level.
- **Resource Initialization** </br> Validation of resource initialization, ensures any read was preceded by a write, now on a per-texel/byte level.
- **Infinite Loops** </br> Detection and guarding of infinite loops.

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
- Command scheduling, record and submit custom (abstracted) command buffers / lists.

Features do not need to concern themselves with backend specifics, such as vectorized versus scalarized execution, control-flow requirements, and other implementation details. Given compliance, each feature
will translate seamlessly to the backend language.

This toolset aims to serve as a **framework** for instrumentation, acting as a modular base from which any number of tools, techniques, optimizations, etc..., can be implemented. 

## Testing Suite

GPU Reshape is routinely tested against a standard testbed, including AAA games, IHV/ISV samples/benchmarks, and user
submitted applications. For a full list, please see the [Testing Suite](Documentation/TestingSuite.md).

## Known Issues

- Raytracing shaders are currently pass-through, no instrumentation is done on them. This is scheduled for the next release.

DX12 features pending support:
- Work Graphs

Vulkan extensions pending support:
- Descriptor Buffers
- Shader Objects

## Credit

GPU Reshape was initially developed as a prototype tool by Miguel Petersen at Avalanche Studios Group, extending validation tools to shader side operations.
It was then requested to continue development externally through an open collaboration on GPUOpen.

Development was supported by:

- Lou Kramer (AMD)
- Jonas Gustavsson (AMD)
- Marek Machlinski (AMD)
- Rys Sommefeldt (AMD)
- Mark Simpson (AMD)
- Daniel Isheden (Avalanche Studios)
- Alexander Polya (Avalanche Studios)
- Wiliam Hjelm (Avalanche Studios)

Copyright (c) 2024 Advanced Micro Devices, Inc., 
Fatalist Development AB (Avalanche Studio Group), 
and Miguel Petersen.

All Rights Reserved.
