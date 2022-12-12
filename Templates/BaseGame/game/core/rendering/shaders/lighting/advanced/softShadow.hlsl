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

#define NUM_TAPS 12

/// The non-uniform poisson disk used in the
/// high quality shadow filtering.
static float2 sNonUniformTaps[NUM_TAPS] = 
{      
	{-.326,-.406},
	{-.840,-.074},
	{-.696, .457},
	{-.203, .621},
	{ .962,-.195},
	{ .473,-.480},
	{ .519, .767},
	{ .185,-.893},
	{ .507, .064},
	{ .896, .412},
	{-.322,-.933},
	{-.792,-.598}
};

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
										 0,
										 NUM_TAPS );
										 
		// This averages the taps above with the results
		// of the prediction samples.
		shadow *= 0.5;                              
	 }

   #endif // SOFTSHADOW

   return shadow;
}