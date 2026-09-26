#ifndef DEPTH_SEMANTIC
#define DEPTH_SEMANTIC SV_Depth
#endif

struct Output
{
    float4 color0 : SV_Target0;
    float4 color1 : SV_Target1;
    float4 ignored : SV_Target2;
    float depth : DEPTH_SEMANTIC;
    uint coverage : SV_Coverage;
    uint stencil : SV_StencilRef;
};

Output main(noperspective centroid float4 pos : SV_Position)
{
    Output output;
    output.color0 = pos;
    output.color1 = pos * 2;
    output.ignored = pos * 3;
    output.depth = pos.z;
    output.coverage = uint(pos.x);
    output.stencil = uint(pos.y);
    return output;
}
