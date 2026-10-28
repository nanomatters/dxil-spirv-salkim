#version 460
#extension GL_EXT_demote_to_helper_invocation : require
#extension GL_KHR_shader_subgroup_arithmetic : require

layout(location = 0) in vec4 TEXCOORD;
layout(location = 0) out vec4 SV_Target;

void main()
{
    mediump float _16 = TEXCOORD.x;
    bool _20 = helperInvocationEXT();
    bool _24 = helperInvocationEXT();
    bool _28 = helperInvocationEXT();
    bool _32 = helperInvocationEXT();
    bool _36 = helperInvocationEXT();
    bool _39 = helperInvocationEXT();
    SV_Target.x = subgroupExclusiveAdd(_36 ? 0.0 : _16) + subgroupAdd(_20 ? 0.0 : _16);
    SV_Target.y = subgroupExclusiveMul(_39 ? 1.0 : _16) + subgroupMul(_24 ? 1.0 : _16);
    SV_Target.z = subgroupMin(_28 ? uintBitsToFloat(0x7f800000u /* inf */) : _16);
    SV_Target.w = subgroupMax(_32 ? uintBitsToFloat(0xff800000u /* -inf */) : _16);
}


#if 0
// SPIR-V disassembly
; SPIR-V
; Version: 1.6
; Generator: Unknown(30017); 21022
; Bound: 51
; Schema: 0
OpCapability Shader
OpCapability GroupNonUniformArithmetic
OpCapability VulkanMemoryModel
OpCapability DemoteToHelperInvocation
OpExtension "SPV_EXT_demote_to_helper_invocation"
OpMemoryModel Logical Vulkan
OpEntryPoint Fragment %3 "main" %8 %10
OpExecutionMode %3 OriginUpperLeft
OpName %3 "main"
OpName %8 "TEXCOORD"
OpName %10 "SV_Target"
OpDecorate %8 Location 0
OpDecorate %10 Location 0
OpDecorate %16 RelaxedPrecision
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
%3 = OpFunction %1 None %2
%4 = OpLabel
OpBranch %50
%50 = OpLabel
%12 = OpAccessChain %11 %8 %14
%15 = OpLoad %5 %12
%16 = OpCopyObject %5 %15
%20 = OpIsHelperInvocationEXT %19
%22 = OpSelect %5 %20 %21 %16
%17 = OpGroupNonUniformFAdd %5 %18 Reduce %22
%24 = OpIsHelperInvocationEXT %19
%26 = OpSelect %5 %24 %25 %16
%23 = OpGroupNonUniformFMul %5 %18 Reduce %26
%28 = OpIsHelperInvocationEXT %19
%30 = OpSelect %5 %28 %29 %16
%27 = OpGroupNonUniformFMin %5 %18 Reduce %30
%32 = OpIsHelperInvocationEXT %19
%34 = OpSelect %5 %32 %33 %16
%31 = OpGroupNonUniformFMax %5 %18 Reduce %34
%36 = OpIsHelperInvocationEXT %19
%37 = OpSelect %5 %36 %21 %16
%35 = OpGroupNonUniformFAdd %5 %18 ExclusiveScan %37
%39 = OpIsHelperInvocationEXT %19
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
