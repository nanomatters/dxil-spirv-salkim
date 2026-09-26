cbuffer Params : register(b0) { float4 Rows[13]; uint Index; };

float main() : SV_Target
{
    // The repeated final word must not be forwarded as a contiguous array.
    float values[6] = {
        Rows[11].x, Rows[11].y, Rows[11].z,
        Rows[11].w, Rows[12].y, Rows[12].y
    };
    return values[Index % 6];
}
