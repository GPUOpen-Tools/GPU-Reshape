/*
	Pass: Resource Initialization
*/
#pragma once

#ifdef __cplusplus
#	define uint uint32_t
#endif

struct ResourceInitializationSRMaskWriteData
{
    uint m_RID;
    uint m_SRMask;
};

struct ResourceInitializationSRMaskFreeData
{
    uint m_RID;
};

#ifdef __cplusplus
#	undef uint
#endif
