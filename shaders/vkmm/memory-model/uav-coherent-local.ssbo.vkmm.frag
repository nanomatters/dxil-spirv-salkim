RWByteAddressBuffer Buf : register(u0);

float4 main(float4 pos : SV_Position) : SV_Target
{
	uint offset = uint(pos.x) * 4;
	Buf.Store(offset, uint(pos.y));
	DeviceMemoryBarrier();
	return Buf.Load(offset + 4);
}
