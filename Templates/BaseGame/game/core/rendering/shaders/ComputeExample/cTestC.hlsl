
RWTexture2D<unorm float4> BufferOut;

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    float4 col = float4(0.0, 0.0, 0.0, 1.0);

    
    BufferOut[id.xy] = float4(id.x & id.y, (id.x & 15)/15.0, (id.y & 15)/15.0, 0.0);
}