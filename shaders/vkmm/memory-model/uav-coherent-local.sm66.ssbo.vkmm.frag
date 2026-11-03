float4 main(float4 pos : SV_Position) : SV_Target
{
	RWByteAddressBuffer Buf = ResourceDescriptorHeap[0];
	uint offset = uint(pos.x) * 4;
	Buf.Store(offset, uint(pos.y));
	DeviceMemoryBarrier();
	return Buf.Load(offset + 4);
}
