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

// Backend
#include <Backend/IL/Analysis/IAnalysis.h>

// Common
#include <Common/Registry.h>

namespace IL {
    template<typename T>
    class AnalysisMap {
    public:
        AnalysisMap() = default;

        /// Analysis maps are never inherited
        /// A slightly strange place to put it, however, it allows for simplified move constructors of targets
        AnalysisMap(AnalysisMap&&) noexcept {

        }

        /// Find a pass or construct and compute it if it doesn't exist
        /// \param args all construction arguments
        /// \return nullptr if failed to create
        template<typename U, typename... AX>
        ComRef<U> FindPassOrCompute(AX&&... args) {
            static_assert(std::is_base_of_v<T, U>, "Invalid analysis target");

            // Already exists?
            if (ComRef<U> analysis = registry.Get<U>()) {
                return std::move(analysis);
            }

            // Create a new one
            ComRef<U> analysis = registry.AddNew<U>(std::forward<AX>(args)...);

            // Compute the pass
            if (!analysis->Compute()) {
                return nullptr;
            }

            // OK
            return analysis;
        }

        /// Find an existing pass
        /// \return nullptr if not found
        template<typename U>
        ComRef<U> FindPass() {
            static_assert(std::is_base_of_v<T, U>, "Invalid analysis target");
            return registry.Get<U>();
        }

        /// Remove an existing pass
        /// \param analysis pass to remove
        template<typename U>
        void Remove(const ComRef<U>& analysis) {
            registry.Remove(analysis);
        }

    private:
        /// Internal registry
        Registry registry;
    };
}
