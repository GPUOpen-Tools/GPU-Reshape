#pragma once

// Transform
#include <Transform/Image/Image.h>

namespace Transform {
    /// Read an image file
    /// \param path File path
    /// \return Empty if not found
    ImageTensor ReadImage(const char* path);

    /// Write an image file
    /// \param path File path, type deduced from the extension
    /// \param tensor Image to be written
    /// \return Success state
    bool WriteImage(const char* path, const ImageTensor& tensor);

    /// Write an image file
    /// \param path File path, type deduced from the extension
    /// \param matrix Image to be written, assumes grayscale
    /// \return Success state
    bool WriteImage(const char* path, const Eigen::MatrixXf& matrix);
}
