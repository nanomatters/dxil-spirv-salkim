#version 460

layout(set = 0, binding = 0, std140) uniform _10_12
{
    vec4 _m0[14];
} _12;

layout(location = 0) out float SV_Target;

void main()
{
    float _18[6];
    _18[0u] = _12._m0[11u].x;
    _18[1u] = _12._m0[11u].y;
    _18[2u] = _12._m0[11u].z;
    _18[3u] = _12._m0[11u].w;
    _18[4u] = _12._m0[12u].y;
    _18[5u] = _12._m0[12u].y;
    SV_Target = _18[floatBitsToUint(_12._m0[13u]).x % 6u];
}


#if 0
// SPIR-V disassembly
; SPIR-V
; Version: 1.3
; Generator: Unknown(30017); 21022
; Bound: 55
; Schema: 0
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %3 "main" %14
OpExecutionMode %3 OriginUpperLeft
OpName %3 "main"
OpName %10 ""
OpName %14 "SV_Target"
OpDecorate %9 ArrayStride 16
OpMemberDecorate %10 0 Offset 0
OpDecorate %10 Block
OpDecorate %12 DescriptorSet 0
OpDecorate %12 Binding 0
OpDecorate %14 Location 0
%1 = OpTypeVoid
%2 = OpTypeFunction %1
%5 = OpTypeInt 32 0
%6 = OpConstant %5 14
%7 = OpTypeFloat 32
%8 = OpTypeVector %7 4
%9 = OpTypeArray %8 %6
%10 = OpTypeStruct %9
%11 = OpTypePointer Uniform %10
%12 = OpVariable %11 Uniform
%13 = OpTypePointer Output %7
%14 = OpVariable %13 Output
%15 = OpConstant %5 6
%16 = OpTypeArray %7 %15
%17 = OpTypePointer Function %16
%19 = OpTypePointer Function %7
%21 = OpConstant %5 0
%22 = OpConstant %5 11
%23 = OpTypePointer Uniform %8
%28 = OpConstant %5 1
%31 = OpConstant %5 2
%34 = OpConstant %5 3
%37 = OpConstant %5 4
%38 = OpConstant %5 12
%43 = OpConstant %5 5
%44 = OpConstant %5 13
%47 = OpTypeVector %5 4
%3 = OpFunction %1 None %2
%4 = OpLabel
%18 = OpVariable %17 Function
OpBranch %53
%53 = OpLabel
%20 = OpInBoundsAccessChain %19 %18 %21
%24 = OpAccessChain %23 %12 %21 %22
%25 = OpLoad %8 %24
%26 = OpCompositeExtract %7 %25 0
OpStore %20 %26
%27 = OpInBoundsAccessChain %19 %18 %28
%29 = OpCompositeExtract %7 %25 1
OpStore %27 %29
%30 = OpInBoundsAccessChain %19 %18 %31
%32 = OpCompositeExtract %7 %25 2
OpStore %30 %32
%33 = OpInBoundsAccessChain %19 %18 %34
%35 = OpCompositeExtract %7 %25 3
OpStore %33 %35
%36 = OpInBoundsAccessChain %19 %18 %37
%39 = OpAccessChain %23 %12 %21 %38
%40 = OpLoad %8 %39
%41 = OpCompositeExtract %7 %40 1
OpStore %36 %41
%42 = OpInBoundsAccessChain %19 %18 %43
OpStore %42 %41
%45 = OpAccessChain %23 %12 %21 %44
%46 = OpLoad %8 %45
%48 = OpBitcast %47 %46
%49 = OpCompositeExtract %5 %48 0
%50 = OpUMod %5 %49 %15
%51 = OpInBoundsAccessChain %19 %18 %50
%52 = OpLoad %7 %51
OpStore %14 %52
OpReturn
OpFunctionEnd
#endif
