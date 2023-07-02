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

#include "../shaderModelAutoGen.hlsl"
#include "../torque.hlsl"
//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct ConnectData
{
   float4 hpos              : TORQUE_POSITION;
   float2 uv                : TEXCOORD0;
   float3 normal            : TEXCOORD1;
   float3 worldPos          : TEXCOORD2;
   float4 projPos           : TEXCOORD3;
   float3x3 tangentMat      : TEXCOORD4;
};


//-----------------------------------------------------------------------------
// Uniforms                                                                  
//-----------------------------------------------------------------------------
TORQUE_UNIFORM_SAMPLER2D(deferredTex, 0);
TORQUE_UNIFORM_SAMPLER2D(reflectMap, 1);
TORQUE_UNIFORM_SAMPLER2D(refractBuff, 2);
TORQUE_UNIFORM_SAMPLERCUBE(skyMap, 3);
TORQUE_UNIFORM_SAMPLER2D(bumpMap,4);
TORQUE_UNIFORM_SAMPLER2D(foamMap, 5);

uniform float4x4    mvpInverse;
uniform float       elapsedTime;
uniform float3      ambientColor;
uniform float3      surfaceColor;
uniform float3      shoreColor;
uniform float3      depthColor;
uniform float3      colorExtinction;    // r = red extinction, g = green extinction, b = blue extinction.
uniform float3      refractionParams;   // x = IOR, y = Power, z = refractionScale.
uniform float2      waterProperties;    // x = clarity, y = transparency.
uniform float3      foamParameters;     // x = shore foam range, y = near shore foam, z = threshold for crest foam.
uniform float3      eyePosWorld;
uniform float3      inLightVec;
uniform float4      specParams;         // xy = specIntensity, z = shininess, w = shininesssExp.
uniform float4      rtParams1;
uniform float       distortionAmt;
uniform float       shoreFadeRange;
uniform float4	    sunColor;
uniform float       ambDensity;
uniform float       diffuseDensity;
uniform float       radianceFactor;
uniform float       farPlaneDist;
uniform float       normalIntensity;

//-----------------------------------------------------------------------------
// Functions                                                                  
//-----------------------------------------------------------------------------
float Fresnel(float2 refractionParams, float3 normal, float3 eyeVec)
{
    float IOR = refractionParams.x;
    float power = refractionParams.y;

    float ang = 1.0f - saturate(dot(normal, eyeVec));
    float fresnel = ang * ang;
    fresnel *= fresnel;
    fresnel *= ang;
    return saturate(fresnel * (1.0f - saturate(IOR)) + IOR - power);
}

float3 RefractionColor(float2 transparencyParams, float2 depthParams, float shore, float3 colorExtinction,
                        float3 refractionColor, float3 shoreColor, float3 surfaceColor, float3 depthColor)
{
    float clarity = transparencyParams.x;
    float transparency = transparencyParams.y;
    float depth = depthParams.x;
    float viewDepth = depthParams.y;

    float accruDepth = viewDepth * clarity;
    float accruExp = saturate(accruDepth / (2.5 * transparency));
    accruExp *= (1.0 - accruExp) * accruExp * accruExp + 1;

    surfaceColor = lerp(shoreColor, surfaceColor, saturate(depth / shore));
    float3 waterColor = lerp(surfaceColor, depthColor, saturate(depth / colorExtinction));

    refractionColor = lerp(refractionColor, surfaceColor * waterColor, saturate(accruDepth / transparency));
    refractionColor = lerp(refractionColor, depthColor, accruExp);
    refractionColor = lerp(refractionColor, depthColor * waterColor, saturate(depth / colorExtinction));

    return refractionColor;
}

float3 NdcToWorld(float4x4 mvpInverse, float3 ndc)
{
    float4 clipPos = float4(ndc.xy * 2.0f - 1.0f, ndc.z, 1.0f);

    float4 pos = mul(mvpInverse, clipPos);

    pos.xyz /= pos.w;
    return pos.xyz;
}

float FoamAmount(TORQUE_SAMPLER2D(FoamTex), float2 texCoord, float2 texCoord2, float2 ranges, float2 factors, float depth, float baseColor)
{
    float f1 = TORQUE_TEX2D( FoamTex, texCoord ).r;
    float f2 = TORQUE_TEX2D( FoamTex, texCoord2 ).r;

    return lerp(f1 * factors.x + f2 * factors.y, baseColor, smoothstep(ranges.x, ranges.y, depth));
}

//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
float4 main( ConnectData IN ) : TORQUE_TARGET0
{   
    float3 eyeVec = normalize( eyePosWorld - IN.worldPos );
    float3 worldPos = IN.worldPos;
    float2 ndcPos = float2(IN.projPos.xy / IN.projPos.w);

    // calculate normal.
    float3 bumpNorm = ( TORQUE_TEX2D( bumpMap, IN.uv ).rgb * 2.0 - 1.0 );
    bumpNorm = mul( bumpNorm, IN.tangentMat );
    
    float pixelDepth = IN.hpos.w / farPlaneDist;
    float2 deferredCoord = viewportCoordToRenderTarget( IN.hpos, rtParams1 );
    float depth = TORQUE_DEFERRED_UNCONDITION( deferredTex, deferredCoord ).w;

    float waterDepth = worldPos.z - depth;
    waterDepth = max(waterDepth, 0.0);
    //waterDepth *= farPlaneDist;
    float viewDepth = length(worldPos - depth);
    float fresnelTerm = Fresnel(refractionParams.xy, bumpNorm, eyeVec);
    
    bumpNorm = normalize(lerp(IN.normal, normalize(bumpNorm), normalIntensity));

    //-----------------------------------------------------------------------------
    // REFRACTION                                       
    //-----------------------------------------------------------------------------
    // refraction distort position.
    float2 distortPos = ndcPos;
    float refractionScale = refractionParams.z * min(waterDepth, 1.0);
    float2 delta = float2(  sin(elapsedTime + 1.0f * abs(depth)),
                            sin(elapsedTime + 5.0f * abs(depth)));
    distortPos.xy += bumpNorm.xy * delta * refractionScale;

    // Get refract color
    float3 trueRefractColor = hdrDecode( TORQUE_TEX2D( refractBuff, distortPos ) ).rgb;
    float2 depthParams = float2(waterDepth, viewDepth);
    float shoreRange = max(foamParameters.x, foamParameters.y) * 2.0;

    float3 refractionColor = RefractionColor(waterProperties, depthParams, shoreRange, 
                            colorExtinction, trueRefractColor, shoreColor, surfaceColor, depthColor );

    //-----------------------------------------------------------------------------
    // REFLECTION                                       
    //-----------------------------------------------------------------------------
    // reflection distorted pos.
    float3 reflectionVec = reflect( eyeVec, bumpNorm );
    distortPos = ndcPos;
    distortPos.xy += bumpNorm.xy * distortionAmt;
    // Get reflection map color.
    float3 reflectColor = fresnelTerm * TORQUE_TEXCUBE( skyMap, reflectionVec ).rgb * radianceFactor;
    reflectColor += TORQUE_TEX2D( reflectMap, distortPos ).rgb;

    //-----------------------------------------------------------------------------
    // SPECULAR REFLECTION                                        
    //-----------------------------------------------------------------------------
    float3 lightVec = inLightVec;
    float shineExp = specParams.w;
    float2 specIntensity = specParams.xy;
    float shininess = specParams.z;
    float3 mirrorEye = reflect(-eyeVec, bumpNorm);
    float dotSpec = saturate(dot(mirrorEye, lightVec) * 0.5 + 0.5);

    float3 specColor = (1.0 - fresnelTerm) * saturate(lightVec.z) * pow(dotSpec, specIntensity.y) * (shininess * shineExp + 0.2f) * sunColor.rgb;
    specColor += specColor * specIntensity.x * saturate(shininess - 0.05) * sunColor.rgb;

    //-----------------------------------------------------------------------------
    // FOAM                                       
    //-----------------------------------------------------------------------------
    float2 texCoord = IN.uv + elapsedTime * 0.01;
    float2 texCoord2 = (IN.uv + elapsedTime * 0.01) * -0.5;
    float foamEdge = max(foamParameters.x, foamParameters.y);
    float deepFoam = FoamAmount(TORQUE_SAMPLER2D_MAKEARG(foamMap), texCoord, texCoord2, float2(foamParameters.x, foamEdge),float2(1.0, 0.5), waterDepth, 0.0);
    float foam = FoamAmount(TORQUE_SAMPLER2D_MAKEARG(foamMap), texCoord, texCoord2, float2(0.0f, foamParameters.x),float2(0.75, 1.5), waterDepth, deepFoam);

    //-----------------------------------------------------------------------------
    // Output color calcs.                                                                        
    //-----------------------------------------------------------------------------
    float shoreFade = saturate(waterDepth * shoreFadeRange);
    float3 ambDifCol = ambientColor.rgb * ambDensity + saturate(dot(bumpNorm, inLightVec)) * diffuseDensity;
    trueRefractColor = lerp(trueRefractColor, reflectColor, fresnelTerm * (saturate(waterDepth / (foamParameters.x * 0.4))));
    trueRefractColor = lerp(trueRefractColor, shoreColor, 0.3 * shoreFade);
    float3 col = lerp(refractionColor, reflectColor, fresnelTerm);
    col = saturate(ambDifCol + col + max(specColor, foam * sunColor.rgb));
    col = lerp(trueRefractColor + specColor * shoreFade, col, shoreFade);

    //col.rgb = 0.5 + 2 * ambientColor + specColor + clamp(dot(bumpNorm, inLightVec), 0, 1) * 0.5;

    return hdrEncode(float4(col, 1.0));
}