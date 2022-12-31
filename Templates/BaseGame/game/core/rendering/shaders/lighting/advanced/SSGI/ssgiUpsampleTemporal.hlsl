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

#include "core/rendering/shaders/postFX/postFx.hlsl"
#include "core/rendering/shaders/torque.hlsl"
#include "core/rendering/shaders/shaderModelAutoGen.hlsl"

TORQUE_UNIFORM_SAMPLER2D(deferredTex, 1);
TORQUE_UNIFORM_SAMPLER2D(inputTex, 0);

uniform float2 oneOverTargetSize;
uniform float4x4 matPrevScreenToWorld;
uniform float4x4 matWorldToScreen;

float4 main(PFXVertToPix IN) : SV_TARGET
{
  float4 deferred = TORQUE_DEFERRED_UNCONDITION(deferredTex, IN.uv0.xy);
  float4 screenPos = float4(IN.uv0.x*2-1, IN.uv0.y*2-1, deferred.a*2-1, 1);
  // calculate position in world
  float4 D = mul(screenPos, matWorldToScreen);
  float4 worldPos = D;

  float4 prevPos = mul(worldPos, matPrevScreenToWorld);

  float2 velocity = ((screenPos - prevPos)).xy;
  IN.uv0.xy -= velocity;

  float4 temporal = TORQUE_TEX2D(inputTex, float2(IN.uv0.x, IN.uv0.y));

  return temporal;
}
