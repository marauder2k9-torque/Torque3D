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

#include "../torque.hlsl"

//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct VertData
{
   float3 position          : POSITION;
};

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
uniform float4x4 modelMat;
uniform float4x4 modelMatInverse;
uniform float4x4 modelview;
uniform float4   waveData[3]; // xy - direction, z = steepness, w = wavelength;
uniform float    elapsedTime;
uniform float3   eyePosWorld;
uniform float    textureTile;

//-----------------------------------------------------------------------------
// Functions                                                                  
//-----------------------------------------------------------------------------
float3 CalcGerstnerWave(float4 wave, float3 pos, inout float3 tangent, inout float3 binormal)
{
    float steepness = wave.z;
    float speed = wave.w;
    float k = 2 * M_PI_F / speed;
    float c = sqrt(9.8 / k);
    float2 d = normalize(wave.xy);
    float f = k * ( dot(d, pos.xy) - c * elapsedTime);
    float a = steepness / k;

    tangent += float3(-d.x * d.x * (steepness * sin(f)), d.x * (steepness * cos(f)), -d.x * d.y * (steepness * sin(f)));
    binormal += float3(-d.x * d.y * (steepness * sin(f)), d.y * (steepness * cos(f)), d.y * d.y * (steepness * sin(f)));

    return float3(d.x * (a * cos(f)),a * sin(f),d.y * (a * cos(f)));

}

float4 ClipToScreen(float4 pos)
{
    float4 o = pos * 0.05f;
    o.xy += o.w;
    o.zw = pos.zw;
    return o;
}

//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
ConnectData main( VertData IN )
{	
    ConnectData OUT;

    // use projection matrix for reflection / refraction texture coords
    float4x4 texGen = { 0.5,  0.0,  0.0,  0.5,
                        0.0, -0.5,  0.0,  0.5,
                        0.0,  0.0,  1.0,  0.0,
                        0.0,  0.0,  0.0,  1.0 };

    float4 inPos = float4(IN.position, 1.0);
    float4 modelPos = float4( IN.position, 1.0);
    float3 wldPos = mul(modelMat, modelPos).xyz;
    float dist = distance( wldPos, eyePosWorld );

    float3 binormal = float3( 1, 0, 0 );
    float3 tangent = float3( 0, 1, 0 );
    for ( int i = 0; i < 3; i++ )
    {
        wldPos += CalcGerstnerWave(waveData[i], wldPos, tangent, binormal);
    }

    modelPos = mul(modelMatInverse, float4(wldPos, 1.0));

    float3 normal = normalize(cross(binormal, tangent));

    float3x3 worldToTangent;
    worldToTangent[0] = binormal;
    worldToTangent[1] = tangent;
    worldToTangent[2] = normal;

    OUT.tangentMat = worldToTangent;
    OUT.uv = wldPos.xy * 0.05 * textureTile;
    OUT.normal = normal;
    OUT.worldPos = wldPos;
    OUT.hpos = mul(modelview, float4(modelPos.xyz, 1.0));
    OUT.projPos = mul(texGen, OUT.hpos);
    
    return OUT;
}