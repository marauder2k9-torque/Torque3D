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
#include "../../../../rendering/shaders/shaderModel.hlsl"
#include "../../../../rendering/shaders/shaderModelAutoGen.hlsl"

// uniforms from postEffect.cpp 
uniform float2 targetSize;

// user defined
uniform float apertureWidth;
uniform float focalLength;
uniform float focusDist;
uniform float filmSize;
uniform float maxCocSize;

TORQUE_UNIFORM_SAMPLER2D(deferredBuffer, 0);
TORQUE_UNIFORM_SAMPLER2D(colorBuffer, 1);

float4 CalculateCoC(float3 col, float z)
{
    float4 output;
    float coc = -(apertureWidth) * (focalLength *(focusDist - z)) / ( z * (focusDist - focalLength));

    coc = (coc / filmSize) * targetSize.x;
    coc = clamp(coc / maxCocSize, -1.0f, 1.0f);

    #ifdef NEAR
        output =  float4(col, 1.0) * max(-coc,0.0f);
        output.xyz = col;
    #else
        output = float4(col, 1.0f) * max(coc, 0.0f);
    #endif

    return output;
}

float4 main(PFXVertToPix IN) : SV_TARGET
{
    float2 pixPos = IN.uv0.xy;

    float4 tapOut = 0.0f;

    [unroll]
    for(int y = 0; y < 2; y++)
    {
        [unroll]
        for(int x = 0; x < 2; x++)
        { 
            float2 pos = pixPos + float2(x,y);
            float3 col = TORQUE_TEX2D(colorBuffer,pos).rgb;
            float depth = TORQUE_DEFERRED_UNCONDITION(deferredBuffer, pos).a;
            tapOut += CalculateCoC(col, depth);
        }
    }

    float4 output = (tapOut / 4.0f);

    return output;
}