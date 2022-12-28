#include "../../../postFX/postFx.hlsl"
#include "../../../shaderModel.hlsl"
#include "../../../shaderModelAutoGen.hlsl"
#include "../../../lighting.hlsl"
#include "../../../brdf.hlsl"


TORQUE_UNIFORM_SAMPLER2D(deferredBuffer, 0);
TORQUE_UNIFORM_SAMPLER2D(colorBuffer, 1);
TORQUE_UNIFORM_SAMPLER2D(matInfoBuffer, 2);
TORQUE_UNIFORM_SAMPLER2D(BRDFTexture, 3);
uniform float3 ambientColor;
uniform float4 rtParams0;
uniform float4 vsFarPlane;
uniform float4x4 cameraToWorld;
uniform float2 targetSize;

//cubemap arrays require all the same size. so shared mips# value
uniform float cubeMips;

uniform int numProbes;

TORQUE_UNIFORM_SAMPLERCUBEARRAY(specularCubemapAR, 4);
TORQUE_UNIFORM_SAMPLERCUBEARRAY(irradianceCubemapAR, 5);
TORQUE_UNIFORM_SAMPLER2D(WetnessTexture, 6);
TORQUE_UNIFORM_SAMPLER2D(stochNorm, 7);
#ifdef USE_SSAO_MASK
TORQUE_UNIFORM_SAMPLER2D(ssaoMask, 8);
uniform float4 rtParams7;
#endif
uniform float accumTime;
uniform float dampness;

uniform float4    probePosArray[MAX_PROBES];
uniform float4    refPosArray[MAX_PROBES];
uniform float4x4  worldToObjArray[MAX_PROBES];
uniform float4    refScaleArray[MAX_PROBES];
uniform float4    probeConfigData[MAX_PROBES];   //r,g,b/mode,radius,atten

uniform int skylightCubemapIdx;
uniform int SkylightDamp;

float4 RayMarch(float3 dir, float3 viewPos, float3 screenPos, float2 screenUV, float stepSize, int stepCount, float thickness)
{
	float mask = 0.0; 
	float3 samplePos = float3(screenPos.xy * 0.5 + 0.5, screenPos.z);
	float Depth;
    float DepthDiff = 0;
	
	float prevDepth = samplePos.z;
	float prevDepthDiff = 0.0;
	float3 prevSamplePos = samplePos;
	for(int i = 0; i < stepCount; i++)
	{
		Depth = TORQUE_DEFERRED_UNCONDITION( deferredBuffer, samplePos.xy).w;
		DepthDiff = samplePos.z - Depth;
		if(0.0 < DepthDiff)
		{
			if(Depth - prevDepth > thickness)
			{
				float blend = (prevDepthDiff - DepthDiff) / max(prevDepth, Depth) * 0.5 + 0.5;
				samplePos = lerp(prevSamplePos, samplePos, blend);
				mask = lerp(0.0, 1.0, blend);
				break;
			}
		}
		else
		{
			prevDepthDiff = -Depth;
			prevSamplePos = samplePos;
		}
		prevDepth = Depth;
		samplePos += dir * stepSize;
	}
	
	return float4(samplePos, mask);
}

float3 getView(float3 screenPos)
{
	float4 viewPos = mul(cameraToWorld, float4(screenPos, 1));
	return viewPos.xyz / viewPos.w;
}

float random (float2 uv) {
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453123); //simple random function
}

float4 main(PFXVertToPix IN) : SV_TARGET
{
   //unpack normal and linear depth 
   float4 normDepth = TORQUE_DEFERRED_UNCONDITION(deferredBuffer, IN.uv0.xy);

   //create surface
   float3 viewDir = TORQUE_TEX2D(stochNorm,IN.uv0.xy).rgb;
   Surface surface = createSurface(float4(viewDir,normDepth.w), TORQUE_SAMPLER2D_MAKEARG(colorBuffer),TORQUE_SAMPLER2D_MAKEARG(matInfoBuffer),
      IN.uv0.xy, eyePosWorld, IN.wsEyeRay, cameraToWorld);

   //early out if emissive
   if (getFlag(surface.matFlag, 0))
   {
      return float4(surface.albedo, 0);
   }
   
   // SSGI raymarch - need this to feed into the uv coord for cubemaps
   float3 screenPos = float3(IN.uv0.xy * 2 - 1, normDepth.a);
   
   float3 viewPos = getView(screenPos);
   
   // may be replaced with surface.R
   float3 dir = normalize(mul(cameraToWorld, float4(viewDir,1)).xyz); 
   
   float jit = random(IN.uv0.xy);
   float stepSize = (1.0 / (float)32);
   stepSize = stepSize * jit + stepSize;
    
   float rayMask = 0.0;
   float4 rayTrace = RayMarch(viewDir, surface.P, screenPos, IN.uv0.xy, stepSize, 32, 1.0);
   float3 hitUV = rayTrace.xyz;
   rayMask = rayTrace.w; 

   #ifdef USE_SSAO_MASK
      float ssao =  1.0 - TORQUE_TEX2D( ssaoMask, viewportCoordToRenderTarget( IN.uv0.xy, rtParams6 ) ).r;
      surface.ao = min(surface.ao, ssao);  
   #endif

   float alpha = 1;
   float wetAmmout = 0;
#if SKYLIGHT_ONLY == 0
   int i = 0;
   float blendFactor[MAX_PROBES];
   float blendSum = 0;
   float blendFacSum = 0;
   float invBlendSum = 0;
   float probehits = 0;
   //Set up our struct data
   float contribution[MAX_PROBES];
   
   float blendCap = 0;
   if (alpha > 0)
   {
      //Process prooooobes
      for (i = 0; i < numProbes; i++)
      {
         contribution[i] = 0.0;

         float atten =1.0-(length(eyePosWorld-probePosArray[i].xyz)/maxProbeDrawDistance);
         if (probeConfigData[i].r == 0) //box
         {
            contribution[i] = defineBoxSpaceInfluence(surface.P, worldToObjArray[i], probeConfigData[i].b)*atten;
         }
         else if (probeConfigData[i].r == 1) //sphere
         {
            contribution[i] = defineSphereSpaceInfluence(surface.P, probePosArray[i].xyz, probeConfigData[i].g)*atten;
         }

            if (contribution[i]>0.0)
               probehits++;
         else
            contribution[i] = 0.0;

         if (refScaleArray[i].w>0)
            wetAmmout += contribution[i];
         else
            wetAmmout -= contribution[i];

         blendSum += contribution[i];
         blendCap = max(contribution[i],blendCap);
      }
      if (wetAmmout<0) wetAmmout =0;
       if (probehits > 0.0)
	   {
         invBlendSum = (probehits - blendSum)/probehits; //grab the remainder 
         for (i = 0; i < numProbes; i++)
         {
               blendFactor[i] = contribution[i]/blendSum; //what % total is this instance
               blendFactor[i] *= blendFactor[i]/invBlendSum;  //what should we add to sum to 1
               blendFacSum += blendFactor[i]; //running tally of results
         }

         for (i = 0; i < numProbes; i++)
         {
            //normalize, but in the range of the highest value applied
            //to preserve blend vs skylight
            contribution[i] = blendFactor[i]/blendFacSum*blendCap;
         }
      }
   }
   for (i = 0; i < numProbes; i++)
   {
      float contrib = contribution[i];
      if (contrib > 0.0f)
      {
         alpha -= contrib;
      }
   }
#endif

   float3 irradiance = float3(0, 0, 0);
   float3 specular = float3(0, 0, 0);

   if (SkylightDamp>0)
      wetAmmout += alpha;
   dampen(surface, TORQUE_SAMPLER2D_MAKEARG(WetnessTexture), accumTime, wetAmmout*dampness);
   
   float3 sampleColor = float3(0.0, 0.0, 0.0);
   float lod = roughnessToMipLevel(surface.roughness, cubeMips);
#if SKYLIGHT_ONLY == 0
   for (i = 0; i < numProbes; i++)
   {
      float contrib = contribution[i];
      if (contrib > 0.0f)
      {
         int cubemapIdx = probeConfigData[i].a;

         irradiance += TORQUE_TEXCUBEARRAYLOD(irradianceCubemapAR, hitUV, cubemapIdx, 0).xyz * contrib;
         specular += TORQUE_TEXCUBEARRAYLOD(specularCubemapAR, hitUV, cubemapIdx, lod).xyz * contrib;
      }
   }
#endif
   if(skylightCubemapIdx != -1 && alpha >= 0.001)
   {
      irradiance = lerp(irradiance,TORQUE_TEXCUBEARRAYLOD(irradianceCubemapAR, hitUV, skylightCubemapIdx, 0).xyz,alpha);
      specular = lerp(specular,TORQUE_TEXCUBEARRAYLOD(specularCubemapAR, hitUV, skylightCubemapIdx, lod).xyz,alpha);
   }
   
   //energy conservation
   float3 F = FresnelSchlickRoughness(surface.NdotV, surface.f0, surface.roughness);
   float3 kS = F;
   float3 kD = 1.0f - kS;
   kD *= 1.0f - surface.metalness;

   float2 envBRDF = TORQUE_TEX2DLOD(BRDFTexture, float4(surface.NdotV, surface.roughness,0,0)).rg;
   specular = specular * (F * envBRDF.x + surface.f90 * envBRDF.y);
   float3 diffuse = irradiance * surface.baseColor.rgb;
   
   float3 ambient = (kD * diffuse + specular) * surface.ao;

	sampleColor = TORQUE_TEX2D(colorBuffer, hitUV.xy).rgb;
	sampleColor = lerp(ambient, sampleColor, alpha);
	return float4(sampleColor,1);
}
