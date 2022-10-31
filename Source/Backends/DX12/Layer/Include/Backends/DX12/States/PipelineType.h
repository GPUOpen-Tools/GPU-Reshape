#pragma once

#include <Common/Enum.h>

enum class PipelineType {
    None = 0,
    Graphics = BIT(1),
    Compute = BIT(2),
    GraphicsSlot = 0,
    ComputeSlot = 1,
    Count = 2
};

BIT_SET(PipelineType);
