#version 460
#if defined(GL_ARB_gpu_shader_int64)
#extension GL_ARB_gpu_shader_int64 : require
#else
#error No extension available for 64-bit integers.
#endif
#if defined(GL_AMD_gpu_shader_half_float)
#extension GL_AMD_gpu_shader_half_float : require
#elif defined(GL_EXT_shader_explicit_arithmetic_types_float16)
#extension GL_EXT_shader_explicit_arithmetic_types_float16 : require
#else
#error No extension available for FP16.
#endif
#extension GL_EXT_shader_16bit_storage : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : require

struct CBVComposite16x8
{
    float16_t _m0;
    float16_t _m1;
    float16_t _m2;
    float16_t _m3;
    float16_t _m4;
    float16_t _m5;
    float16_t _m6;
    float16_t _m7;
};

layout(set = 5, binding = 0, std140) uniform BindlessCBV
{
    u64vec2 _m0[4096];
} _16[];

layout(set = 5, binding = 0, std140) uniform _20_23
{
    vec4 _m0[4096];
} _23[];

layout(set = 5, binding = 0, std140) uniform _27_30
{
    dvec2 _m0[4096];
} _30[];

layout(push_constant, std430) uniform RootConstants
{
    uint _m0;
    uint _m1;
    uint _m2;
    uint _m3;
    uint _m4;
    uint _m5;
    uint _m6;
    uint _m7;
} registers;

layout(location = 0) out vec4 SV_Target;

void main()
{
    uint _37 = registers._m5 + 2u;
    uint _43 = registers._m5 + 1u;
    f16vec2 _68 = unpackFloat2x16(floatBitsToUint(_23[registers._m5]._m0[1u].x));
    f16vec2 _71 = unpackFloat2x16(floatBitsToUint(_23[registers._m5]._m0[1u].y));
    f16vec2 _74 = unpackFloat2x16(floatBitsToUint(_23[registers._m5]._m0[1u].z));
    f16vec2 _77 = unpackFloat2x16(floatBitsToUint(_23[registers._m5]._m0[1u].w));
    CBVComposite16x8 _81 = CBVComposite16x8(_68.x, _68.y, _71.x, _71.y, _74.x, _74.y, _77.x, _77.y);
    SV_Target.x = (((float(_81._m0) + _23[registers._m5]._m0[0u].x) + float(int64_t(_16[registers._m5]._m0[2u].x))) + _23[_43]._m0[0u].x) + float(_30[_37]._m0[0u].x);
    SV_Target.y = (((float(_81._m1) + _23[registers._m5]._m0[0u].y) + float(int64_t(_16[registers._m5]._m0[2u].y))) + _23[_43]._m0[0u].y) + float(_30[_37]._m0[0u].y);
    SV_Target.z = (((float(_81._m2) + _23[registers._m5]._m0[0u].z) + float(int64_t(_16[registers._m5]._m0[3u].x))) + _23[_43]._m0[0u].z) + float(_30[_37]._m0[1u].x);
    SV_Target.w = (((float(_81._m3) + _23[registers._m5]._m0[0u].w) + float(int64_t(_16[registers._m5]._m0[3u].y))) + _23[_43]._m0[0u].w) + float(_30[_37]._m0[1u].y);
}


#if 0
// SPIR-V disassembly
; SPIR-V
; Version: 1.3
; Generator: Unknown(30017); 21022
; Bound: 146
; Schema: 0
OpCapability Shader
OpCapability Float16
OpCapability Float64
OpCapability Int64
OpCapability DenormPreserve
OpCapability RuntimeDescriptorArray
OpCapability PhysicalStorageBufferAddresses
OpExtension "SPV_EXT_descriptor_indexing"
OpExtension "SPV_KHR_float_controls"
OpExtension "SPV_KHR_physical_storage_buffer"
OpMemoryModel PhysicalStorageBuffer64 GLSL450
OpEntryPoint Fragment %3 "main" %32
OpExecutionMode %3 OriginUpperLeft
OpExecutionMode %3 DenormPreserve 16
OpExecutionMode %3 DenormPreserve 64
OpName %3 "main"
OpName %6 "RootConstants"
OpName %8 "registers"
OpName %13 "BindlessCBV"
OpName %20 "BindlessCBV"
OpName %27 "BindlessCBV"
OpName %32 "SV_Target"
OpName %80 "CBVComposite16x8"
OpDecorate %6 Block
OpMemberDecorate %6 0 Offset 0
OpMemberDecorate %6 1 Offset 4
OpMemberDecorate %6 2 Offset 8
OpMemberDecorate %6 3 Offset 12
OpMemberDecorate %6 4 Offset 16
OpMemberDecorate %6 5 Offset 20
OpMemberDecorate %6 6 Offset 24
OpMemberDecorate %6 7 Offset 28
OpDecorate %12 ArrayStride 16
OpDecorate %13 Block
OpMemberDecorate %13 0 Offset 0
OpDecorate %16 DescriptorSet 5
OpDecorate %16 Binding 0
OpDecorate %19 ArrayStride 16
OpDecorate %20 Block
OpMemberDecorate %20 0 Offset 0
OpDecorate %23 DescriptorSet 5
OpDecorate %23 Binding 0
OpDecorate %26 ArrayStride 16
OpDecorate %27 Block
OpMemberDecorate %27 0 Offset 0
OpDecorate %30 DescriptorSet 5
OpDecorate %30 Binding 0
OpDecorate %32 Location 0
%1 = OpTypeVoid
%2 = OpTypeFunction %1
%5 = OpTypeInt 32 0
%6 = OpTypeStruct %5 %5 %5 %5 %5 %5 %5 %5
%7 = OpTypePointer PushConstant %6
%8 = OpVariable %7 PushConstant
%9 = OpTypeInt 64 0
%10 = OpTypeVector %9 2
%11 = OpConstant %5 4096
%12 = OpTypeArray %10 %11
%13 = OpTypeStruct %12
%14 = OpTypeRuntimeArray %13
%15 = OpTypePointer Uniform %14
%16 = OpVariable %15 Uniform
%17 = OpTypeFloat 32
%18 = OpTypeVector %17 4
%19 = OpTypeArray %18 %11
%20 = OpTypeStruct %19
%21 = OpTypeRuntimeArray %20
%22 = OpTypePointer Uniform %21
%23 = OpVariable %22 Uniform
%24 = OpTypeFloat 64
%25 = OpTypeVector %24 2
%26 = OpTypeArray %25 %11
%27 = OpTypeStruct %26
%28 = OpTypeRuntimeArray %27
%29 = OpTypePointer Uniform %28
%30 = OpVariable %29 Uniform
%31 = OpTypePointer Output %18
%32 = OpVariable %31 Output
%33 = OpTypePointer PushConstant %5
%35 = OpConstant %5 5
%38 = OpConstant %5 2
%39 = OpTypePointer Uniform %27
%44 = OpConstant %5 1
%45 = OpTypePointer Uniform %20
%49 = OpTypePointer Uniform %13
%52 = OpConstant %5 0
%53 = OpTypePointer Uniform %18
%60 = OpTypeFloat 16
%63 = OpTypeVector %60 2
%80 = OpTypeStruct %60 %60 %60 %60 %60 %60 %60 %60
%94 = OpTypePointer Uniform %10
%99 = OpConstant %5 3
%122 = OpTypePointer Uniform %25
%139 = OpTypePointer Output %17
%3 = OpFunction %1 None %2
%4 = OpLabel
OpBranch %144
%144 = OpLabel
%34 = OpAccessChain %33 %8 %35
%36 = OpLoad %5 %34
%37 = OpIAdd %5 %36 %38
%40 = OpAccessChain %39 %30 %37
%41 = OpAccessChain %33 %8 %35
%42 = OpLoad %5 %41
%43 = OpIAdd %5 %42 %44
%46 = OpAccessChain %45 %23 %43
%47 = OpAccessChain %33 %8 %35
%48 = OpLoad %5 %47
%50 = OpAccessChain %49 %16 %48
%51 = OpAccessChain %45 %23 %48
%54 = OpAccessChain %53 %51 %52 %52
%55 = OpLoad %18 %54
%56 = OpCompositeExtract %17 %55 0
%57 = OpCompositeExtract %17 %55 1
%58 = OpCompositeExtract %17 %55 2
%59 = OpCompositeExtract %17 %55 3
%61 = OpAccessChain %53 %51 %52 %44
%62 = OpLoad %18 %61
%64 = OpCompositeExtract %17 %62 0
%65 = OpCompositeExtract %17 %62 1
%66 = OpCompositeExtract %17 %62 2
%67 = OpCompositeExtract %17 %62 3
%68 = OpBitcast %63 %64
%69 = OpCompositeExtract %60 %68 0
%70 = OpCompositeExtract %60 %68 1
%71 = OpBitcast %63 %65
%72 = OpCompositeExtract %60 %71 0
%73 = OpCompositeExtract %60 %71 1
%74 = OpBitcast %63 %66
%75 = OpCompositeExtract %60 %74 0
%76 = OpCompositeExtract %60 %74 1
%77 = OpBitcast %63 %67
%78 = OpCompositeExtract %60 %77 0
%79 = OpCompositeExtract %60 %77 1
%81 = OpCompositeConstruct %80 %69 %70 %72 %73 %75 %76 %78 %79
%82 = OpCompositeExtract %60 %81 0
%83 = OpCompositeExtract %60 %81 1
%84 = OpCompositeExtract %60 %81 2
%85 = OpCompositeExtract %60 %81 3
%86 = OpFConvert %17 %82
%87 = OpFConvert %17 %83
%88 = OpFConvert %17 %84
%89 = OpFConvert %17 %85
%90 = OpFAdd %17 %86 %56
%91 = OpFAdd %17 %87 %57
%92 = OpFAdd %17 %88 %58
%93 = OpFAdd %17 %89 %59
%95 = OpAccessChain %94 %50 %52 %38
%96 = OpLoad %10 %95
%97 = OpCompositeExtract %9 %96 0
%98 = OpCompositeExtract %9 %96 1
%100 = OpAccessChain %94 %50 %52 %99
%101 = OpLoad %10 %100
%102 = OpCompositeExtract %9 %101 0
%103 = OpCompositeExtract %9 %101 1
%104 = OpConvertSToF %17 %97
%105 = OpConvertSToF %17 %98
%106 = OpConvertSToF %17 %102
%107 = OpConvertSToF %17 %103
%108 = OpFAdd %17 %90 %104
%109 = OpFAdd %17 %91 %105
%110 = OpFAdd %17 %92 %106
%111 = OpFAdd %17 %93 %107
%112 = OpAccessChain %53 %46 %52 %52
%113 = OpLoad %18 %112
%114 = OpCompositeExtract %17 %113 0
%115 = OpCompositeExtract %17 %113 1
%116 = OpCompositeExtract %17 %113 2
%117 = OpCompositeExtract %17 %113 3
%118 = OpFAdd %17 %108 %114
%119 = OpFAdd %17 %109 %115
%120 = OpFAdd %17 %110 %116
%121 = OpFAdd %17 %111 %117
%123 = OpAccessChain %122 %40 %52 %52
%124 = OpLoad %25 %123
%125 = OpCompositeExtract %24 %124 0
%126 = OpCompositeExtract %24 %124 1
%127 = OpAccessChain %122 %40 %52 %44
%128 = OpLoad %25 %127
%129 = OpCompositeExtract %24 %128 0
%130 = OpCompositeExtract %24 %128 1
%131 = OpFConvert %17 %125
%132 = OpFConvert %17 %126
%133 = OpFConvert %17 %129
%134 = OpFConvert %17 %130
%135 = OpFAdd %17 %118 %131
%136 = OpFAdd %17 %119 %132
%137 = OpFAdd %17 %120 %133
%138 = OpFAdd %17 %121 %134
%140 = OpAccessChain %139 %32 %52
OpStore %140 %135
%141 = OpAccessChain %139 %32 %44
OpStore %141 %136
%142 = OpAccessChain %139 %32 %38
OpStore %142 %137
%143 = OpAccessChain %139 %32 %99
OpStore %143 %138
OpReturn
OpFunctionEnd
#endif
