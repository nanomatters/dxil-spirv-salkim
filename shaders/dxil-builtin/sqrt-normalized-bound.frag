float2 main(float2 value : VALUE) : SV_Target
{
	float normalized = value.x * rsqrt(dot(value, value));
	return float2(sqrt(1.0 - normalized), sqrt(2.0 - 2.0 * normalized));
}
