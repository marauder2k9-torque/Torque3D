#include "../../../postFX/postFx.hlsl"
#include "../../../shaderModel.hlsl"
#include "../../../shaderModelAutoGen.hlsl"
#include "../../../lighting.hlsl"
#include "../../../brdf.hlsl"

TORQUE_UNIFORM_SAMPLER2D(deferredBuffer, 0);
TORQUE_UNIFORM_SAMPLER2D(colorBuffer, 1);
TORQUE_UNIFORM_SAMPLER2D(matInfoBuffer, 2);

uniform float4 rtParams0;
uniform float4 vsFarPlane;
uniform float4x4 cameraToWorld;
uniform float2 targetSize;

float3 getPerpendicularVector(float3 u) {
    float3 a = abs(u);
    uint xm = ((a.x - a.y)<0 && (a.x - a.z)<0) ? 1 : 0;
    uint ym = (a.y - a.z)<0 ? (1 ^ xm) : 0;
    uint zm = 1 ^ (xm | ym);
    return cross(u, float3(xm, ym, zm));
}

float3 getCosHemisphereSample(float2 rand, float3 hitNorm) {
    // Cosine weighted hemisphere sample from RNG
    float3 bitangent = getPerpendicularVector(hitNorm);
    float3 tangent = cross(bitangent, hitNorm);
    float r = sqrt(rand.x);
    float phi = 2.0f * M_PI_F * rand.y;

    // Get our cosine-weighted hemisphere lobe sample direction
    return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * sqrt(1 - rand.x);
}

float GradientNoise(float2 screenPos)
{
	float3 mag = float3(0.06711056f, 0.00583715f, 52.9829189f);
	return frac(mag.z * frac(dot(screenPos, mag.xy)));
}


float4 main(PFXVertToPix IN) : SV_TARGET
{
   //unpack normal and linear depth 
   float4 normDepth = TORQUE_DEFERRED_UNCONDITION(deferredBuffer, IN.uv0.xy);

   //create surface
   Surface surface = createSurface(normDepth, TORQUE_SAMPLER2D_MAKEARG(colorBuffer),TORQUE_SAMPLER2D_MAKEARG(matInfoBuffer),
      IN.uv0.xy, eyePosWorld, IN.wsEyeRay, cameraToWorld);
   
   float2 uv = IN.uv0.xy;
   float2 pos = uv * targetSize;
   
   float2 rand = float2(GradientNoise(pos.x), GradientNoise(pos.y));
   float3 stochastic = getCosHemisphereSample(rand, surface.N);
   
   return normalize(float4(stochastic,1.0));
}
	  