#pragma once

// Transform
#include <Transform/Image/Image.h>

namespace Transform {
    /// Get the luminance of a color
    template<typename T>
    float Luminance(const Eigen::Vector<T, 3>& color) {
        return 0.2126f * color(0) + 0.7152f * color(1) + 0.0722f * color(2);
    }

    /// Get the luminance of an image
    /// \param tensor image tensor
    /// \return luminance matrix
    Eigen::MatrixXf Luminance(const ImageTensor& tensor) {
        Eigen::DSizes<int64_t, 3> size = tensor.dimensions();

        // Destination matrix
        Eigen::MatrixXf matrix;
        matrix.resize(size[1], size[2]);

        // Compute luminance of each pixel
        for (uint32_t y = 0; y < size[2]; y++) {
            for (uint32_t x = 0; x < size[1]; x++) {
                matrix(x, y) = Luminance(Eigen::Vector<float, 3>(
                    tensor(0, x, y),
                    tensor(1, x, y),
                    tensor(2, x, y)
                ));
            }
        }

        // OK
        return matrix;
    }
}
