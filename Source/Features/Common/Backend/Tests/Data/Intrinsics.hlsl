// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

//! KERNEL   Compute "main"
//! DISPATCH 1, 1, 1

/**
 * Tests common intrinsics
 */

//! RESOURCE RWBuffer<R32Float> size:64
[[vk::binding(0)]] RWBuffer<float> bufferRW;

[numthreads(64, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID) {
    float value = dtid.x;

    // Special
    bufferRW[dtid.x] += isnan(value);
    bufferRW[dtid.x] += isinf(value);
    bufferRW[dtid.x] += isfinite(value);
    bufferRW[dtid.x] += saturate(value);

    // Unary
    bufferRW[uint(cos(dtid.x) * 10.0f)] += cos(value);
    bufferRW[uint(sin(dtid.x) * 10.0f)] += sin(value);
    bufferRW[uint(tan(dtid.x) * 10.0f)] += tan(value);
    bufferRW[uint(abs(dtid.x) * 10.0f)] += abs(value);
    bufferRW[uint(saturate(dtid.x) * 10.0f)] += saturate(value);
    bufferRW[uint(acos(dtid.x) * 10.0f)] += acos(value);
    bufferRW[uint(asin(dtid.x) * 10.0f)] += asin(value);
    bufferRW[uint(atan(dtid.x) * 10.0f)] += atan(value);
    bufferRW[uint(exp(dtid.x) * 10.0f)] += exp(value);
    bufferRW[uint(frac(dtid.x) * 10.0f)] += frac(value);
    bufferRW[uint(log(dtid.x) * 10.0f)] += log(value);
    bufferRW[uint(log(dtid.x) * 10.0f)] += log(value);
    bufferRW[uint(sqrt(dtid.x) * 10.0f)] += sqrt(value);
    bufferRW[uint(rsqrt(dtid.x) * 10.0f)] += rsqrt(value);
    bufferRW[uint(round(dtid.x) * 10.0f)] += round(value);

    float a = dtid.x;
    float b = dtid.y;

    // Binary
    bufferRW[uint(max(a, b) * 10.0f)] += max(a, b);
    bufferRW[uint(min(a, b) * 10.0f)] += min(a, b);
    bufferRW[uint(a * b * 10.0f)] += a * b;
    bufferRW[uint(a / b * 10.0f)] += a / b;
    bufferRW[uint(a % b * 10.0f)] += a % b;
    bufferRW[uint(a + b * 10.0f)] += a + b;
    bufferRW[uint(a - b * 10.0f)] += a - b;

    float3 av = dtid.xyx;
    float3 bv = dtid.yxy;

    // Vector
    bufferRW[uint(dot(av.x, bv.x) * 10.0f)] += dot(av.x, bv.x);
    bufferRW[uint(dot(av.xy, bv.xy) * 10.0f)] += dot(av.xy, bv.xy);
    bufferRW[uint(dot(av.xyz, bv.xyz) * 10.0f)] += dot(av.xyz, bv.xyz);
}
