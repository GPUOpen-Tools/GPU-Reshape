#pragma once

float TransformValue(float x)
{
    return rsqrt(fmod(x, 59.0f));
}
