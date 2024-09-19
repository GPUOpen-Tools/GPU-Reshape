## What's New

We're excited to share the release of [GPU Reshape Beta 2](https://gpuopen.com/gpu-reshape/), hosting a wealth of additions and general improvements.
Including, but not limited to:

* **AMD|Waterfall Feature**` `  
  A brand new AMD-specific feature that reveals potential performance issues with regards to register indexing
* **NonUniformResourceIndex Validation**` `  
  Validation that descriptor indexing is uniform, or appropriately annotated with `NonUniformResourceIndex`
* **Per-Texel/Byte Addressing for Initialization and Concurrency**` `  
  Initialization and concurrency tracking now occurs on a per-texel/byte level for resources, greatly improving
  accuracy
* **Infinite Loops**` `  
  No longer experimental, and features additional safe guarding against TDRs
* **Multi Device/Process**` `  
  Hook multiple graphical devices or entire process trees with ease
* **Mesh Shader Support**` `  
  Full support for existing feature set against mesh shaders
* **And, of course, bug fixes**

Please see the left hand sections for more information.
