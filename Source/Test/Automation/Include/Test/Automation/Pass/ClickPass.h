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

// Automation
#include <Test/Automation/Pass/ITestPass.h>

class ClickPass : public ITestPass {
public:
    COMPONENT(ClickPass);

    /// Constructor
    /// \param localX relative (to window) x coordinate of click
    /// \param localY relative (to window) y coordinate of click
    /// \param normalized if true, coordinates are 0-1 normalized values
    ClickPass(float localX, float localY, bool normalized);

    /// Overrides
    bool Run() override;

private:
    /// Relative coordinates
    float localX;
    float localY;
    bool normalized;
};
