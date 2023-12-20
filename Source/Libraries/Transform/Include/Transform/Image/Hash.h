// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

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
