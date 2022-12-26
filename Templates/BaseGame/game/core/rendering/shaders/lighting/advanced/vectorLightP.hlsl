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
#include "softShadow.hlsl"

TORQUE_UNIFORM_SAMPLER2D(deferredBuffer, 0);
TORQUE_UNIFORM_SAMPLER2DARRAY(shadowMap, 1);
TORQUE_UNIFORM_SAMPLER2D(colorBuffer, 2);
TORQUE_UNIFORM_SAMPLER2D(matInfoBuffer, 3);

uniform float  lightBrightness;
uniform float3 lightDirection;

uniform float4 lightColor;
uniform float4 lightAmbient;

uniform float4 zNearFarInvNearFar;

uniform float4 lightParams;
 
uniform float2 fadeStartLength;
uniform float4x4 eyeMat;
uniform float4x4 cameraToWorld; 

// Shadows       
uniform float4x4 worldToLightProj;
uniform float4 cascadeSplits; 
uniform float4 cascadeOffsets[4];  
uniform float4 cascadeScales[4];
uniform float shadowSoftness;
uniform float shadowRes;
uniform float shadowMapSize;
uniform int shadowMethod;

#define USEPROJECTION 1
#define USEBLEND 0
#define ERR 0.0005f

float3 GetShadowPosOffset(TORQUE_SAMPLER2DARRAY(sourceShadowMap), in float nDotL, in float3 normal)
{
    float2 shadowMapSize;
    float numSlices;
    TORQUE_TEX2DGETSIZEZARRAY(sourceShadowMap, shadowMapSize.x, shadowMapSize.y, numSlices);
    float texelSize = 2.0f / shadowMapSize.x;
    float nmlOffsetScale = saturate(1.0f - nDotL);
    return texelSize * 0.001f * nmlOffsetScale * normal;
}


float4 AL_VectorLightShadowCast( TORQUE_SAMPLER2DARRAY(sourceShadowMap),
								float2 screenPos,
                                float2 texCoord,
                                float4x4 worldToLightProj,
                                float3 worldPos,
								float4 cascadeOffsets[4],
								float4 cascadeScales[4],
                                float dotNL,
								float3 normal,
								float depth,
								int shadowMethod)
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
			float3 cascadePos = float3(pxlPosLightProj.xy / pxlPosLightProj.w, pxlPosLightProj.z / pxlPosLightProj.w);
			cascadePos *= cascadeScales[i].xyz;
			cascadePos += cascadeOffsets[i].xyz;
			cascadePos = abs(cascadePos);
			if(all(cascadePos <= 0.99f))
                cascadeId = i;
		#else
			if(depth <= cascadeSplits[i])
			cascadeId = i;
		#endif  
	  }         
	         
	  float3 shadowPos = float3(baseShadowCoord, distToLight);
	  
	  float3 shadowPosDX = ddx_fine(shadowPos);
	  float3 shadowPosDY = ddy_fine(shadowPos);
	  
	  shadowPosDX *= cascadeScales[cascadeId].xyz;
	  shadowPosDY *= cascadeScales[cascadeId].xyz;
	  
	  shadowPos *= cascadeScales[cascadeId].xyz;
	  shadowPos += cascadeOffsets[cascadeId].xyz;
	  
	  // Convert to texcoord space
	  // convert all floats to between [0,1]
      shadowPos.xy = 0.5 * shadowPos.xy + float2(0.5, 0.5);
      shadowPos.y = 1.0f - shadowPos.y;
	   
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
	  
      float4 shadowSample = float4(debugColor, SampleShadow(  
	  TORQUE_SAMPLER2D_MAKEARG(sourceShadowMap),
	  screenPos, 
	  texCoord, 
	  shadowPos,
	  shadowPosDX,
	  shadowPosDY,
	  shadowRes,
	  shadowMapSize,
	  lightParams.z,
	  cascadeId,
	  shadowSoftness,
	  lightParams.y,
	  dotNL,
	  shadowMethod));
	  
	  
	  #if USEBLEND      
	      
	  if(cascadeId != 3)
	  {
	  
		  float nextSplit = cascadeSplits[cascadeId];
		  float splitSize = cascadeId == 0 ? nextSplit : nextSplit - cascadeSplits[cascadeId - 1];
          float fadeFactor = (nextSplit - depth) / splitSize;
		  
		  #if USEPROJECTION
			float3 cascadePos = float3(pxlPosLightProj.xy / pxlPosLightProj.w, pxlPosLightProj.z / pxlPosLightProj.w);
			cascadePos *= cascadeScales[cascadeId].xyz;
			cascadePos += cascadeOffsets[cascadeId].xyz;
			
			// Convert to texcoord space
			cascadePos.xy = 0.5 * cascadePos.xy + float2(0.5, 0.5);
			cascadePos = abs(cascadePos * 2.0f - 1.0f);
			
			float distToEdge = 1.0f -  max(max(cascadePos.x, cascadePos.y), cascadePos.z);
            fadeFactor = max(distToEdge, fadeFactor);
		  #endif
	   
		  float alpha = 0.01;  
	      float3 nextCascadePos = float3(baseShadowCoord, distToLight);
		  
		  float3 nextShadowPosDX = ddx_fine(nextCascadePos);
		  float3 nextShadowPosDY = ddy_fine(nextCascadePos);
		  
		  nextShadowPosDX *= cascadeScales[cascadeId + 1].xyz;
		  nextShadowPosDY *= cascadeScales[cascadeId + 1].xyz;
		     
		  nextCascadePos *= cascadeScales[cascadeId + 1].xyz;
		  nextCascadePos += cascadeOffsets[cascadeId + 1].xyz;
		  
		  nextCascadePos.xy = 0.5 * nextCascadePos.xy + float2(0.5, 0.5);
		  nextCascadePos.y = 1.0f - nextCascadePos.y;
		   
		  float nextShadow = SampleShadow(  
		  				  TORQUE_SAMPLER2D_MAKEARG(sourceShadowMap),
		  				  screenPos, 
		  				  texCoord, 
		  				  nextCascadePos,
						  nextShadowPosDX,
						  nextShadowPosDY,
						  shadowRes, 
						  shadowMapSize,
						  lightParams.z,
		  				  cascadeId + 1,
		  				  shadowSoftness,
		  				  lightParams.y,
		  				  dotNL,
		  				  shadowMethod);
		  				  
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

      // Fade out the shadow at the end of the range.
      float4 zDist = (zNearFarInvNearFar.x + zNearFarInvNearFar.y * surface.depth);
      float fadeOutAmt = ( zDist.x - fadeStartLength.x ) * fadeStartLength.y;

      float4 shadowed_colors = AL_VectorLightShadowCast( 
	  TORQUE_SAMPLER2D_MAKEARG(shadowMap), 
	  IN.hpos.xy, 
	  IN.uv0.xy,
	  worldToLightProj, 
	  surface.P, 
	  cascadeOffsets,
	  cascadeScales,
	  surfaceToLight.NdotL, 
	  surface.N, 
	  surface.depth,
	  shadowMethod);
    
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
