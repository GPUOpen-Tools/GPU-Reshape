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

// Common
#include <Common/IComponent.h>
#include <Common/ComRef.h>

// Json
#include <nlohmann/json.hpp>

// Forward declarations
class ITestPass;
class TestContainer;

class Parser : public IComponent {
public:
    /// Constructor
    Parser();

    /// Parse a json object
    /// \param json given object
    /// \param out destination test pass
    /// \return success state
    bool Parse(const nlohmann::json& json, ComRef<ITestPass>& out);

private:
    /// Parse an application
    bool ParseApplication(const nlohmann::json& json, ComRef<ITestPass>& out);

private:
    /// Parse a specific pass
    bool ParsePass(const nlohmann::json& json, ComRef<ITestPass>& out);

private:
    /// Parse a wait for screenshot pass
    bool ParseWaitForScreenshot(const nlohmann::json& json, ComRef<ITestPass>& out);

    /// Parse a key pass
    bool ParseKey(const nlohmann::json& json, ComRef<ITestPass>& out);

    /// Parse a wait pass
    bool ParseWait(const nlohmann::json& json, ComRef<ITestPass>& out);

    /// Parse a click pass
    bool ParseClick(const nlohmann::json& json, ComRef<ITestPass>& out);

private:
    /// Get the key for a given string
    bool GetKeyFor(std::string_view key, struct KeyInfo& out);
};
