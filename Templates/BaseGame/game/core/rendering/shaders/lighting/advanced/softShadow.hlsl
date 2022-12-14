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

#include "../../shaderModel.hlsl"

#define NUM_TAPS 16

#define g_MinVariance 0.000001

float2 VogelDiskScale(float2 mapSize, int sampleSize)
{
	float2 texelSize = 2.0 / mapSize;
	return texelSize * sampleSize;
}

float2 VogelDisk(int sampleIndex, int sampleCount, float gradient)
{
	float gA = 2.4f;
	float r = sqrt(sampleIndex + 0.5f) / sqrt(sampleCount);
	float theta = sampleIndex * gA + gradient;
	
	float sine, cosine;
	sincos(theta, sine, cosine);
	
	return float2(r * cosine, r * sine);
}

float GradientNoise(float2 screenPos)
{
	float3 mag = float3(0.06711056f, 0.00583715f, 52.9829189f);
	return frac(mag.z * frac(dot(screenPos, mag.xy)));
}

float2 ComputeMoments(float Depth) 
{   
	float2 Moments;   
	// First moment is the depth itself.   
	Moments.x = Depth;   
	// Compute partial derivatives of depth.    
	float dx = ddx(Depth);   
	float dy = ddy(Depth);   
	// Compute second moment over the pixel extents.   
	Moments.y = Depth*Depth + 0.25*(dx*dx + dy*dy);   
	return Moments; 
} 

float linstep(float min, float max, float v) 
{   
	return clamp((v - min) / (max - min), 0, 1); 
} 

float ReduceLightBleeding(float p_max, float Amount) 
{   
	// Remove the [0, Amount] tail and linearly rescale (Amount, 1].    
	return linstep(Amount, 1, p_max); 
}

float ChebyshevUpperBound(float2 moments, float t) 
{    
	float p = (t <= moments.x);   
	float var = moments.y - (moments.x * moments.x);   
	var = max(var, g_MinVariance);   
	// Compute probabilistic upper bound 
	float d = t - moments.x;
	float p_max = var / (var + d*d);
	p_max = ReduceLightBleeding(p_max, 0.93);
	return max(p, p_max);
}

/// The texture used to do per-pixel pseudorandom
/// rotations of the filter taps.
TORQUE_UNIFORM_SAMPLER2D(gTapRotationTex, 2);

float softShadow_filter(   
TORQUE_SAMPLER2D(shadowMap),
TORQUE_SAMPLER2DCMP(shadowMapCMP),
						   float2 screenPos,
                           float2 vpos,
                           float2 shadowPos,
						   float cascade,
                           float filterRadius,
                           float distToLight,
                           float dotNL,
                           float esmFactor )
{
   #ifndef SOFTSHADOW

      // If softshadow is undefined then we skip any complex 
      // filtering... just do a single sample ESM.

      float occluder = TORQUE_TEX2DLOD( shadowMap, float4(shadowPos,0,0) ).r;
      float shadow = occluder;

   #else
   
    float gradient = 6.28318530718 * GradientNoise(screenPos);
	float2 textureSize;
	TORQUE_TEX2DGETSIZE(shadowMap, textureSize.x, textureSize.y);
	float2 shadowFilterSize = VogelDiskScale(textureSize, 4);
   
    float avgOccDepth = 0;
	float occCount = 0;
	[unroll]
	for(int i = 0; i < NUM_TAPS; i++)
	{
		float2 sampleUV = VogelDisk(i, NUM_TAPS, gradient);
		sampleUV = shadowPos + sampleUV * shadowFilterSize;
		float occDepth = TORQUE_TEX2DLOD(shadowMap, float4(sampleUV,0,cascade) ).r;
		if(occDepth < distToLight)
		{
			avgOccDepth += occDepth;
			occCount += 1.0f;
		}
	}
	
	// early out.
	if(occCount <= 0.0)
	{
		return 1.0;
	}
	
	avgOccDepth /= occCount;
	float penumbra = ((distToLight - avgOccDepth) * 1000.0f) / avgOccDepth;
	penumbra *= penumbra;
	float shadow = 0;
	[unroll]
	for ( int t = 0; t < NUM_TAPS; t++ )
    {
		float2 tap = VogelDisk(t, NUM_TAPS, gradient);
		tap = shadowPos + tap * penumbra * shadowFilterSize * filterRadius;
		
		float2 moments = TORQUE_TEX2D( shadowMap, tap ).xy;
		shadow += ChebyshevUpperBound(moments, distToLight);
	}

   #endif // SOFTSHADOW

   return shadow = shadow / float(NUM_TAPS);
}