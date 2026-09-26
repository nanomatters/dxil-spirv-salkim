float4 main(float4 value : TEXCOORD0) : SV_Target
{
	half4 x = half4(value);
	return WaveActiveSum(x) + WaveActiveProduct(x) + WaveActiveMin(x) + WaveActiveMax(x) +
	       WavePrefixSum(x) + WavePrefixProduct(x);
}
