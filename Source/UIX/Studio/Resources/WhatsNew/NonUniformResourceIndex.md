## NonUniformResourceIndex

Additionally to identifying code sections that may lead to a waterfall loop, GPU Reshape's AMD|Waterfall feature also validates the correct use of the `NonUniformResourceIndex` intrinsic. Per [the HLSL Dynamic Resources specification](https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_DynamicResources.html), an index is uniform when it is used to index into an array of resources or into a descriptor heap in case of a bindless renderer. If the index is non-uniform, it has to be annotated with the `NonUniformResourceIndex` intrinsic. Failing to do so can lead to undefined behavior on some hardware.

```hlsl
StructuredBuffer<PerInstanceData> instanceData : register(t0, space0);
Texture2D materialTextures[] : register(t0, space1);

float4 PSmain(PSInput input, uint instanceID : SV_InstanceID) : SV_TARGET 
{ 
    ...
    // instance ID can be non-uniform in a wave
    uint materialID = instanceData[instanceID].materialID;
    // we need to add the NonUniformResourceIndex flag to the actual resource indexing
    float4 value = materialTextures[NonUniformResourceIndex(materialID)].Sample(s_LinearClamp, input.UV);
    ...
}
```

Note: This code snippet is specification-compliant.

GPU Reshape analyzes the index that was used to access the resource during runtime on live shader code. This means that GPU Reshape is able to detect if a wave accesses the array with a uniform index or with a non-uniform index. If the index was non-uniform, but the `NonUniformResourceIndex` intrinsic is missing, GPU Reshape will send an error message:

![divergent-resource-addressing](avares://GPUReshape/Resources/WhatsNew/Images/divergent-resource-addressing.png)

If you want to read more on the topic about non-uniform resource access, you can check out the [Porting Detroit 2](https://gpuopen.com/learn/porting-detroit-2/) blogpost.

If you want to know more about the topic of occupancy, I recommend the [Occupancy Explained](https://gpuopen.com/learn/occupancy-explained/) blogpost.
