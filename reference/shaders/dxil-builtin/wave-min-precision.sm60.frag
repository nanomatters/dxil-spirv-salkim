#version 460
#extension GL_KHR_shader_subgroup_arithmetic : require

layout(location = 0) in vec4 TEXCOORD;
layout(location = 0) out vec4 SV_Target;

void main()
{
    mediump float _16 = TEXCOORD.x;
    SV_Target.x = subgroupExclusiveAdd(gl_HelperInvocation ? 0.0 : _16) + subgroupAdd(gl_HelperInvocation ? 0.0 : _16);
    SV_Target.y = subgroupExclusiveMul(gl_HelperInvocation ? 1.0 : _16) + subgroupMul(gl_HelperInvocation ? 1.0 : _16);
    SV_Target.z = subgroupMin(gl_HelperInvocation ? uintBitsToFloat(0x7f800000u /* inf */) : _16);
    SV_Target.w = subgroupMax(gl_HelperInvocation ? uintBitsToFloat(0xff800000u /* -inf */) : _16);
}


#if 0
// SPIR-V disassembly
; SPIR-V
; Version: 1.3
; Generator: Unknown(30017); 21022
; Bound: 54
; Schema: 0
OpCapability Shader
OpCapability GroupNonUniformArithmetic
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %3 "main" %8 %10 %52
OpExecutionMode %3 OriginUpperLeft
OpName %3 "main"
OpName %8 "TEXCOORD"
OpName %10 "SV_Target"
OpDecorate %8 Location 0
OpDecorate %10 Location 0
OpDecorate %16 RelaxedPrecision
OpDecorate %52 BuiltIn HelperInvocation
%1 = OpTypeVoid
%2 = OpTypeFunction %1
%5 = OpTypeFloat 32
%6 = OpTypeVector %5 4
%7 = OpTypePointer Input %6
%8 = OpVariable %7 Input
%9 = OpTypePointer Output %6
%10 = OpVariable %9 Output
%11 = OpTypePointer Input %5
%13 = OpTypeInt 32 0
%14 = OpConstant %13 0
%18 = OpConstant %13 3
%19 = OpTypeBool
%21 = OpConstant %5 0
%25 = OpConstant %5 1
%29 = OpConstant %5 0x1p+128
%33 = OpConstant %5 -0x1p+128
%43 = OpTypePointer Output %5
%46 = OpConstant %13 1
%48 = OpConstant %13 2
%51 = OpTypePointer Input %19
%52 = OpVariable %51 Input
%3 = OpFunction %1 None %2
%4 = OpLabel
OpBranch %50
%50 = OpLabel
%12 = OpAccessChain %11 %8 %14
%15 = OpLoad %5 %12
%16 = OpCopyObject %5 %15
%20 = OpLoad %19 %52
%22 = OpSelect %5 %20 %21 %16
%17 = OpGroupNonUniformFAdd %5 %18 Reduce %22
%24 = OpLoad %19 %52
%26 = OpSelect %5 %24 %25 %16
%23 = OpGroupNonUniformFMul %5 %18 Reduce %26
%28 = OpLoad %19 %52
%30 = OpSelect %5 %28 %29 %16
%27 = OpGroupNonUniformFMin %5 %18 Reduce %30
%32 = OpLoad %19 %52
%34 = OpSelect %5 %32 %33 %16
%31 = OpGroupNonUniformFMax %5 %18 Reduce %34
%36 = OpLoad %19 %52
%37 = OpSelect %5 %36 %21 %16
%35 = OpGroupNonUniformFAdd %5 %18 ExclusiveScan %37
%39 = OpLoad %19 %52
%40 = OpSelect %5 %39 %25 %16
%38 = OpGroupNonUniformFMul %5 %18 ExclusiveScan %40
%41 = OpFAdd %5 %35 %17
%42 = OpFAdd %5 %38 %23
%44 = OpAccessChain %43 %10 %14
OpStore %44 %41
%45 = OpAccessChain %43 %10 %46
OpStore %45 %42
%47 = OpAccessChain %43 %10 %48
OpStore %47 %27
%49 = OpAccessChain %43 %10 %18
OpStore %49 %31
OpReturn
OpFunctionEnd
#endif
