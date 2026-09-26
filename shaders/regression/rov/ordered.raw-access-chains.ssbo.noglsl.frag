RasterizerOrderedByteAddressBuffer Raw : register(u0);
RasterizerOrderedStructuredBuffer<uint2> Structured : register(u1);
RWByteAddressBuffer Ordinary : register(u2);

[earlydepthstencil]
void main(float4 pos : SV_Position)
{
    uint index = uint(pos.x);
    if (index & 1)
        Raw.Store(index * 4, Raw.Load(index * 4) + 1);
    Structured[index] = Structured[index] + uint2(1, 2);
    Ordinary.Store(index * 4, index);
}
