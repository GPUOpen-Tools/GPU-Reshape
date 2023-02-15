//! KERNEL   Compute "main"
//! DISPATCH 256, 1, 1
//! DISPATCH 256, 1, 1

//! SCHEMA "Schemas/Features/Concurrency.h"

//! RESOURCE RWBuffer<R32Float> size:64
[[vk::binding(0)]] RWBuffer<float> bufferRW : register(u0, space0);

//! RESOURCE RWBuffer<R32Float> size:64
[[vk::binding(1)]] RWBuffer<float> bufferRW2 : register(u1, space0);

[numthreads(1, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
    if (dtid.x >= 128) {
        //! MESSAGE ResourceRaceCondition[{x > 0 && x <= 128}] LUID:{x==720 || x==721}
    	bufferRW[dtid.x] = 1.0f;
    }

    //! MESSAGE ResourceRaceCondition[>0] LUID:{x==720 || x==721}
	bufferRW2[dtid.x] = 1.0f;
}
