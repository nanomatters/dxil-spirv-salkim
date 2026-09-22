#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require

layout(set = 5, binding = 0, scalar) uniform BindlessCBV
{
    float _m0[16384];
} _15[];

layout(set = 5, binding = 0, std140) uniform _19_22
{
    vec4 _m0[4096];
} _22[];

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

layout(location = 0) flat in uint A;
layout(location = 1) in vec4 P;
layout(location = 0) out vec4 SV_Target;

void main()
{
    uint _89 = A % 6u;
    SV_Target.x = _15[registers._m5]._m0[_89 * 4u] * P.x;
    SV_Target.y = _15[registers._m5]._m0[(_89 * 4u) + 1u] * P.y;
    SV_Target.z = _15[registers._m5]._m0[(_89 * 4u) + 2u] * P.z;
    SV_Target.w = _15[registers._m5]._m0[(_89 * 4u) + 3u] * P.w;
}


#if 0
// SPIR-V disassembly
; SPIR-V
; Version: 1.3
; Generator: Unknown(30017); 21022
; Bound: 118
; Schema: 0
OpCapability Shader
OpCapability RuntimeDescriptorArray
OpCapability PhysicalStorageBufferAddresses
OpExtension "SPV_EXT_descriptor_indexing"
OpExtension "SPV_KHR_physical_storage_buffer"
OpMemoryModel PhysicalStorageBuffer64 GLSL450
OpEntryPoint Fragment %3 "main" %24 %26 %28
OpExecutionMode %3 OriginUpperLeft
OpName %3 "main"
OpName %6 "RootConstants"
OpName %8 "registers"
OpName %12 "BindlessCBV"
OpName %19 "BindlessCBV"
OpName %24 "A"
OpName %26 "P"
OpName %28 "SV_Target"
OpDecorate %6 Block
OpMemberDecorate %6 0 Offset 0
OpMemberDecorate %6 1 Offset 4
OpMemberDecorate %6 2 Offset 8
OpMemberDecorate %6 3 Offset 12
OpMemberDecorate %6 4 Offset 16
OpMemberDecorate %6 5 Offset 20
OpMemberDecorate %6 6 Offset 24
OpMemberDecorate %6 7 Offset 28
OpDecorate %11 ArrayStride 4
OpDecorate %12 Block
OpMemberDecorate %12 0 Offset 0
OpDecorate %15 DescriptorSet 5
OpDecorate %15 Binding 0
OpDecorate %18 ArrayStride 16
OpDecorate %19 Block
OpMemberDecorate %19 0 Offset 0
OpDecorate %22 DescriptorSet 5
OpDecorate %22 Binding 0
OpDecorate %24 Flat
OpDecorate %24 Location 0
OpDecorate %26 Location 1
OpDecorate %28 Location 0
%1 = OpTypeVoid
%2 = OpTypeFunction %1
%5 = OpTypeInt 32 0
%6 = OpTypeStruct %5 %5 %5 %5 %5 %5 %5 %5
%7 = OpTypePointer PushConstant %6
%8 = OpVariable %7 PushConstant
%9 = OpTypeFloat 32
%10 = OpConstant %5 16384
%11 = OpTypeArray %9 %10
%12 = OpTypeStruct %11
%13 = OpTypeRuntimeArray %12
%14 = OpTypePointer Uniform %13
%15 = OpVariable %14 Uniform
%16 = OpTypeVector %9 4
%17 = OpConstant %5 4096
%18 = OpTypeArray %16 %17
%19 = OpTypeStruct %18
%20 = OpTypeRuntimeArray %19
%21 = OpTypePointer Uniform %20
%22 = OpVariable %21 Uniform
%23 = OpTypePointer Input %5
%24 = OpVariable %23 Input
%25 = OpTypePointer Input %16
%26 = OpVariable %25 Input
%27 = OpTypePointer Output %16
%28 = OpVariable %27 Output
%29 = OpTypePointer PushConstant %5
%31 = OpConstant %5 5
%33 = OpTypePointer Uniform %12
%35 = OpTypePointer Uniform %19
%37 = OpTypePointer Input %9
%39 = OpConstant %5 0
%42 = OpConstant %5 1
%45 = OpConstant %5 2
%48 = OpConstant %5 3
%51 = OpTypePointer Uniform %16
%76 = OpConstant %5 4
%90 = OpConstant %5 6
%92 = OpTypePointer Uniform %9
%111 = OpTypePointer Output %9
%3 = OpFunction %1 None %2
%4 = OpLabel
OpBranch %116
%116 = OpLabel
%30 = OpAccessChain %29 %8 %31
%32 = OpLoad %5 %30
%34 = OpAccessChain %33 %15 %32
%36 = OpAccessChain %35 %22 %32
%38 = OpAccessChain %37 %26 %39
%40 = OpLoad %9 %38
%41 = OpAccessChain %37 %26 %42
%43 = OpLoad %9 %41
%44 = OpAccessChain %37 %26 %45
%46 = OpLoad %9 %44
%47 = OpAccessChain %37 %26 %48
%49 = OpLoad %9 %47
%50 = OpLoad %5 %24
%52 = OpAccessChain %51 %36 %39 %39
%53 = OpLoad %16 %52
%54 = OpCompositeExtract %9 %53 0
%55 = OpCompositeExtract %9 %53 1
%56 = OpCompositeExtract %9 %53 2
%57 = OpCompositeExtract %9 %53 3
%58 = OpAccessChain %51 %36 %39 %42
%59 = OpLoad %16 %58
%60 = OpCompositeExtract %9 %59 0
%61 = OpCompositeExtract %9 %59 1
%62 = OpCompositeExtract %9 %59 2
%63 = OpCompositeExtract %9 %59 3
%64 = OpAccessChain %51 %36 %39 %45
%65 = OpLoad %16 %64
%66 = OpCompositeExtract %9 %65 0
%67 = OpCompositeExtract %9 %65 1
%68 = OpCompositeExtract %9 %65 2
%69 = OpCompositeExtract %9 %65 3
%70 = OpAccessChain %51 %36 %39 %48
%71 = OpLoad %16 %70
%72 = OpCompositeExtract %9 %71 0
%73 = OpCompositeExtract %9 %71 1
%74 = OpCompositeExtract %9 %71 2
%75 = OpCompositeExtract %9 %71 3
%77 = OpAccessChain %51 %36 %39 %76
%78 = OpLoad %16 %77
%79 = OpCompositeExtract %9 %78 0
%80 = OpCompositeExtract %9 %78 1
%81 = OpCompositeExtract %9 %78 2
%82 = OpCompositeExtract %9 %78 3
%83 = OpAccessChain %51 %36 %39 %31
%84 = OpLoad %16 %83
%85 = OpCompositeExtract %9 %84 0
%86 = OpCompositeExtract %9 %84 1
%87 = OpCompositeExtract %9 %84 2
%88 = OpCompositeExtract %9 %84 3
%89 = OpUMod %5 %50 %90
%91 = OpIMul %5 %89 %76
%93 = OpAccessChain %92 %34 %39 %91
%94 = OpIMul %5 %89 %76
%95 = OpIAdd %5 %94 %42
%96 = OpAccessChain %92 %34 %39 %95
%97 = OpIMul %5 %89 %76
%98 = OpIAdd %5 %97 %45
%99 = OpAccessChain %92 %34 %39 %98
%100 = OpIMul %5 %89 %76
%101 = OpIAdd %5 %100 %48
%102 = OpAccessChain %92 %34 %39 %101
%103 = OpLoad %9 %93
%104 = OpLoad %9 %96
%105 = OpLoad %9 %99
%106 = OpLoad %9 %102
%107 = OpFMul %9 %103 %40
%108 = OpFMul %9 %104 %43
%109 = OpFMul %9 %105 %46
%110 = OpFMul %9 %106 %49
%112 = OpAccessChain %111 %28 %39
OpStore %112 %107
%113 = OpAccessChain %111 %28 %42
OpStore %113 %108
%114 = OpAccessChain %111 %28 %45
OpStore %114 %109
%115 = OpAccessChain %111 %28 %48
OpStore %115 %110
OpReturn
OpFunctionEnd
#endif
