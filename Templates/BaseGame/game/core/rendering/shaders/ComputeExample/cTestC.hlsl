struct Pixel
{
    int colour;
};

RWStructuredBuffer<Pixel> BufferOut;

void writeToPixel(int2 texel, float3 colour)
{
    // 256 x 256 size of texture.
	uint index = (texel.x + texel.y * 256);
	
	int ired   = (int)(clamp(colour.r,0,1) * 255);
	int igreen = (int)(clamp(colour.g,0,1) * 255) << 8;
	int iblue  = (int)(clamp(colour.b,0,1) * 255) << 16;
	
    BufferOut[index].colour = ired + igreen + iblue;
}

[numthreads(1, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID )
{
    float4 col = float4(0.0, 0.0, 0.0, 1.0);
    int2 texel = int2(dispatchThreadID.x, dispatchThreadID.y);

    // 256 is here because hlsl cannot bring in the number work groups.
    col.r = float(texel.x) / 256;
    col.g = float(texel.y) / 256;

    writeToPixel(texel, col.rgb);
}