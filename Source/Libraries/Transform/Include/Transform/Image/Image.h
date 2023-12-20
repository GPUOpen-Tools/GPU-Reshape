#pragma once

// Transform
#include <Transform/Eigen.h>

namespace Transform {
    /// Standard image tensor, stored as channels - width - height
    using ImageTensor = Eigen::Tensor<float, 3>;
}
