#pragma once

// Transform
#include <Transform/Image/Image.h>
#include <Transform/Image/Filter.h>
#include <Transform/Image/Color.h>

namespace Transform {
    /// Perform an average hash of an image
    /// \param tensor input image
    /// \return hash bit set
    uint64_t AverageHash(const ImageTensor& tensor) {
        // Reduce the image to 8x8 -> 64 pixels
        ImageTensor reducedTensor = ReduceAverage(tensor, Eigen::DSizes<int64_t, 3>(3, 8, 8));

        // Get luminance of all pixels
        Eigen::MatrixXf luminance = Luminance(reducedTensor);

        // Mean luminance
        float average = luminance.mean();

        // Hash, order invariant, bit set if luminance is greater than
        // the mean
        uint64_t hash = 0x0;
        for (uint32_t i = 0; i < 64; i++) {
            hash |= (luminance.data()[i] > average) << i;
        }

        // OK
        return hash;
    }
}
