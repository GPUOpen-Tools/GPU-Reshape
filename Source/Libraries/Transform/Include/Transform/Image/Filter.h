#pragma once

// Transform
#include <Transform/Image/Image.h>

namespace Transform {
    /// Reduce an image size by averaging
    /// \param tensor image to reduce
    /// \param size destination size
    /// \return reduced image
    ImageTensor ReduceAverage(const ImageTensor& tensor, Eigen::DSizes<int64_t, 3> size) {
        Eigen::DSizes<int64_t, 3> tensorSize = tensor.dimensions();

        /** Note: This is a rather poor implementation, it will suffice for now until something better is needed */

        // All per pixel weights
        ImageTensor weights;
        weights.resize(size);
        weights.setZero();

        // Final reduced tensor
        ImageTensor reduced;
        reduced.resize(size);
        reduced.setZero();

        // Compute coordinate factor
        Eigen::Array3f factor = Eigen::Array3f(
            static_cast<float>(size[0]),
            static_cast<float>(size[1]),
            static_cast<float>(size[2])
        ) / Eigen::Array3f(
            static_cast<float>(tensorSize[0]),
            static_cast<float>(tensorSize[1]),
            static_cast<float>(tensorSize[2])
        );

        // Sum all pixels
        for (uint32_t y = 0; y < tensorSize[2]; y++) {
            for (uint32_t x = 0; x < tensorSize[1]; x++) {
                for (uint32_t c = 0; c < tensorSize[0]; c++) {
                    Eigen::Array3i dest = (Eigen::Array3f(static_cast<float>(c), static_cast<float>(x), static_cast<float>(y)) * factor).cast<int32_t>();
                    reduced(dest[0], dest[1], dest[2]) += tensor(c, x, y);
                    weights(dest[0], dest[1], dest[2]) += 1u;
                }
            }
        }

        // Average
        reduced /= weights;
        return reduced;
    }
}
