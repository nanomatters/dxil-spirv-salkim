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

f16vec4 _32;

layout(location = 0) in vec4 TEXCOORD;
layout(location = 0) out vec4 SV_Target;

void main()
{
    f16vec4 _31;
    _31.x = float16_t(TEXCOORD.x);
    _31.y = float16_t(TEXCOORD.y);
    _31.z = float16_t(TEXCOORD.z);
    _31.w = float16_t(TEXCOORD.w);
    vec4 _68 = vec4(((((subgroupMul(gl_HelperInvocation ? f16vec4(float16_t(1.0)) : _31) + subgroupAdd(gl_HelperInvocation ? f16vec4(float16_t(0.0)) : _31)) + subgroupMin(gl_HelperInvocation ? f16vec4(float16_t(1.0 / 0.0)) : _31)) + subgroupMax(gl_HelperInvocation ? f16vec4(float16_t(-1.0 / 0.0)) : _31)) + subgroupExclusiveAdd(gl_HelperInvocation ? f16vec4(float16_t(0.0)) : _31)) + subgroupExclusiveMul(gl_HelperInvocation ? f16vec4(float16_t(1.0)) : _31));
    SV_Target.x = _68.x;
    SV_Target.y = _68.y;
    SV_Target.z = _68.z;
    SV_Target.w = _68.w;
}


#if 0
// SPIR-V disassembly
; SPIR-V
; Version: 1.6
; Generator: Unknown(30017); 21022
; Bound: 82
; Schema: 0
OpCapability Shader
OpCapability Float16
OpCapability GroupNonUniformArithmetic
OpCapability DenormPreserve
OpExtension "SPV_KHR_float_controls"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %3 "main" %8 %10 %80
OpExecutionMode %3 OriginUpperLeft
OpExecutionMode %3 DenormPreserve 16
OpName %3 "main"
OpName %8 "TEXCOORD"
OpName %10 "SV_Target"
OpDecorate %8 Location 0
OpDecorate %10 Location 0
OpDecorate %80 BuiltIn HelperInvocation
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
%17 = OpConstant %13 1
%20 = OpConstant %13 2
%23 = OpConstant %13 3
%25 = OpTypeFloat 16
%30 = OpTypeVector %25 4
%37 = OpTypeBool
%39 = OpConstant %25 0x0p+0
%40 = OpConstantComposite %30 %39 %39 %39 %39
%44 = OpConstant %25 0x1p+0
%45 = OpConstantComposite %30 %44 %44 %44 %44
%50 = OpConstant %25 0x1p+16
%51 = OpConstantComposite %30 %50 %50 %50 %50
%56 = OpConstant %25 -0x1p+16
%57 = OpConstantComposite %30 %56 %56 %56 %56
%70 = OpTypePointer Output %5
%79 = OpTypePointer Input %37
%80 = OpVariable %79 Input
%3 = OpFunction %1 None %2
%4 = OpLabel
%32 = OpUndef %30
OpBranch %78
%78 = OpLabel
%12 = OpAccessChain %11 %8 %14
%15 = OpLoad %5 %12
%16 = OpAccessChain %11 %8 %17
%18 = OpLoad %5 %16
%19 = OpAccessChain %11 %8 %20
%21 = OpLoad %5 %19
%22 = OpAccessChain %11 %8 %23
%24 = OpLoad %5 %22
%26 = OpFConvert %25 %15
%27 = OpFConvert %25 %18
%28 = OpFConvert %25 %21
%29 = OpFConvert %25 %24
%31 = OpCompositeInsert %30 %26 %32 0
%33 = OpCompositeInsert %30 %27 %31 1
%34 = OpCompositeInsert %30 %28 %33 2
%35 = OpCompositeInsert %30 %29 %34 3
%38 = OpLoad %37 %80
%41 = OpSelect %30 %38 %40 %35
%36 = OpGroupNonUniformFAdd %30 %23 Reduce %41
%43 = OpLoad %37 %80
%46 = OpSelect %30 %43 %45 %35
%42 = OpGroupNonUniformFMul %30 %23 Reduce %46
%47 = OpFAdd %30 %42 %36
%49 = OpLoad %37 %80
%52 = OpSelect %30 %49 %51 %35
%48 = OpGroupNonUniformFMin %30 %23 Reduce %52
%53 = OpFAdd %30 %47 %48
%55 = OpLoad %37 %80
%58 = OpSelect %30 %55 %57 %35
%54 = OpGroupNonUniformFMax %30 %23 Reduce %58
%59 = OpFAdd %30 %53 %54
%61 = OpLoad %37 %80
%62 = OpSelect %30 %61 %40 %35
%60 = OpGroupNonUniformFAdd %30 %23 ExclusiveScan %62
%63 = OpFAdd %30 %59 %60
%65 = OpLoad %37 %80
%66 = OpSelect %30 %65 %45 %35
%64 = OpGroupNonUniformFMul %30 %23 ExclusiveScan %66
%67 = OpFAdd %30 %63 %64
%68 = OpFConvert %6 %67
%69 = OpCompositeExtract %5 %68 0
%71 = OpAccessChain %70 %10 %14
OpStore %71 %69
%72 = OpCompositeExtract %5 %68 1
%73 = OpAccessChain %70 %10 %17
OpStore %73 %72
%74 = OpCompositeExtract %5 %68 2
%75 = OpAccessChain %70 %10 %20
OpStore %75 %74
%76 = OpCompositeExtract %5 %68 3
%77 = OpAccessChain %70 %10 %23
OpStore %77 %76
OpReturn
OpFunctionEnd
#endif
