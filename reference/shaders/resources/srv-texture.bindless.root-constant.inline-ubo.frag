#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_samplerless_texture_functions : require

layout(set = 6, binding = 1, std140) uniform RootConstants
{
    uint _m0;
    uint _m1;
    uint _m2;
    uint _m3;
    uint _m4;
    uint _m5;
    uint _m6;
    uint _m7;
    uint _m8;
    uint _m9;
    uint _m10;
    uint _m11;
    uint _m12;
    uint _m13;
    uint _m14;
    uint _m15;
} registers;

layout(set = 0, binding = 0) uniform texture2D _13[];

layout(location = 1) flat in uint INDEX;
layout(location = 0) out vec4 SV_Target;

void main()
{
    uint _37 = uint(int(gl_FragCoord.x));
    uint _38 = uint(int(gl_FragCoord.y));
    vec4 _39 = texelFetch(_13[registers._m0 + 3u], ivec2(uvec2(_37, _38)), int(0u));
    vec4 _67 = texelFetch(_13[registers._m0 + (uvec4(registers._m4, registers._m5, registers._m6, registers._m7).x + 4u)], ivec2(uvec2(_37, _38)), int(0u));
    vec4 _84 = texelFetch(_13[nonuniformEXT(registers._m0 + (INDEX + 100u))], ivec2(uvec2(_37, _38)), int(0u));
    vec4 _100 = texelFetch(_13[registers._m0 + 101u], ivec2(uvec2(_37, _38)), int(0u));
    SV_Target.x = ((_67.x + _39.x) + _84.x) + _100.x;
    SV_Target.y = ((_67.y + _39.y) + _84.y) + _100.y;
    SV_Target.z = ((_67.z + _39.z) + _84.z) + _100.z;
    SV_Target.w = ((_67.w + _39.w) + _84.w) + _100.w;
}


#if 0
// SPIR-V disassembly
; SPIR-V
; Version: 1.3
; Generator: Unknown(30017); 21022
; Bound: 118
; Schema: 0
OpCapability Shader
OpCapability SampledImageArrayDynamicIndexing
OpCapability RuntimeDescriptorArray
OpCapability SampledImageArrayNonUniformIndexing
OpCapability PhysicalStorageBufferAddresses
OpExtension "SPV_EXT_descriptor_indexing"
OpExtension "SPV_KHR_physical_storage_buffer"
OpMemoryModel PhysicalStorageBuffer64 GLSL450
OpEntryPoint Fragment %3 "main" %16 %18 %20
OpExecutionMode %3 OriginUpperLeft
OpName %3 "main"
OpName %6 "RootConstants"
OpName %8 "registers"
OpName %16 "SV_Position"
OpName %18 "INDEX"
OpName %20 "SV_Target"
OpDecorate %6 Block
OpMemberDecorate %6 0 Offset 0
OpMemberDecorate %6 1 Offset 4
OpMemberDecorate %6 2 Offset 8
OpMemberDecorate %6 3 Offset 12
OpMemberDecorate %6 4 Offset 16
OpMemberDecorate %6 5 Offset 20
OpMemberDecorate %6 6 Offset 24
OpMemberDecorate %6 7 Offset 28
OpMemberDecorate %6 8 Offset 32
OpMemberDecorate %6 9 Offset 36
OpMemberDecorate %6 10 Offset 40
OpMemberDecorate %6 11 Offset 44
OpMemberDecorate %6 12 Offset 48
OpMemberDecorate %6 13 Offset 52
OpMemberDecorate %6 14 Offset 56
OpMemberDecorate %6 15 Offset 60
OpDecorate %8 DescriptorSet 6
OpDecorate %8 Binding 1
OpDecorate %13 DescriptorSet 0
OpDecorate %13 Binding 0
OpDecorate %16 BuiltIn FragCoord
OpDecorate %18 Flat
OpDecorate %18 Location 1
OpDecorate %20 Location 0
OpDecorate %81 NonUniform
OpDecorate %83 NonUniform
%1 = OpTypeVoid
%2 = OpTypeFunction %1
%5 = OpTypeInt 32 0
%6 = OpTypeStruct %5 %5 %5 %5 %5 %5 %5 %5 %5 %5 %5 %5 %5 %5 %5 %5
%7 = OpTypePointer Uniform %6
%8 = OpVariable %7 Uniform
%9 = OpTypeFloat 32
%10 = OpTypeImage %9 2D 0 0 0 1 Unknown
%11 = OpTypeRuntimeArray %10
%12 = OpTypePointer UniformConstant %11
%13 = OpVariable %12 UniformConstant
%14 = OpTypeVector %9 4
%15 = OpTypePointer Input %14
%16 = OpVariable %15 Input
%17 = OpTypePointer Input %5
%18 = OpVariable %17 Input
%19 = OpTypePointer Output %14
%20 = OpVariable %19 Output
%21 = OpTypePointer Uniform %5
%23 = OpConstant %5 0
%26 = OpConstant %5 3
%27 = OpTypePointer UniformConstant %10
%31 = OpTypePointer Input %9
%35 = OpConstant %5 1
%40 = OpTypeVector %5 2
%47 = OpConstant %5 4
%50 = OpConstant %5 5
%53 = OpConstant %5 6
%56 = OpConstant %5 7
%58 = OpTypeVector %5 4
%78 = OpConstant %5 100
%97 = OpConstant %5 101
%110 = OpTypePointer Output %9
%114 = OpConstant %5 2
%3 = OpFunction %1 None %2
%4 = OpLabel
OpBranch %116
%116 = OpLabel
%22 = OpAccessChain %21 %8 %23
%24 = OpLoad %5 %22
%25 = OpIAdd %5 %24 %26
%28 = OpAccessChain %27 %13 %25
%29 = OpLoad %10 %28
%30 = OpLoad %5 %18
%32 = OpAccessChain %31 %16 %23
%33 = OpLoad %9 %32
%34 = OpAccessChain %31 %16 %35
%36 = OpLoad %9 %34
%37 = OpConvertFToS %5 %33
%38 = OpConvertFToS %5 %36
%41 = OpCompositeConstruct %40 %37 %38
%39 = OpImageFetch %14 %29 %41 Lod %23
%42 = OpCompositeExtract %9 %39 0
%43 = OpCompositeExtract %9 %39 1
%44 = OpCompositeExtract %9 %39 2
%45 = OpCompositeExtract %9 %39 3
%46 = OpAccessChain %21 %8 %47
%48 = OpLoad %5 %46
%49 = OpAccessChain %21 %8 %50
%51 = OpLoad %5 %49
%52 = OpAccessChain %21 %8 %53
%54 = OpLoad %5 %52
%55 = OpAccessChain %21 %8 %56
%57 = OpLoad %5 %55
%59 = OpCompositeConstruct %58 %48 %51 %54 %57
%60 = OpCompositeExtract %5 %59 0
%61 = OpIAdd %5 %60 %47
%62 = OpAccessChain %21 %8 %23
%63 = OpLoad %5 %62
%64 = OpIAdd %5 %63 %61
%65 = OpAccessChain %27 %13 %64
%66 = OpLoad %10 %65
%68 = OpCompositeConstruct %40 %37 %38
%67 = OpImageFetch %14 %66 %68 Lod %23
%69 = OpCompositeExtract %9 %67 0
%70 = OpCompositeExtract %9 %67 1
%71 = OpCompositeExtract %9 %67 2
%72 = OpCompositeExtract %9 %67 3
%73 = OpFAdd %9 %69 %42
%74 = OpFAdd %9 %70 %43
%75 = OpFAdd %9 %71 %44
%76 = OpFAdd %9 %72 %45
%77 = OpIAdd %5 %30 %78
%79 = OpAccessChain %21 %8 %23
%80 = OpLoad %5 %79
%81 = OpIAdd %5 %80 %77
%82 = OpAccessChain %27 %13 %81
%83 = OpLoad %10 %82
%85 = OpCompositeConstruct %40 %37 %38
%84 = OpImageFetch %14 %83 %85 Lod %23
%86 = OpCompositeExtract %9 %84 0
%87 = OpCompositeExtract %9 %84 1
%88 = OpCompositeExtract %9 %84 2
%89 = OpCompositeExtract %9 %84 3
%90 = OpFAdd %9 %73 %86
%91 = OpFAdd %9 %74 %87
%92 = OpFAdd %9 %75 %88
%93 = OpFAdd %9 %76 %89
%94 = OpAccessChain %21 %8 %23
%95 = OpLoad %5 %94
%96 = OpIAdd %5 %95 %97
%98 = OpAccessChain %27 %13 %96
%99 = OpLoad %10 %98
%101 = OpCompositeConstruct %40 %37 %38
%100 = OpImageFetch %14 %99 %101 Lod %23
%102 = OpCompositeExtract %9 %100 0
%103 = OpCompositeExtract %9 %100 1
%104 = OpCompositeExtract %9 %100 2
%105 = OpCompositeExtract %9 %100 3
%106 = OpFAdd %9 %90 %102
%107 = OpFAdd %9 %91 %103
%108 = OpFAdd %9 %92 %104
%109 = OpFAdd %9 %93 %105
%111 = OpAccessChain %110 %20 %23
OpStore %111 %106
%112 = OpAccessChain %110 %20 %35
OpStore %112 %107
%113 = OpAccessChain %110 %20 %114
OpStore %113 %108
%115 = OpAccessChain %110 %20 %26
OpStore %115 %109
OpReturn
OpFunctionEnd
#endif
