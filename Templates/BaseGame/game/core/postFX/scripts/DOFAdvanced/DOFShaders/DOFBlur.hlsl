//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "../../../../rendering/shaders/postFX/postFx.hlsl"

uniform float2 targetSize;
uniform int kernalSize;

#ifndef BLUR_DIR
#define BLUR_DIR float2(0.0,0.0);
#endif

TORQUE_UNIFORM_SAMPLER2D(inputTex, 0);

float calcGaussianWeight(int sampleDist, float sigma)
{
    float g = 1.0f / sqrt(2.0f * 3.14159 * sigma * sigma);
    return (g * exp(-(sampleDist * sampleDist) / (2 * sigma * sigma)));
}

float4 main(PFXVertToPix IN) : SV_TARGET
{
    float4 col = 0; 
    float weightSum = 0.0f;

    for(int i = -6; i < 6; i++) 
    { 
        float weight = calcGaussianWeight(i, 2.0f);
        weightSum += weight;
        float2 uv = IN.uv0.xy;
        uv += (i / targetSize) * BLUR_DIR;

        float4 sample = TORQUE_TEX2D(inputTex, uv);
        col += sample * weight;
    }

    col /= weightSum; 

    return col;
}
