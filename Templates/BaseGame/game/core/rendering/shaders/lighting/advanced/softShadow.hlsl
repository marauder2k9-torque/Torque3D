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

#if defined( SOFTSHADOW ) && defined( SOFTSHADOW_HIGH_QUALITY )

#define NUM_PRE_TAPS 4
#define NUM_TAPS 12

/// The non-uniform poisson disk used in the
/// high quality shadow filtering.
static float2 sNonUniformTaps[NUM_TAPS] = 
{      
   // These first 4 taps are located around the edges
   // of the disk and are used to predict fully shadowed
   // or unshadowed areas.
   { 0.992833, 0.979309 },
   { -0.998585, 0.985853 },
   { 0.949299, -0.882562 },
   { -0.941358, -0.893924 },

   // The rest of the samples.
   { 0.545055, -0.589072 },
   { 0.346526, 0.385821 },
   { -0.260183, 0.334412 },
   { 0.248676, -0.679605 },
   { -0.569502, -0.390637 },
   { -0.614096, 0.212577 },
   { -0.259178, 0.876272 },
   { 0.649526, 0.864333 },
};

#else

#define NUM_PRE_TAPS 5

/// The non-uniform poisson disk used in the
/// high quality shadow filtering.
static float2 sNonUniformTaps[NUM_PRE_TAPS] = 
{      
   { 0.892833, 0.959309 },
   { -0.941358, -0.873924 },
   { -0.260183, 0.334412 },
   { 0.348676, -0.679605 },
   { -0.569502, -0.390637 },
};

#endif


/// The texture used to do per-pixel pseudorandom
/// rotations of the filter taps.
TORQUE_UNIFORM_SAMPLER2D(gTapRotationTex, 2);

float softShadow_sampleTaps(  TORQUE_SAMPLER2D(shadowMap1),
                              float2 sinCos,
                              float2 shadowPos,
                              float filterRadius,
                              float distToLight,
                              float esmFactor,
                              int startTap,
                              int endTap )
{
   float shadow = 0;

   float2 tap = 0;
   for ( int t = startTap; t < endTap; t++ )
   {
      tap.x = ( sNonUniformTaps[t].x * sinCos.y - sNonUniformTaps[t].y * sinCos.x ) * filterRadius;
      tap.y = ( sNonUniformTaps[t].y * sinCos.y + sNonUniformTaps[t].x * sinCos.x ) * filterRadius;
      float occluder = TORQUE_TEX2DLOD( shadowMap1, float4( shadowPos + tap, 0, 0 ) ).r;

      float esm = saturate( exp( esmFactor * ( occluder - distToLight ) ) );
      shadow += esm / float( endTap - startTap );
   }

   return shadow;
}


float softShadow_filter(   TORQUE_SAMPLER2D(shadowMap),
                           float2 vpos,
                           float2 shadowPos,
                           float filterRadius,
                           float distToLight,
                           float dotNL,
                           float esmFactor )
{
   #ifndef SOFTSHADOW

      // If softshadow is undefined then we skip any complex 
      // filtering... just do a single sample ESM.

      float occluder = TORQUE_TEX2DLOD(shadowMap, float4(shadowPos, 0, 0)).r;
      float shadow = saturate( exp( esmFactor * ( occluder - distToLight ) ) );

   #else
      // Lookup the random rotation for this screen pixel.
      float2 sinCos = ( TORQUE_TEX2DLOD(gTapRotationTex, float4(vpos * 16, 0, 0)).rg - 0.5) * 2;

      // Do the prediction taps first.
      float shadow = softShadow_sampleTaps(  TORQUE_SAMPLER2D_MAKEARG(shadowMap),
                                             sinCos,
                                             shadowPos,
                                             filterRadius,
                                             distToLight,
                                             esmFactor,
                                             0,
                                             NUM_PRE_TAPS );

      // We live with only the pretap results if we don't
      // have high quality shadow filtering enabled.
      #ifdef SOFTSHADOW_HIGH_QUALITY

         // Only do the expensive filtering if we're really
         // in a partially shadowed area.
         if ( shadow * ( 1.0 - shadow ) * max( dotNL, 0 ) > 0.06 )
         {
            shadow += softShadow_sampleTaps( TORQUE_SAMPLER2D_MAKEARG(shadowMap),
                                             sinCos,
                                             shadowPos,
                                             filterRadius,
                                             distToLight,
                                             esmFactor,
                                             NUM_PRE_TAPS,
                                             NUM_TAPS );
                                             
            // This averages the taps above with the results
            // of the prediction samples.
            shadow *= 0.5;                              
         }

      #endif // SOFTSHADOW_HIGH_QUALITY

   #endif // SOFTSHADOW

   return shadow;
}

//------------------------------------------------------------------
// Shadow soften methods.
//------------------------------------------------------------------

//------------------------------------------------------------------
// Shadow helper functions.
//------------------------------------------------------------------

float getShadowBias(float shadowSetBias, float3 fragWorldNormal, float3 fragPosWorld, float3 lightPosWorld)
{
   float3 lightDiff = fragPosWorld - lightPosWorld;
   float cosTheta = dot(fragWorldNormal, normalize(lightDiff));
   cosTheta = clamp(cosTheta, 0.0, 1.0);
   return clamp(shadowSetBias * 0.1 * (1.0-(cosTheta)), shadowSetBias * 0.01, shadowSetBias);
}

float3 getShadowPosOffset(float nDotL, float texelSize, float3 worldNormal, float offsetScale)
{
   float normalOffset = saturate(1.0f - nDotL);
   return texelSize * offsetScale * normalOffset * worldNormal;
}

float Random(float2 co)
{
	return frac(sin(dot(co.xy, float2(12.9898, 78.233))) * 43758.5453);
}

float GradientNoise(float2 screenPos)
{
	float3 mag = float3(0.06711056f, 0.00583715f, 52.9829189f);
	return frac(mag.z * frac(dot(screenPos, mag.xy)));
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

//------------------------------------------------------------------
// PCF
//------------------------------------------------------------------

float softShadow_filterPCF( TORQUE_SAMPLER2D(shadowMap),
                           float3 shadowCoord,
                           float resolution,
                           float pixelSize,
                           int filterSize)
{
   float shadow = 0.0f;
   float2 grad = frac(shadowCoord.xy * resolution + 0.5f);

   for(int i = -filterSize; i <= filterSize; i++)
   {
      for(int j = -filterSize; j <= filterSize; j++)
      {
         float4 tmp = TORQUE_TEX2DGATHER(shadowMap, shadowCoord.xy + float2(i,j) * float2(pixelSize, pixelSize));
         tmp.x = tmp.x < shadowCoord.z ? 0.0f : 1.0f;
			tmp.y = tmp.y < shadowCoord.z ? 0.0f : 1.0f;
			tmp.z = tmp.z < shadowCoord.z ? 0.0f : 1.0f;
			tmp.w = tmp.w < shadowCoord.z ? 0.0f : 1.0f;
			shadow += lerp(lerp(tmp.w, tmp.z, grad.x), lerp(tmp.x, tmp.y, grad.x), grad.y);
      }
   }

   return shadow / (float)((2*filterSize + 1) * (2 * filterSize + 1));
}

//------------------------------------------------------------------
// PCSS
//------------------------------------------------------------------

static const int pcss_SampleCount = 32;

static const float2 pcss_Samples[pcss_SampleCount] = {
	float2(0.06407013, 0.05409927),
	float2(0.7366577, 0.5789394),
	float2(-0.6270542, -0.5320278),
	float2(-0.4096107, 0.8411095),
	float2(0.6849564, -0.4990818),
	float2(-0.874181, -0.04579735),
	float2(0.9989998, 0.0009880066),
	float2(-0.004920578, -0.9151649),
	float2(0.1805763, 0.9747483),
	float2(-0.2138451, 0.2635818),
	float2(0.109845, 0.3884785),
	float2(0.06876755, -0.3581074),
	float2(0.374073, -0.7661266),
	float2(0.3079132, -0.1216763),
	float2(-0.3794335, -0.8271583),
	float2(-0.203878, -0.07715034),
	float2(0.5912697, 0.1469799),
	float2(-0.88069, 0.3031784),
	float2(0.5040108, 0.8283722),
	float2(-0.5844124, 0.5494877),
	float2(0.6017799, -0.1726654),
	float2(-0.5554981, 0.1559997),
	float2(-0.3016369, -0.3900928),
	float2(-0.5550632, -0.1723762),
	float2(0.925029, 0.2995041),
	float2(-0.2473137, 0.5538505),
	float2(0.9183037, -0.2862392),
	float2(0.2469421, 0.6718712),
	float2(0.3916397, -0.4328209),
	float2(-0.03576927, -0.6220032),
	float2(-0.04661255, 0.7995201),
	float2(0.4402924, 0.3640312),
};

float2 pcss_Rotate(float2 pos, float2 rotation)
{
   return float2(pos.x * rotation.x - pos.y * rotation.y, pos.y * rotation.x + pos.x * rotation.y);
}

float pcss_Penumbra(TORQUE_SAMPLER2D(shadowMap),
                     float3 penumbraCoord, 
                     float searchUV)
{
   int blockCount = 0;
   float avgBlockDistance = 0.0f;

   [unroll]
   for(int i = 0; i < pcss_SampleCount; i++)
   {
      float2 offset = pcss_Samples[i] * searchUV;
      float z = TORQUE_TEX2D(shadowMap, penumbraCoord.xy + offset).r;

      if(z < penumbraCoord.z) 
      {
         blockCount++;
         avgBlockDistance += z;
      }
   }

   if(blockCount > 0)
	{
		avgBlockDistance /= float(blockCount);
		return avgBlockDistance;
	}
	else
	{
		return 999.0f;
	}
}

float softShadow_filterPCSS(TORQUE_SAMPLER2D(shadowMap),
                           float3 shadowCoord,
                           float softness,
                           float lightSize)
{
   float shadow = 1.0f;

   float2 shadowUV = shadowCoord.xy;
   float depth = shadowCoord.z;
   float searchUV = lightSize * softness;

   float penumbra = pcss_Penumbra(TORQUE_SAMPLER2D_MAKEARG(shadowMap), shadowCoord, searchUV);
   if(penumbra > 997.0f)
   {
      return 1.0f;
   }
   
   float filterRadius = penumbra * lightSize * softness;

   [unroll]
   for(int i = 0; i < pcss_SampleCount; i++)
   {
      float2 offset = pcss_Samples[i] * filterRadius;
      float shadowTemp = TORQUE_TEX2D(shadowMap, shadowCoord.xy + offset).r;
      shadow += depth < shadowTemp;
   }

   shadow /= float(pcss_SampleCount);
   return shadow;
}