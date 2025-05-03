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

// Backend
#include <Backend/Device/DeviceStateVote.h>

template<typename T>
struct DeviceStateRef {
    DeviceStateRef() = default;

    /// Constructor
    /// @param stateVote the voter interface
    /// @param value the value to vote for
    DeviceStateRef(IDeviceStateVote* stateVote, const T& value) : stateVote{stateVote}, value(value) {
        uid = stateVote->AllocateUID();
        stateVote->Register(uid, value);
    }

    /// Destructor
    ~DeviceStateRef() {
        if (stateVote) {
            stateVote->Deregister<T>(uid);
        }
    }

    /// Copy constructor
    DeviceStateRef(const DeviceStateRef& other) {
        *this = other;
    }

    /// Move constructor
    DeviceStateRef(DeviceStateRef&& other) {
        *this = std::move(other);
    }

    /// Copy assignment
    DeviceStateRef& operator=(const DeviceStateRef& other) {
        if (stateVote) {
            stateVote->Deregister<T>(uid);
        }
        
        stateVote = other.stateVote;
        value = other.value;

        // Register new user
        if (stateVote) {
            uid = stateVote->AllocateUID();
            stateVote->Register(uid, value);
        }
        
        return *this;
    }

    /// Move assignment
    DeviceStateRef& operator=(DeviceStateRef&& other) {
        if (stateVote) {
            stateVote->Deregister<T>(uid);
        }
        
        stateVote = other.stateVote;
        uid = other.uid;
        value = other.value;

        // Reset remote
        other.stateVote = nullptr;
        return *this;
    }

    /// Is this reference set?
    bool IsSet() const {
        return stateVote != nullptr;
    }

private:
    /// The voting interface
    IDeviceStateVote* stateVote{nullptr};

    /// Assigned UID
    uint64_t uid = 0;

    /// Value we're voting for
    T value{};
};
