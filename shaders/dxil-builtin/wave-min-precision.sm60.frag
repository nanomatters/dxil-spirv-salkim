float4 main(float4 value : TEXCOORD0) : SV_Target
{
	min16float x = min16float(value.x);
	return float4(WaveActiveSum(x), WaveActiveProduct(x), WaveActiveMin(x), WaveActiveMax(x)) +
	       float4(WavePrefixSum(x), WavePrefixProduct(x), 0.0, 0.0);
}
