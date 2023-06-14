#pragma once

enum class RootParameterVisibility {
    Compute = 0,
    Vertex = 0,
    Hull,
    Domain,
    Geometry,
    Pixel,
    Count
};
