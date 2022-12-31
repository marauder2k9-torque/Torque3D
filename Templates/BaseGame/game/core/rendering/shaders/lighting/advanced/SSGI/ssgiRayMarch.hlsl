#include "../../../postFX/postFx.hlsl"
#include "../../../shaderModel.hlsl"
#include "../../../shaderModelAutoGen.hlsl"
#include "../../../lighting.hlsl"
#include "../../../brdf.hlsl"


TORQUE_UNIFORM_SAMPLER2D(deferredBuffer, 0);
TORQUE_UNIFORM_SAMPLER2D(colorBuffer, 1);
TORQUE_UNIFORM_SAMPLER2D(matInfoBuffer, 2);
TORQUE_UNIFORM_SAMPLER2D(stochNorm, 3);
uniform float4 rtParams0;
uniform float4 vsFarPlane;
uniform float4x4 cameraToWorld;
uniform float4x4 invCameraMat;

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
				float blend = (prevDepthDiff - DepthDiff) / max(prevDepth, Depth) * 0.5 + 0.5;
				samplePos = lerp(prevSamplePos, samplePos, blend);
				mask = blend;
				break;
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
   Surface surface = createSurface(normDepth, TORQUE_SAMPLER2D_MAKEARG(colorBuffer),TORQUE_SAMPLER2D_MAKEARG(matInfoBuffer),
      IN.uv0.xy, eyePosWorld, IN.wsEyeRay, cameraToWorld);

   //early out if emissive
   if (getFlag(surface.matFlag, 0))
   {
      return float4(surface.albedo, 0);
   }
   
   // SSGI raymarch - need this to feed into the uv coord for cubemaps
   float3 screenPos = float3(IN.uv0.xy * 2 - 1, normDepth.a);
   
   float3 viewPos = getView(screenPos);
   
   float jit = random(IN.uv0.xy);
   float stepSize = (1.0 / (float)16);
   stepSize = stepSize * jit + stepSize;
   float3 dir = normalize(mul(float4(viewDir,1), cameraToWorld)).xyz;
   float rayMask = 0.0;
   float4 rayTrace = RayMarch(dir, surface.P, screenPos, IN.uv0.xy, stepSize * 1.8, 16, 1.0);  
   float3 hitUV = rayTrace.xyz;
   rayMask = rayTrace.w; 
   rayMask = dot(viewDir,mul( float4(surface.N, 1),invCameraMat).xyz);

	float3 sampleColor = TORQUE_TEX2D(colorBuffer, hitUV.xy).rgb * 5.0;
	return float4(sampleColor,rayMask);
}
