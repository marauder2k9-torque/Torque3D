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

#include "../../torque.hlsl"

struct ConnectData
{
    float4 hpos     : SV_Position;
    float2 uv      : TEXCOORD;
};

TORQUE_UNIFORM_SAMPLER2DARRAY(shadowMap, 0);

uniform float2 resolution;
uniform float2 blurDir;
uniform int blurSamples;

float4 main(ConnectData IN) : TORQUE_TARGET0
{
	float2 texel = 1 / resolution;
	float blurSizeInv = 1 / float(blurSamples);
	float2 sampleOffset = texel * blurDir;
	float2 offset = 0.5 * float(blurSamples - 1) * sampleOffset;
	float2 baseTex = IN.uv - offset;
	
	float4 sum = float4(0,0,0,0);
	for(int i = 0; i < blurSamples; ++i)
	{
		sum += TORQUE_TEX2DLEVEL(shadowMap, float4(baseTex + i * sampleOffset, 0, 0) );
	}
	
	
	return sum * blurSizeInv;
}