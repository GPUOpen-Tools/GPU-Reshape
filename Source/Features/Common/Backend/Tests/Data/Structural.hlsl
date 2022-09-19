//! KERNEL   Compute "main"
//! DISPATCH 1, 1, 1

/**
 * Tests structural instructions
 */

//! RESOURCE RWBuffer<R32Float> size:64
[[vk::binding(0)]] RWBuffer<float> bufferRW;

struct Foo {
    float a;
    float b;
    float c;
};

[numthreads(64, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID) {
    Foo foo;
    foo.a = dtid.x;
    foo.b = dtid.y;
    foo.c = dtid.z;

    float3 fooVec = float3(foo.a, foo.b, foo.c);

    foo.a = fooVec.x;
    foo.a = (fooVec.xxx).x;
    foo.b = fooVec.x;

    bufferRW[dtid.x] += foo.a;
    bufferRW[dtid.x] += foo.b;
    bufferRW[dtid.x] += foo.c;
}
