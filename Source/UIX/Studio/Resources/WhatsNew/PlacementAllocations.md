## Placement Allocations

The initialization feature received another upgrade in this release. It now correctly verifies if placed resources have been initialized correctly. The caveat with placed resources is, that by specification a write access within a shader is not a valid initialization if the resource is a render target or a depth stencil target. (See [ID3D12Device::CreatePlacedResource](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-createplacedresource#notes-on-the-required-resource-initialization)).

It's important to correctly initialize placed resources even if you are going to overwrite all its pixels as a render target or UAV, because the metadata could contain garbage data. Visual artifacts could survive such an overwrite. This is especially relevant in regards to DCC compressed resources. Failing to initialize a resource correctly can lead to undefined behavior, such as corruptions or GPU hangs.

![uninitialized-placed-resource](avares://GPUReshape/Resources/WhatsNew/Images/uninitialized-placed-resource.png)
