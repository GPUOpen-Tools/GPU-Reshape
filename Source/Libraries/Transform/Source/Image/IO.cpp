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

#include <Transform/Image/IO.h>

// Common
#include <Common/String.h>
#include <Common/Assert.h>

// STB
#include <stb_image.h>
#include <stb_image_write.h>

// Std
#include <filesystem>

Transform::ImageTensor Transform::ReadImage(const char* path) {
    // Try to load the image
    int width, height, dimensions;
    unsigned char *data = stbi_load(path, &width, &height, &dimensions, 0);

    // Failed?
    if (!data) {
        return {};
    }

    // Create tensor
    Eigen::Tensor<float, 3> tensor;
    tensor.resize(dimensions, width, height);

    uint64_t dwordOffset = 0;

    // Write all pixels to tensor
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            for (int c = 0; c < dimensions; c++) {
                tensor(c, x, y) = static_cast<float>(data[dwordOffset++]) / 255.0f;
            }
        }
    }

    // Cleanup
    stbi_image_free(data);
    return tensor;
}

bool Transform::WriteImage(const char* path, const ImageTensor& tensor) {
    Eigen::DSizes<int64_t, 3> size = tensor.dimensions();

    // Pixel data
    std::vector<uint8_t> data;
    data.resize(size[0] * size[1] * size[2]);

    uint64_t dwordOffset = 0;

    // Write all pixels as unorm
    for (int y = 0; y < size[2]; y++) {
        for (int x = 0; x < size[1]; x++) {
            for (int c = 0; c < size[0]; c++) {
                float value = tensor(c, x, y);
                data[dwordOffset++] = static_cast<uint8_t>(std::min(1.0f, std::max(0.0f, value)) * 255.f);
            }
        }
    }

    ASSERT(dwordOffset == data.size(), "Unexpected offset");

    // Get extension
    std::string extension = std::filesystem::path(path).extension().string();

    // Write image to disk
    if (std::iequals(extension, ".jpg")) {
        stbi_write_jpg(path, static_cast<int32_t>(size[1]), static_cast<int32_t>(size[2]), static_cast<int32_t>(size[0]), data.data(), 100);
    } else if (std::iequals(extension, ".png")) {
        stbi_write_png(path, static_cast<int32_t>(size[1]), static_cast<int32_t>(size[2]), static_cast<int32_t>(size[0]), data.data(), static_cast<int32_t>(size[0] * size[1]));
    } else if (std::iequals(extension, ".bmp")) {
        stbi_write_bmp(path, static_cast<int32_t>(size[1]), static_cast<int32_t>(size[2]), static_cast<int32_t>(size[0]), data.data());
    } else if (std::iequals(extension, ".tga")) {
        stbi_write_tga(path, static_cast<int32_t>(size[1]), static_cast<int32_t>(size[2]), static_cast<int32_t>(size[0]), data.data());
    } else {
        // Unsupported file type
        return false;
    }

    // OK
    return true;
}

bool Transform::WriteImage(const char* path, const Eigen::MatrixXf& matrix) {
    // Map as image tensor, assume grayscale
    return WriteImage(path, Eigen::TensorMap<const ImageTensor>(
        matrix.data(),
        matrix.rows(),
        matrix.cols(),
        1
    ));
}
