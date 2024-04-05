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
in vec4 color;

out vec4 OUT_col;

uniform vec2 sizeUni;
uniform vec2 rectCenter;
uniform vec2 oneOverViewport;
uniform float radius;
uniform float borderSize;
uniform vec4 borderCol;

float RoundedRectSDF(vec2 p, vec2 size, float radius)
{
    // Calculate distance to each side of the rectangle
    vec2 dist = abs(p) - size + vec2(radius, radius);

    // Compute the distance to the rounded corners
    float cornerDist = length(max(dist, 0.0));

    // Return the minimum distance (negative inside, positive outside)
    return min(max(dist.x, dist.y), 0.0) + cornerDist - radius; 
}
 
void main()
{  
    vec2 p = gl_FragCoord.xy;
    
    vec2 halfSize = sizeUni * 0.5;
    p -= rectCenter; 

    // alpha
    vec4 toColor = vec4(0.0, 0.0, 0.0, 0.0);

    float cornerRadius = radius;
    float borderFinalSize = borderSize;

    if(cornerRadius > 0.0 || borderSize > 0.0)
    {
        float sdf = RoundedRectSDF(p, halfSize, cornerRadius);

        float borderSoftening = clamp(borderFinalSize *0.5, 0.0, 2.0);
        float edgeSoftening = clamp(radius * 0.5, 0.0, 2.0);

        float alpha = 1.0 - smoothstep(0.0, edgeSoftening, sdf); 
        float borderAlpha = 1.0 - smoothstep(borderFinalSize - borderSoftening, borderFinalSize, abs(sdf));

        OUT_col = mix(toColor, mix(IN.color, borderCol, borderAlpha), alpha);
    }
    else
    {
        OUT_col = color;
    }
}