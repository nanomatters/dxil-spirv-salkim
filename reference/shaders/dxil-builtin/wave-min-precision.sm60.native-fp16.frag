#version 460
#if defined(GL_AMD_gpu_shader_half_float)
#extension GL_AMD_gpu_shader_half_float : require
#elif defined(GL_EXT_shader_explicit_arithmetic_types_float16)
#extension GL_EXT_shader_explicit_arithmetic_types_float16 : require
#else
#error No extension available for FP16.
#endif
#extension GL_EXT_shader_16bit_storage : require
#extension GL_EXT_shader_subgroup_extended_types_float16 : require
#extension GL_KHR_shader_subgroup_arithmetic : require

layout(location = 0) in vec4 TEXCOORD;
layout(location = 0) out vec4 SV_Target;

void main()
{
    float16_t _17 = float16_t(TEXCOORD.x);
    SV_Target.x = float(subgroupExclusiveAdd(gl_HelperInvocation ? float16_t(0.0) : _17)) + float(subgroupAdd(gl_HelperInvocation ? float16_t(0.0) : _17));
    SV_Target.y = float(subgroupExclusiveMul(gl_HelperInvocation ? float16_t(1.0) : _17)) + float(subgroupMul(gl_HelperInvocation ? float16_t(1.0) : _17));
    SV_Target.z = float(subgroupMin(gl_HelperInvocation ? float16_t(1.0 / 0.0) : _17));
    SV_Target.w = float(subgroupMax(gl_HelperInvocation ? float16_t(-1.0 / 0.0) : _17));
}


#if 0
// SPIR-V disassembly
; SPIR-V
; Version: 1.3
; Generator: Unknown(30017); 21022
; Bound: 61
; Schema: 0
OpCapability Shader
OpCapability Float16
OpCapability GroupNonUniformArithmetic
OpCapability DenormPreserve
OpExtension "SPV_KHR_float_controls"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %3 "main" %8 %10 %59
OpExecutionMode %3 OriginUpperLeft
OpExecutionMode %3 DenormPreserve 16
OpName %3 "main"
OpName %8 "TEXCOORD"
OpName %10 "SV_Target"
OpDecorate %8 Location 0
OpDecorate %10 Location 0
OpDecorate %59 BuiltIn HelperInvocation
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
%16 = OpTypeFloat 16
%19 = OpConstant %13 3
%20 = OpTypeBool
%22 = OpConstant %16 0x0p+0
%27 = OpConstant %16 0x1p+0
%32 = OpConstant %16 0x1p+16
%37 = OpConstant %16 -0x1p+16
%50 = OpTypePointer Output %5
%53 = OpConstant %13 1
%55 = OpConstant %13 2
%58 = OpTypePointer Input %20
%59 = OpVariable %58 Input
%3 = OpFunction %1 None %2
%4 = OpLabel
OpBranch %57
%57 = OpLabel
%12 = OpAccessChain %11 %8 %14
%15 = OpLoad %5 %12
%17 = OpFConvert %16 %15
%21 = OpLoad %20 %59
%23 = OpSelect %16 %21 %22 %17
%18 = OpGroupNonUniformFAdd %16 %19 Reduce %23
%24 = OpFConvert %5 %18
%26 = OpLoad %20 %59
%28 = OpSelect %16 %26 %27 %17
%25 = OpGroupNonUniformFMul %16 %19 Reduce %28
%29 = OpFConvert %5 %25
%31 = OpLoad %20 %59
%33 = OpSelect %16 %31 %32 %17
%30 = OpGroupNonUniformFMin %16 %19 Reduce %33
%34 = OpFConvert %5 %30
%36 = OpLoad %20 %59
%38 = OpSelect %16 %36 %37 %17
%35 = OpGroupNonUniformFMax %16 %19 Reduce %38
%39 = OpFConvert %5 %35
%41 = OpLoad %20 %59
%42 = OpSelect %16 %41 %22 %17
%40 = OpGroupNonUniformFAdd %16 %19 ExclusiveScan %42
%43 = OpFConvert %5 %40
%45 = OpLoad %20 %59
%46 = OpSelect %16 %45 %27 %17
%44 = OpGroupNonUniformFMul %16 %19 ExclusiveScan %46
%47 = OpFConvert %5 %44
%48 = OpFAdd %5 %43 %24
%49 = OpFAdd %5 %47 %29
%51 = OpAccessChain %50 %10 %14
OpStore %51 %48
%52 = OpAccessChain %50 %10 %53
OpStore %52 %49
%54 = OpAccessChain %50 %10 %55
OpStore %54 %34
%56 = OpAccessChain %50 %10 %19
OpStore %56 %39
OpReturn
OpFunctionEnd
#endif
