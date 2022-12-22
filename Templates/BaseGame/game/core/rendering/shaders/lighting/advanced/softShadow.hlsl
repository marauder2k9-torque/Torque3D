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

#define g_MinVariance 0.0000000001
#define ShadowVSM 0
#define ShadowESM 1
#define ShadowMSM 2
#define ShadowPCF 3

//-----------------------------------------------------------------------------
// HELPER FUNCTIONS
//-----------------------------------------------------------------------------
float2 GetEVSMExponent(float pos, float neg)
{
	float max = 42.0f;
	
	float2 exponents = float2(pos, neg);
	
	return min(exponents, max);
}

float2 ComputeReceiverDepthBias(float3 shadowDX, float3 shadowDY)
{
	float2 biasUV;
    biasUV.x = shadowDY.y * shadowDX.z - shadowDX.y * shadowDY.z;
    biasUV.y = shadowDX.x * shadowDY.z - shadowDY.x * shadowDX.z;
    biasUV *= 1.0f / ((shadowDX.x * shadowDY.y) - (shadowDX.y * shadowDY.x));
    return biasUV;
}

float2 WarpDepth(float depth, float2 exponents)
{
	depth = 2.0f * depth - 1.0f;
	float pos = exp(exponents.x * depth);
	float neg = -exp(-exponents.y * depth);
	
	return float2(pos, neg);
}

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

float3 VogelDiskCube(int sampleIndex, int sampleCount, float gradient)
{
	float gA = 2.4f;
	float r = sqrt(sampleIndex + 0.5f) / sqrt(sampleCount);
	float theta = sampleIndex * gA + gradient;
	
	float sine, cosine;
	sincos(theta, sine, cosine);
	
	return float3(r * cosine, r * sine, sin(theta));
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
	if(t <= moments.x)
		return 1.0f;
		
	float var = moments.y - (moments.x * moments.x);   
	var = max(var, g_MinVariance);   
	// Compute probabilistic upper bound 
	float d = t - moments.x;
	float p_max = var / (var + d*d);
	p_max = ReduceLightBleeding(p_max, 0.02);
	return p_max;
}

float ChebyshevUpperBound(float2 moments, float t, float minVar) 
{    
	if(t <= moments.x)
		return 1.0f;
		
	float var = moments.y - (moments.x * moments.x);   
	var = max(var, minVar);   
	// Compute probabilistic upper bound 
	float d = t - moments.x;
	float p_max = var / (var + d*d);
	p_max = ReduceLightBleeding(p_max, 0.95);
	return p_max;
}

float AvgOccDepthToPenumbra(float shadowZ, float avgOccDepth)
{
	float penumbra = (shadowZ - avgOccDepth) / avgOccDepth;
	penumbra *= penumbra;
	return saturate(80.0f * penumbra);
}

float AvgOccDepthToPenumbra(float lightSize, float zShadow, float avgDepth)
{
	float penumbra = lightSize * (zShadow - avgDepth) / avgDepth;
	return penumbra;
}

float Penumbra(TORQUE_SAMPLER2DARRAY(shadowMap),
				int samples,
				float3 shadowPos,
				float searchUV,
				float gradient,
				uint cascade)
{
	float avgOccDepth = 0;
	float occCount = 0;
	[unroll]
	for(int i = 0; i < samples; i++)
	{
		float2 sampleUV = VogelDisk(i, samples, gradient);
		sampleUV = shadowPos.xy + sampleUV * searchUV;
		float occDepth = TORQUE_TEX2DLEVEL(shadowMap, float4(sampleUV,cascade,0)).r;
		if(occDepth < shadowPos.z)
		{
			avgOccDepth += occDepth;
			occCount += 1.0f;
		}
	}
	
	if(occCount > 0.0f)
	{
		avgOccDepth /= occCount;
		return avgOccDepth;
	}
	else
	{
		return 1.0f;
	}
}

//-----------------------------------------------------------------------------
// PCF
//-----------------------------------------------------------------------------
float SampleShadowPCF(   
						   TORQUE_SAMPLER2DARRAY(shadowMap),
                           float3 shadowPos,
						   float esmFactor,
						   uint cascade,
                           float filterRadius,
						   float gradient,
						   float dotNL)
{
	return TORQUE_TEX2DLEVEL( shadowMap, float4(shadowPos.xy,cascade,0)).x;
}

float softShadow_filterCube(   
						   TORQUE_SAMPLERCUBE(shadowMap),
						   float2 screenPos,
                           float2 vpos,
                           float3 shadowPos,
						   float distToLight,
						   uint cascade,
                           float filterRadius,
						   float dotNL)
{
    float gradient = 6.28318530718 * GradientNoise(screenPos);
	float shadow = 1.0;
	[unroll]
	for ( int t = 0; t < NUM_TAPS; t++ )
    {
		float3 tap = VogelDiskCube(t, NUM_TAPS, gradient);
		tap = shadowPos.xyz + tap * filterRadius;
		
		shadow += TORQUE_TEXCUBE( shadowMap, tap ).x;
	}
	
   return shadow = shadow / float(NUM_TAPS);
}

//-----------------------------------------------------------------------------
// VSM
//-----------------------------------------------------------------------------
float SampleShadowVSM(  TORQUE_SAMPLER2DARRAY(shadowMap),
                        float3 shadowPos,
                        float3 shadowPosDX,
                        float3 shadowPosDY,
						uint cascade)
{
	float2 depth = TORQUE_TEX2DGRAD(shadowMap, float3(shadowPos.xy, cascade),shadowPosDX.xy, shadowPosDY.xy).xy;
	return ChebyshevUpperBound(depth, shadowPos.z);
}

float SampleShadowVSM(  TORQUE_SAMPLER2DARRAY(shadowMap),
                        float3 shadowPos,
						uint cascade)
{
	float2 depth = TORQUE_TEX2DLEVEL(shadowMap, float4(shadowPos.xy, cascade, 0)).xy;
	return ChebyshevUpperBound(depth, shadowPos.z);
}

//-----------------------------------------------------------------------------
// EVSM
//-----------------------------------------------------------------------------
float SampleShadowEVSM( TORQUE_SAMPLER2DARRAY(shadowMap),
                        float3 shadowPos,
						float3 shadowPosDX,
                        float3 shadowPosDY,
						uint cascade)
{
	// positive and negative exponents should be a shader input.
	float2 exponents = GetEVSMExponent(40.0f, 5.0f);
	float2 warpDepth = WarpDepth(shadowPos.z, exponents);
	
	float4 depth = TORQUE_TEX2DGRAD(shadowMap, float3(shadowPos.xy, cascade),shadowPosDX.xy, shadowPosDY.xy);
	
	float2 depthScale = exponents * warpDepth;
	float2 minVar = depthScale * depthScale;
	
	float posContrib = ChebyshevUpperBound(depth.xz, warpDepth.x, g_MinVariance);
	float negContrib = ChebyshevUpperBound(depth.yw, warpDepth.y, g_MinVariance);
	
	return min(posContrib, negContrib);
}

float SampleShadow( TORQUE_SAMPLER2DARRAY(shadowMap),
					float2 screenPos,
                    float2 vpos,
                    float3 shadowPos,
					float3 shadowPosDX,
					float3 shadowPosDY,
					float shadowRes,
					float shadowScale,
					float lightSize,
					uint cascade,
                    float filterRadius,
					float esmFactor,
					float dotNL,
					int samplerMethod)
{
	float gradient = 6.28318530718 * GradientNoise(screenPos);
	
	float2 depthBias = ComputeReceiverDepthBias(shadowPosDX, shadowPosDY);
	float fracError = 2 * dot(float2(1.0f, 1.0f) * shadowScale, abs(depthBias));
	//shadowPos.z -= min(fracError, 0.01f);
	
	float searchUV = lightSize * filterRadius;
	float penumbra = Penumbra(TORQUE_SAMPLER2D_MAKEARG(shadowMap), 16, shadowPos, searchUV, gradient, cascade);
	float penumbraFilter = penumbra * lightSize * filterRadius;
	
	float shadow = 1.0;
	
	[unroll]
	for ( int i = 0; i < 16; i++ )
	{
		float2 tap = VogelDisk(i, 16, gradient);
		tap = shadowPos.xy + penumbraFilter * tap;
		if ( shadow * (1.0 - shadow) * max( dotNL, 0 ) > esmFactor)
			shadow += SampleShadowVSM(TORQUE_SAMPLER2D_MAKEARG(shadowMap), float3(tap,shadowPos.z), shadowPosDX, shadowPosDY, cascade);
		else
			break;
	}
	
	//shadow = SampleShadowPCF(TORQUE_SAMPLER2D_MAKEARG(shadowMap), screenPos, shadowPos, esmFactor, cascade, filterRadius, dotNL);
	//shadow = SampleShadowVSM(TORQUE_SAMPLER2D_MAKEARG(shadowMap), shadowPos, cascade);
	//shadow = SampleShadowEVSM(TORQUE_SAMPLER2D_MAKEARG(shadowMap), shadowPos, shadowPosDX, shadowPosDY, cascade);
	
	return shadow = shadow / float(16);
}

float SampleShadow( TORQUE_SAMPLER2DARRAY(shadowMap),
					float2 screenPos,
                    float2 vpos,
                    float3 shadowPos,
					float shadowRes,
					float shadowScale,
					float lightSize,
					uint cascade,
                    float filterRadius,
					float esmFactor,
					float dotNL,
					int samplerMethod)
{
	float gradient = 6.28318530718 * GradientNoise(screenPos);
	
	float searchUV = lightSize * filterRadius;
	float penumbra = Penumbra(TORQUE_SAMPLER2D_MAKEARG(shadowMap), 16, shadowPos, searchUV, gradient, cascade);
	float penumbraFilter = penumbra * lightSize * filterRadius;
	
	float shadow = 1.0;
	
	[unroll]
	for ( int i = 0; i < 16; i++ )
	{
		float2 tap = VogelDisk(i, 16, gradient);
		tap = shadowPos.xy + penumbraFilter * tap;
		if ( shadow * (1.0 - shadow) * max( dotNL, 0 ) > esmFactor)
			shadow += SampleShadowVSM(TORQUE_SAMPLER2D_MAKEARG(shadowMap), float3(tap,shadowPos.z), cascade);
		else
			break;
	}
	
	//shadow = SampleShadowPCF(TORQUE_SAMPLER2D_MAKEARG(shadowMap), screenPos, shadowPos, esmFactor, cascade, filterRadius, dotNL);
	//shadow = SampleShadowVSM(TORQUE_SAMPLER2D_MAKEARG(shadowMap), shadowPos, cascade);
	//shadow = SampleShadowEVSM(TORQUE_SAMPLER2D_MAKEARG(shadowMap), shadowPos, shadowPosDX, shadowPosDY, cascade);
	
	return shadow = shadow / float(16);
}