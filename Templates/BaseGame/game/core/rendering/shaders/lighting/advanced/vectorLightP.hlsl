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
#include "../../shaderModelAutoGen.hlsl"

#include "farFrustumQuad.hlsl"
#include "../../torque.hlsl"
#include "../../lighting.hlsl"
#include "../shadowMap/shadowMapIO_HLSL.h"

TORQUE_UNIFORM_SAMPLER2D(deferredBuffer, 0);
TORQUE_UNIFORM_SAMPLER2D(shadowMap, 1);
//contains gTapRotationTex sampler 
#include "softShadow.hlsl"
TORQUE_UNIFORM_SAMPLER2D(colorBuffer, 3);
TORQUE_UNIFORM_SAMPLER2D(matInfoBuffer, 4);
TORQUE_UNIFORM_SAMPLER2DCMP(shadowMapCmp, 5);

uniform float  lightBrightness;
uniform float3 lightDirection;

uniform float4 lightColor;
uniform float4 lightAmbient;

uniform float shadowSoftness;

uniform float4 atlasXOffset;
uniform float4 atlasYOffset;
uniform float4 zNearFarInvNearFar;
uniform float4 lightMapParams;
uniform float4 farPlaneScalePSSM;
uniform float4 overDarkPSSM;

uniform float2 fadeStartLength;
uniform float2 atlasScale;

uniform float4x4 eyeMat;
uniform float4x4 cameraToWorld;

// Static Shadows
uniform float4x4 worldToLightProj;
uniform float4 cascadeSplits;

uniform float4 cascadeOffsets0;
uniform float4 cascadeOffsets1;
uniform float4 cascadeOffsets2;
uniform float4 cascadeOffsets3;

uniform float4 cascadeScales0;
uniform float4 cascadeScales1;
uniform float4 cascadeScales2;
uniform float4 cascadeScales3;

uniform float4 scaleX;
uniform float4 scaleY;
uniform float4 offsetX;
uniform float4 offsetY;

#define USEPROJECTION 1
#define USEBLEND 1

float3 GetShadowOffset(TORQUE_SAMPLER2D(shadowMapSize), float nDotL, float3 normal)
{
	float offsetScale = 0.001f;
	float2 shadowSize;
	TORQUE_TEX2DGETSIZE(shadowMapSize, shadowSize.x, shadowSize.y);
	float texel = 2.0f / shadowSize.x;
	float normalOffset = saturate(1.0 - nDotL);
	return texel * offsetScale * normalOffset * normal;
}

float4 AL_VectorLightShadowCast( TORQUE_SAMPLER2D(sourceShadowMap),
								TORQUE_SAMPLER2DCMP(sourceShadowMapCMP),
								float2 screenPos,
                                float2 texCoord,
                                float4x4 worldToLightProj,
                                float3 worldPos,
                                float4 scaleX,
                                float4 offsetX,
                                float4 offsetY,
								float4 cascadeOffsets[4],
								float4 cascadeScales[4],
                                float4 farPlaneScalePSSM,
                                float dotNL,
								float3 normal,
								float depth)
{
      // Compute shadow map coordinate
	  float4 pxlPosLightProj = mul(worldToLightProj, float4(worldPos,1));
	  float2 baseShadowCoord = pxlPosLightProj.xy / pxlPosLightProj.w;   
	  
	  // Distance to light, in shadowmap space
      float distToLight = pxlPosLightProj.z / pxlPosLightProj.w;
	  
	  uint cascadeId = 3;
	  [unroll]
	  for(int i = 3; i >= 0; --i)
	  {
	  
		#if USEPROJECTION
			float2 shadowCoord = baseShadowCoord;
			shadowCoord.xy *= cascadeScales[i].xy;
			shadowCoord.xy += cascadeOffsets[i].xy;
			float farPlaneDists = distToLight.x;
			farPlaneDists *= farPlaneScalePSSM[i];
			
			if(shadowCoord.x > -0.99 && shadowCoord.x < 0.99 && shadowCoord.y > -0.99 && shadowCoord.y < 0.99 && farPlaneDists < 1.0)
				cascadeId = i;
		#else
			if(depth <= cascadeSplits[i])
			cascadeId = i;
		#endif
			
	  }
	  
	   // Each split has a different far plane, take this into account.
      float farPlaneScale = farPlaneScalePSSM[cascadeId];
	  distToLight *= farPlaneScale;
	  
	  float3 shadowPos;
	  shadowPos.xy = (baseShadowCoord * cascadeScales[cascadeId].xy ) + cascadeOffsets[cascadeId].xy;
	  shadowPos.z = distToLight;
	  
	  // Convert to texcoord space
      shadowPos.xy = 0.5 * shadowPos.xy + float2(0.5, 0.5);
      shadowPos.y = 1.0f - shadowPos.y;
	  
	  // Move around inside of atlas 
      float2 aOffset;
      aOffset.x = atlasXOffset[cascadeId];
      aOffset.y = atlasYOffset[cascadeId];
	  
	  shadowPos.xy *= atlasScale;
	  shadowPos.xy += aOffset;
	  
	  float3 debugColor = float3(0,0,0);
	  
	  #ifdef PSSM_DEBUG_RENDER
         const float3 CascadeColors[4] =
        {
            float3(1.0f, 0.0, 0.0f),
            float3(0.0f, 1.0f, 0.0f),
            float3(0.0f, 0.0f, 1.0f),
            float3(1.0f, 1.0f, 0.0f)
        };
		
		debugColor = CascadeColors[cascadeId];
      #endif
	  
      float4 shadowSample = float4(debugColor, softShadow_filter(  
	  TORQUE_SAMPLER2D_MAKEARG(sourceShadowMap),
	  TORQUE_SAMPLER2D_MAKEARG(sourceShadowMapCMP),
	  screenPos, 
	  texCoord, 
	  shadowPos, 
	  cascadeId,
	  farPlaneScale * shadowSoftness));
	  
	  #if USEBLEND
	  
	  if(cascadeId != 3)
	  {
	  
		  float nextSplit = cascadeSplits[cascadeId];
		  float splitSize = cascadeId == 0 ? nextSplit : nextSplit - cascadeSplits[cascadeId - 1];
          float fadeFactor = (nextSplit - depth) / splitSize;
		  
		  #if USEPROJECTION
			  float3 cascadePos;
			  cascadePos.xy = (baseShadowCoord * cascadeScales[cascadeId].xy ) + cascadeOffsets[cascadeId].xy;
			  cascadePos.z = distToLight;
			  
			  // Convert to texcoord space
			  cascadePos.xy = 0.5 * cascadePos.xy + float2(0.5, 0.5);
			  cascadePos.xy *= atlasScale;
			  cascadePos.xy += aOffset;
			  cascadePos = abs(cascadePos * 2.0f - 1.0f);
			
			float distToEdge = 1.0f - max(max(cascadePos.x, cascadePos.y), cascadePos.z);
            fadeFactor = max(distToEdge, fadeFactor);
		  #endif
	  
		  float alpha = 0.1;
		  cascadeId = cascadeId + 1;
		  
		  // Each split has a different far plane, take this into account.
		  distToLight = pxlPosLightProj.z / pxlPosLightProj.w;
		  float farPlaneScale = farPlaneScalePSSM[cascadeId];
		  distToLight *= farPlaneScale;
		  
		  float3 nextShadowPos;
		  nextShadowPos.xy = (baseShadowCoord * cascadeScales[cascadeId].xy) + cascadeOffsets[cascadeId].xy;
		  nextShadowPos.z = distToLight;
		  
		  nextShadowPos.xy = 0.5 * nextShadowPos.xy + float2(0.5, 0.5);
		  nextShadowPos.y = 1.0f - nextShadowPos.y;
		  
		  float2 aOffset;
		  aOffset.x = atlasXOffset[cascadeId];
		  aOffset.y = atlasYOffset[cascadeId];
		  
		  nextShadowPos.xy *= atlasScale;
		  nextShadowPos.xy += aOffset;
		  
		  float nextShadow = softShadow_filter(  
							  TORQUE_SAMPLER2D_MAKEARG(sourceShadowMap),
							  TORQUE_SAMPLER2D_MAKEARG(sourceShadowMapCMP),
							  screenPos, 
							  texCoord, 
							  nextShadowPos, 
							  cascadeId,
							  farPlaneScale * shadowSoftness);
							  
		   float lerpAmt = smoothstep(0.0f, alpha, fadeFactor);
		
		   shadowSample.a = lerp(nextShadow, shadowSample.a, lerpAmt);
	  }
	  
	  #endif

	  //shadowSample.a = saturate(shadowSample.a + 1.0 - any(cascadeMask));
	  
	  return shadowSample;
};


float4 main(FarFrustumQuadConnectP IN) : SV_TARGET
{
   //unpack normal and linear depth  
   float4 normDepth = TORQUE_DEFERRED_UNCONDITION(deferredBuffer, IN.uv0);
  
   //create surface
   Surface surface = createSurface( normDepth, TORQUE_SAMPLER2D_MAKEARG(colorBuffer),TORQUE_SAMPLER2D_MAKEARG(matInfoBuffer),
                                    IN.uv0, eyePosWorld, IN.wsEyeRay, cameraToWorld);
                                    
   //early out if emissive
   if (getFlag(surface.matFlag, 0))
   {   
      return float4(0, 0, 0, 0);
	}
   
   //create surface to light                           
   SurfaceToLight surfaceToLight = createSurfaceToLight(surface, -lightDirection);

   //light color might be changed by PSSM_DEBUG_RENDER
   float3 lightingColor = lightColor.rgb;
   
   #ifdef NO_SHADOW
      float shadow = 1.0;
   #else
   
	  float4 cascadeOffsets[4] = {
		cascadeOffsets0,
		cascadeOffsets1,
		cascadeOffsets2,
		cascadeOffsets3
	  };
	  
	
      float4 cascadeScales[4] = {
		cascadeScales0,
		cascadeScales1,
		cascadeScales2,
		cascadeScales3
	  };
	

      // Fade out the shadow at the end of the range.
      float4 zDist = (zNearFarInvNearFar.x + zNearFarInvNearFar.y * surface.depth);
      float fadeOutAmt = ( zDist.x - fadeStartLength.x ) * fadeStartLength.y;

      float4 shadowed_colors = AL_VectorLightShadowCast( 
	  TORQUE_SAMPLER2D_MAKEARG(shadowMap), 
	  TORQUE_SAMPLER2D_MAKEARG(shadowMapCmp),
	  IN.hpos.xy, 
	  IN.uv0.xy,
	  worldToLightProj, 
	  surface.P, 
	  scaleX, 
	  offsetX, 
	  offsetY,
	  cascadeOffsets,
	  cascadeScales,
      farPlaneScalePSSM, 
	  surfaceToLight.NdotL,
	  surface.N,
	  surface.depth);

      float shadow = shadowed_colors.a;
	  
      #ifdef PSSM_DEBUG_RENDER
	     lightingColor = shadowed_colors.rgb;
      #endif

      shadow = lerp( shadow, 1.0, saturate( fadeOutAmt ) );

      #ifdef PSSM_DEBUG_RENDER
         if ( fadeOutAmt > 1.0 )
            lightingColor = 1.0;
      #endif

   #endif //NO_SHADOW
   
   #ifdef DIFFUSE_LIGHT_VIZ
      float3 factor = lightingColor.rgb * max(surfaceToLight.NdotL, 0) * shadow * lightBrightness;
      float3 diffuse = BRDF_GetDebugDiffuse(surface,surfaceToLight) * factor;

      float3 final = max(0.0f, diffuse);
      return float4(final, 0);
   #endif

   #ifdef SPECULAR_LIGHT_VIZ
      float3 factor = lightingColor.rgb * max(surfaceToLight.NdotL, 0) * shadow * lightBrightness;
      float3 spec = BRDF_GetDebugSpecular(surface, surfaceToLight) * factor;

      float3 final = max(0.0f, factor);
      return float4(final, 0);
   #endif

   #ifdef DETAIL_LIGHTING_VIZ
      float3 factor = lightingColor.rgb * max(surfaceToLight.NdotL, 0) * shadow * lightBrightness;
      float3 diffuse = BRDF_GetDebugDiffuse(surface,surfaceToLight) * factor;
      float3 spec = BRDF_GetDebugSpecular(surface,surfaceToLight) * factor;

      float3 final = max(0.0f, diffuse + spec);
      return float4(final,0);
   #endif

   //get directional light contribution   
   float3 lighting = getDirectionalLight(surface, surfaceToLight, lightingColor.rgb, lightBrightness, shadow);

   return float4(lighting, 0);
}
