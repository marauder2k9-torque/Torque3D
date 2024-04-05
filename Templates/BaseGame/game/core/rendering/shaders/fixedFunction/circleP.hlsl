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

#include "../shaderModel.hlsl"

struct Conn
{
   float4 HPOS             : TORQUE_POSITION;
   float4 color            : COLOR;
};

uniform float2 sizeUni;
uniform float radius;
uniform float2 rectCenter;
uniform float borderSize;
uniform float4 borderCol;

float circle(float2 p, float2 center, float r)
{
    return length(p - center) - r;
}
 
float4 main(Conn IN) : TORQUE_TARGET0
{   
    float borderFinalSize = borderSize;
    float circleRadius = radius;

    float distance = circle(IN.HPOS.xy, rectCenter, circleRadius);
    
    float4 toColor = float4(0.0, 0.0, 0.0, 0.0);

    float edgeSoftening = clamp(circleRadius *0.5, 0.0, 0.5);
    float borderSoftening = clamp(borderFinalSize *0.5, 0.0, 2.0);

    float alpha = 1.0 - smoothstep(0.0, edgeSoftening, distance); 
    float borderAlpha = 1.0 - smoothstep(borderFinalSize - borderSoftening, borderFinalSize, abs(distance));

    return lerp(toColor, lerp(IN.color, borderCol, borderAlpha), alpha);
}