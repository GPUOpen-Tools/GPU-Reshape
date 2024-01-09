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

// Catch2
#include <catch2/catch.hpp>

// D3D12
#include <dxgi.h>

// Std
#include <chrono>

/// Unwrapping GUID
static const GUID IID_Unwrap = { 0xd3cd71b6, 0x5e41, 0x4a9c, { 0xbb, 0x4, 0x7d, 0x8e, 0xf2, 0x7c, 0xfb, 0x57 } };

/// Base object
class Object {
public:
    float data = 0.5f;
};

/// Generic wrapper, such as a resource
class Wrapper {
public:
    Wrapper(Object* next) : next(next) {

    }

    Object* next{nullptr};

    virtual HRESULT QueryInterface(const IID &riid, void **ppvObject) {
        if (riid == IID_Unwrap) {
            *ppvObject = next;
            return S_OK;
        }

        return S_FALSE;
    }
};

/// Generic sink for backend, such as a command list
class ISink {
public:
    virtual void Sink(Object* signature, uint32_t iterations) {
        float seqAccum = 0;

        for (uint32_t i = 0; i < iterations; i++) {
            seqAccum += signature->data + seqAccum * signature->data;
        }

        instructionsArePrecious += seqAccum;
    }

private:
    float instructionsArePrecious = 0.0f;
};

/// Generic wrapper for a sink
class SinkWrapper {
public:
    SinkWrapper(ISink* next) : next(next) {

    }

    ISink* next{nullptr};

    virtual void Sink(Wrapper* wrapper, uint32_t iterations) {
        // Query base type
        Object* object{};
        wrapper->QueryInterface(IID_Unwrap, reinterpret_cast<void**>(&object));

        // Next
        return next->Sink(object, iterations);
    }
};

/// Ad-hoc preventing of devirtualization
template<typename T>
T* NoOpt(T* opt) {
    static volatile T* value;
    value = opt;
    return const_cast<T*>(value);
}

TEST_CASE("WrappingBenchmark") {
    Object object;

    auto sink        = NoOpt(new ISink);
    auto wrapper     = NoOpt(new Wrapper(&object));
    auto sinkWrapper = NoOpt(new SinkWrapper(sink));

    BENCHMARK("LowIterations.Baseline_Or_VTable") {
        sink->Sink(&object, 10);
    };

    BENCHMARK("LowIterations.Wrapper") {
        sinkWrapper->Sink(wrapper, 10);
    };

    BENCHMARK("HighIterations.Baseline_Or_VTable") {
        sink->Sink(&object, 100);
    };

    BENCHMARK("HighIterations.Wrapper") {
        sinkWrapper->Sink(wrapper, 100);
    };
}
