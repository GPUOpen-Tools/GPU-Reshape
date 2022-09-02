//! KERNEL   Compute "main"
//! DISPATCH 4, 1, 1

//! SCHEMA "Schemas/Features/ExportStability.h"

//! RESOURCE RWBuffer<R32Float> size:64
[[vk::binding(0)]] RWBuffer<float> bufferRW : register(u0, space0);

[numthreads(1, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
    float inf = 1.#INF;
    float nan = sqrt(-1.0f);

    //! MESSAGE UnstableExport[4]
	bufferRW[dtid.x] = nan;

    //! MESSAGE UnstableExport[3]
	bufferRW[dtid.x] = dtid == 0 ? 0.0f : nan;

    //! MESSAGE UnstableExport[3]
	bufferRW[dtid.x] = dtid == 0 ? 0.0f : inf;
}
