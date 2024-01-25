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
  
uniform float  lightBrightness;
uniform float3 lightDirection;

uniform float4 lightColor;
uniform float4 lightAmbient; 

uniform float shadowSoftness;
 
uniform float4 zNearFarInvNearFar;
uniform float4 lightMapParams;

uniform float4 overDarkPSSM;
uniform float2 fadeStartLength;
uniform float2 atlasScale;
uniform float4x4 eyeMat; 
uniform float4x4 cameraToWorld; 

// Static Shadows  
uniform float4x4 worldToLightProj;
uniform float4 farPlaneScalePSSM;
uniform float4 cascadeSplits;
uniform float4 cascadeData[4];
uniform float2 atlasOffset[4];

#define USEPROJECTION 1
//#define PSSM_DEBUG_RENDER 
 
float4 AL_VectorLightShadowCast( TORQUE_SAMPLER2D(sourceShadowMap),
                                float4x4 worldToLightProj,
                                float3 worldPos,
                                float3 worldNormal,
                                float dotNL,
                                float depth)
{  
      float4 projectionPos = mul(worldToLightProj, float4(worldPos,1));

      int cascadeId = -1;  
      for(int i = 3; i >= 0; i--)
      {
      #if USEPROJECTION
         float3 cascadePos = projectionPos.xyz / projectionPos.w;

         cascadePos.xy *= cascadeData[i].xy;
         cascadePos.xy += cascadeData[i].zw;
         cascadePos.z *= farPlaneScalePSSM[i];
         if (  cascadePos.x > -0.99 && cascadePos.x < 0.99 && 
            cascadePos.y > -0.99 && cascadePos.y < 0.99 &&
            cascadePos.z < 1.0 )
            cascadeId = i; 
            
		#else
			if(depth <= cascadeSplits[i])
			cascadeId = i;
		#endif  
      }
 
      // get shadow bias.  

      float shadowBias = getShadowBias(0.02, worldNormal, worldPos, worldPos + lightDirection); 
 
      // get shadow pos offset / this is normal bias.  
      float pixelSize = 1.0f / 1024.0f; 
      float texelSize = 2.0f / 1024.0f; 
      float3 shadDepthOffset = getShadowPosOffset(dotNL, texelSize, worldNormal, 0.2) / farPlaneScalePSSM[cascadeId];
        
      float4 pxlPosLightProj = mul(worldToLightProj, float4(worldPos+worldNormal*shadDepthOffset*shadowBias,1)); 
   
      float3 debugColor = float3(0,0,0);  
      #ifdef NO_SHADOW 
         debugColor = float3(1.0,1.0,1.0);
      #endif
	  
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
  
      float3 newShadowCoord = pxlPosLightProj.xyz / pxlPosLightProj.w;
    
      newShadowCoord.xy *= cascadeData[cascadeId].xy;
      newShadowCoord.xy += cascadeData[cascadeId].zw; 
                
      // Convert to texcoord space
      newShadowCoord.xy = 0.5 * newShadowCoord.xy + float2(0.5, 0.5);   
      newShadowCoord.y = 1.0f - newShadowCoord.y; 
           
      // Move around inside of atlas  
      newShadowCoord.xy *= atlasScale; 
      newShadowCoord.xy += atlasOffset[cascadeId];
        
      newShadowCoord.z *= farPlaneScalePSSM[cascadeId];
          
      float lightSize = 2.0f;
      float scaledSoftness = shadowSoftness * float(cascadeId); 
      return float4(debugColor, softShadow_filterPCSS(TORQUE_SAMPLER2D_MAKEARG(shadowMap), newShadowCoord, shadowSoftness, lightSize) );  
};   
   
float4 main(FarFrustumQuadConnectP IN) : SV_TARGET 
{             
   //unpack normal and linear depth  
   float4 normDepth = TORQUE_DEFERRED_UNCONDITION(deferredBuffer, IN.uv0);
      
   //create surface
   Surface surface = createSurface( normDepth, TORQUE_SAMPLER2D_MAKEARG(colorBuffer),TORQUE_SAMPLER2D_MAKEARG(matInfoBuffer),
                                    IN.uv0, eyePosWorld, IN.wsEyeRay, cameraToWorld);
   if (getFlag(surface.matFlag, 2))
   {
      return surface.baseColor; 
   }                           
   //create surface to light                           
   SurfaceToLight surfaceToLight = createSurfaceToLight(surface, -lightDirection);
    
   //light color might be changed by PSSM_DEBUG_RENDER
   float3 lightingColor = lightColor.rgb;
      
   float shadow = 1.0;   
   #ifndef NO_SHADOW 
   if (getFlag(surface.matFlag, 0)) //also skip if we don't recieve shadows
   {  
      // Fade out the shadow at the end of the range.
      float4 zDist = (zNearFarInvNearFar.x + zNearFarInvNearFar.y * surface.depth);
      float fadeOutAmt = ( zDist.x - fadeStartLength.x ) * fadeStartLength.y;

      float4 shadowed_colors = AL_VectorLightShadowCast( TORQUE_SAMPLER2D_MAKEARG(shadowMap), 
                                                         worldToLightProj, 
                                                         surface.P, 
                                                         surface.N,
                                                         surfaceToLight.NdotL,
                                                         surface.depth);

      shadow *= shadowed_colors.a;
	  
      #ifdef PSSM_DEBUG_RENDER
	     lightingColor = shadowed_colors.rgb;
      #endif

      shadow = lerp( shadow, 1.0, saturate( fadeOutAmt ) );

      #ifdef PSSM_DEBUG_RENDER
         if ( fadeOutAmt > 1.0 )
            lightingColor = 1.0;
      #endif
   }
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
