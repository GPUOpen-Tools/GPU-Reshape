## The AMD|Waterfall Feature

The new AMD|Waterfall feature is one of the main additions to the Beta 2 release. The feature is two-fold. It includes an AMD-specific analysis part that highlights a potential performance intensive code-section, and a general part that validates if the `NonUniformResourceIndex` intrinsic is used where required.

What does waterfall in this context mean? GPU Reshape gives the following explanation:

![waterfall-feature-with-code](avares://GPUReshape/Resources/WhatsNew/Images/waterfall-feature-with-code.png)

As indicated, waterfall loops can occur if registers cannot be dynamically indexed. It's a way to scalarize a non-uniform variable.
A non-uniform variable - also called a varying - is stored in a wave-wide vector register (VGPR) where every thread has its own potentially unique value.

| | thread 0 | thread 1 | thread 2 | thread 3 |
|-|----------|----------|----------|----------|
|VGPR variable| 4 | 10 | 4 | 6 |

If a variable is uniform, every thread in a wave holds the same value.

| | thread 0 | thread 1 | thread 2 | thread 3 |
|-|----------|----------|----------|----------|
|VGPR variable| 4 | 4 | 4 | 4 |

As every thread holds the same value, it's a bit wasteful to use a VGPR to hold a copy of the same value for every thread. It's enough to just store it once. Modern AMD GPUs take advantage of that and so each workgroup processor (WGP), which is the part of the architecture which executes shader programs, has a separate scalar ALU which comes with its own scalar registers (SGPRs).

| | wave (all threads) |
|-|--------------------|
|SGPR variable| 4 |

Scalarizing a uniform variable that is stored in a VGPR is straight forward - we can just pick the value of any thread and put it in a SGPR, e.g. via the HLSL wave operation [WaveReadLaneFirst](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/wavereadfirstlane).

However, the more interesting case is if a varying variable needs to be scalarized. Sometimes the compiler needs to do this as certain hardware operations require thread-level uniformity, but the higher level shading language allows the programmer to write their code with non-uniformity. Examples are indexing into resource arrays, indexing into a descriptor heap if we use a bindless rendering model, or dynamic indexing of any array of scalar values, which will be backed by VGPRs.

When scalarizing a non-uniform variable, we can't just pick a random thread and take its value as the other threads might have different values. A solution to this problem is called waterfalling. There are other solutions our compiler can do, which we won’t go into detail in this blogpost, but all of them incur a cost. A waterfall loop essentially loops over the non-uniform variable as many times as there are unique values. In pseudo-code, it would be something like this:

```hlsl
while (activeThreads)
{
    scalar = WaveReadLaneFirst(varyingVariable);
    if (varyingVariable == scalar) {
        doOperationThatRequiresScalar(scalar);
        break; // deactivate this thread
    }
}
// after the while loop, all threads are active again
```

In the above example, if you had 4 threads with 3 unique values, the while loop would have 3 iterations.

The more per-thread unique values a non-uniform variable holds, the more iterations are required with the worst case being the total number of active threads in the wave. This can be quite costly. GPU Reshape's AMD|Waterfall feature is intended to identify cases where waterfalling might happen, so that the developer can inspect the shader and determine if a waterfall loop has an impact on performance or not. If it has a performance impact, for example because the number of unique values of the non-uniform variable within a wave is typically very high, it might be a good idea to try techniques that do not require a waterfall loop or that reduce the number of required loop iterations.

Let's look at one concrete example that involves dynamic indexing into an array.

Arrays can be used to implement a stack.

```hlsl
uint stack[32];
uint currentStackIndex;
```

Each thread has its own stack which it pushes and pops nodes to. The content and current index of the stack can be different per thread, hence the current stack index is a non-uniform variable.

Since the stack size is 32 in this example, the maximum number of iterations for the waterfall loop is 32. Depending on the use case, a smaller stack size might be sufficient, but obviously you can't go down to 0.

An alternative to a stack based on a VGPR-backed array is a stack based on an array in groupshared memory, which AMD implements in a wave-wide shared memory called LDS.

```hlsl
groupshared uint lds_stack[32*32];
```

Note, that we assume a fixed wavesize of 32 in this example.

Using a LDS stack can be a bit counter-intuitive at first, since LDS access is slower than VGPR access. Also, threads do not need to exchange their stack entries between each other, so no efficient data transfer between threads is needed, which is what you would typically use LDS for.

However, dynamic access of an LDS array does not lead to a waterfall loop because AMD GPUs can address LDS individually per thread in the wave. Therefore, it's a trade-off between increased latency for read and writes to the stack and the latency introduced by a waterfall loop.

There is also an additional factor to consider in this example: a VGPR-backed stack might need to allocate a lot of VGPRs and thus limit the maximum occupancy of a wave. Offloading some of the VGPR pressure to LDS might help to improve occupancy which in turn can improve performance as well. This actually goes the other way too: A large LDS-backed stack can reduce the maximum occupancy, so it might make sense to offload some of that to a VGPR-backed stack.

Which backing store for the stack is now the recommended choice? As usual, the answer is "it depends". It could make sense to implement both stacks, or a combination of both, and measure the performance characteristics to determine which solution provides the best result. It's a balance between occupancy limits, latency to reads and writes to the stack and overhead introduced by a VGPR-backed stack due to waterfalling.

GPU Reshape's AMD|Waterfall feature is intended to identify these cases so that developers can experiment and find the best possible solution for their use-case.

![waterfall-feature-code-only](avares://GPUReshape/Resources/WhatsNew/Images/waterfall-feature-code-only.png)
