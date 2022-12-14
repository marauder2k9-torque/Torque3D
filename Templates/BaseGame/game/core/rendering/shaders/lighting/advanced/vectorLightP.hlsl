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
uniform float4 scaleX;
uniform float4 scaleY;
uniform float4 offsetX;
uniform float4 offsetY;

float4 AL_VectorLightShadowCast( TORQUE_SAMPLER2D(sourceShadowMap),
								TORQUE_SAMPLER2DCMP(sourceShadowMapCMP),
								float2 screenPos,
                                float2 texCoord,
                                float4x4 worldToLightProj,
                                float3 worldPos,
                                float4 scaleX,
                                float4 scaleY,
                                float4 offsetX,
                                float4 offsetY,
                                float4 farPlaneScalePSSM,
                                float dotNL)
{
      // Compute shadow map coordinate
	  // Distance to light, in shadowmap space
	  float4 pxlPosLightProj = mul(worldToLightProj, float4(worldPos,1));
	  float2 baseShadowCoord = pxlPosLightProj.xy / pxlPosLightProj.w;   
      float distToLight = pxlPosLightProj.z / pxlPosLightProj.w;
	  
	  float4 farPlaneDists = distToLight.xxxx;
	  farPlaneDists *= farPlaneScalePSSM;
	  
	  float4 cascadeX = (baseShadowCoord.xxxx * scaleX) + offsetX;
	  float4 cascadeY = (baseShadowCoord.yyyy * scaleY) + offsetY;
	  
	  float4 inCascadeX = abs(cascadeX) <= 1.0;
	  float4 inCascadeY = abs(cascadeY) <= 1.0;
	  float4 inCascade = inCascadeX * inCascadeY;
	  
	  float4 cascadeMask; // = inCascade;
	  //cascadeMask.yzw = (1.0 - cascadeMask.x) * cascadeMask.yzw;
	  //cascadeMask.zw = (1.0 - cascadeMask.y) * cascadeMask.zw;
	  //cascadeMask.w = (1.0 - cascadeMask.z) * cascadeMask.w;
	  
	  if (  cascadeX.x > -0.99 && cascadeX.x < 0.99 && 
            cascadeY.x > -0.99 && cascadeY.x < 0.99 &&
            farPlaneDists.x < 1.0 )
         cascadeMask = float4(1, 0, 0, 0);

      else if (   cascadeX.y > -0.99 && cascadeX.y < 0.99 &&
                  cascadeY.y > -0.99 && cascadeY.y < 0.99 && 
                  farPlaneDists.y < 1.0 )
         cascadeMask = float4(0, 1, 0, 0);

      else if (   cascadeX.z > -0.99 && cascadeX.z < 0.99 && 
                  cascadeY.z > -0.99 && cascadeY.z < 0.99 && 
                  farPlaneDists.z < 1.0 )
         cascadeMask = float4(0, 0, 1, 0);
         
      else
         cascadeMask = float4(0, 0, 0, 1);
	  
	  /// should be passed to shadowfilter.
	  float bestCascade = dot(cascadeMask, float4(0.0, 1.0, 2.0, 3.0));
	  
	  float3 debugColor = float3(0,0,0);
   
      #ifdef NO_SHADOW
         debugColor = float3(1.0,1.0,1.0);
      #endif
	  
      #ifdef PSSM_DEBUG_RENDER
         if ( cascadeMask.x > 0 )
            debugColor += float3( 1, 0, 0 );
         else if ( cascadeMask.y > 0 )
            debugColor += float3( 0, 1, 0 );
         else if ( cascadeMask.z > 0 )
            debugColor += float3( 0, 0, 1 );
         else if ( cascadeMask.w > 0 )
            debugColor += float3( 1, 1, 0 );
      #endif
	  
	  float3 uvd;
	  uvd.x = dot(cascadeX, cascadeMask);
	  uvd.y = dot(cascadeY, cascadeMask);
	  uvd.z = pxlPosLightProj.z;
	  
	  uvd.xy = 0.5 * uvd.xy + 0.5;
	  uvd.y = 1.0 - uvd.y;
	  
	  float2 aOffset;
      aOffset.x = dot(cascadeMask, atlasXOffset);
      aOffset.y = dot(cascadeMask, atlasYOffset);
	  
	  uvd.xy *= atlasScale;
	  uvd.xy += aOffset;
	  
	  // Each split has a different far plane, take this into account.
      float farPlaneScale = dot( farPlaneScalePSSM, cascadeMask );
      distToLight *= farPlaneScale;
	  
      float4 shadowSample = float4(debugColor, softShadow_filter(  
	  TORQUE_SAMPLER2D_MAKEARG(sourceShadowMap),
	  TORQUE_SAMPLER2D_MAKEARG(sourceShadowMapCMP),
	  screenPos, 
	  texCoord, 
	  uvd.xy, 
	  bestCascade,
	  shadowSoftness,
      distToLight, 
	  dotNL, 
	  dot( cascadeMask, overDarkPSSM ) ) );
	  
	  shadowSample.a = saturate(shadowSample.a + 1.0 - any(cascadeMask));
	  
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
	  TORQUE_SAMPLER2D_MAKEARG(shadowMapCmp),
	  IN.hpos.xy, 
	  IN.uv0.xy,
	  worldToLightProj, 
	  surface.P, 
	  scaleX, 
	  scaleY, 
	  offsetX, 
	  offsetY,
      farPlaneScalePSSM, 
	  surfaceToLight.NdotL);

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
