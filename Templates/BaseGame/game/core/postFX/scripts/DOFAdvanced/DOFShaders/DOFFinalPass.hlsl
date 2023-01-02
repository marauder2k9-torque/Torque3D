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
TORQUE_UNIFORM_SAMPLER2D(backBuffer, 1);
TORQUE_UNIFORM_SAMPLER2D(nearTex, 2);
TORQUE_UNIFORM_SAMPLER2D(farTex, 3);

float COC(float z)
{
    float coc = -apertureWidth * (focalLength * (focusDist - z)) / ( z * (focusDist - focalLength));

    coc = (coc / filmSize) * targetSize.x;
    coc = clamp(coc / 21.0f, -1.0f, 1.0f);

    return coc;
}

float4 main(PFXVertToPix IN) : SV_TARGET
{
    float3 col = TORQUE_TEX2D(backBuffer, IN.uv0.xy).rgb;
    float depth = TORQUE_DEFERRED_UNCONDITION(deferredBuffer, IN.uv0.xy).a;
    float coc = COC(depth);

    float4 farSample = TORQUE_TEX2D(farTex, IN.uv0.xy); 
    float3 far = farSample.rgb;

    float farBlend = saturate(saturate(coc) * 21.0f - 0.5);
    float3 result = lerp(col, far.xyz, smoothstep(0.0f,1.0f,farBlend));

    float4 nearSample = TORQUE_TEX2D(nearTex, IN.uv0.xy);
    float3 near = nearSample.rgb;

    float nearBlend = saturate(nearSample.w * 2.0f);
    result = lerp(result, near.xyz, smoothstep(0.0f, 1.0f, nearBlend));

    return float4(result, 1); 
}