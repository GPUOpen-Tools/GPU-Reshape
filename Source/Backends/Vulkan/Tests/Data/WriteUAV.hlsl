
[[vk::binding(0)]]
RWBuffer<uint> output;

[numthreads(1, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
	output[dtid.x] = dtid;
}
