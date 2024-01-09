// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
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
