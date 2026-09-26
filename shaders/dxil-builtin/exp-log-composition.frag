float4 main(float x : X) : SV_Target
{
	return float4(exp2(exp2(x)), log2(log2(x)), exp2(log2(x)), log2(exp2(x)));
}
