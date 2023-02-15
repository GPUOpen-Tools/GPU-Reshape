#pragma once

namespace Backend::IL {
    enum class TextureSampleMode {
        Default,
        DepthComparison,
        Projection,
        ProjectionDepthComparison,
        Unexposed,
    };
}
