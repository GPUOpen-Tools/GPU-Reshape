/*
	Pass: Resource Initialization
*/

#include "ResourceInitializationSharedData.h"

[[vk::push_constant]] ResourceInitializationSRMaskWriteData pc_Data;
[[vk::binding(0, 0)]] RWBuffer<uint> g_ResourceSRMaskBuffer;

[numthreads(1, 1, 1)]
void main()
{
	InterlockedOr(g_ResourceSRMaskBuffer[pc_Data.m_RID], pc_Data.m_SRMask);
}