#pragma once

// Backend
#include <Backend/IL/Metadata/KernelMetadata.h>

// Shared workgroup size
// TODO: Could target wave64
static constexpr IL::KernelWorkgroupSizeMetadata kKernelSize{ 32, 1, 1 };
