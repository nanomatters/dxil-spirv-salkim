#version 460
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : require
#extension GL_EXT_shader_16bit_storage : require
#if defined(GL_AMD_gpu_shader_half_float)
#extension GL_AMD_gpu_shader_half_float : require
#elif defined(GL_EXT_shader_explicit_arithmetic_types_float16)
#extension GL_EXT_shader_explicit_arithmetic_types_float16 : require
#else
#error No extension available for FP16.
#endif
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 1, binding = 0, std430) restrict readonly buffer SSBO
{
    uint _m0[];
} _13[];

layout(set = 1, binding = 0, std430) restrict readonly buffer _17_20
{
    u16vec2 _m0[];
} _20[];

layout(set = 1, binding = 0, std430) restrict readonly buffer _23_26
{
    u16vec4 _m0[];
} _26[];

layout(set = 1, binding = 0, std430) restrict readonly buffer _29_32
{
    uvec4 _m0[];
} _32[];

layout(set = 4, binding = 0, std430) readonly buffer _34_37
{
    uint _m0[];
} _37[];

layout(set = 4, binding = 0, std430) readonly buffer _39_42
{
    u16vec2 _m0[];
} _42[];

layout(set = 4, binding = 0, std430) buffer _44_47
{
    u16vec4 _m0[];
} _47[];

layout(set = 4, binding = 0, std430) buffer _49_52
{
    uvec4 _m0[];
} _52[];

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

layout(location = 0) flat in uint INDEX;
layout(location = 1) flat in ivec4 UV;
layout(location = 0) out vec4 SV_Target;

void main()
{
    f16vec2 _99 = uint16BitsToFloat16(_20[registers._m1 + 1u]._m0[uint(UV.y)]);
    uint _111 = _37[registers._m4 + 2u]._m0[uint(UV.z)];
    u16vec2 _120 = _42[registers._m4 + 3u]._m0[uint(UV.w)];
    f16vec2 _121 = uint16BitsToFloat16(_120);
    uint _129 = registers._m1 + 4u;
    f16vec4 _138 = uint16BitsToFloat16(_26[_129]._m0[1u]);
    vec4 _154 = uintBitsToFloat(_32[_129]._m0[1u]);
    uint _165 = registers._m4 + 5u;
    u16vec4 _172 = _47[_165]._m0[1u];
    f16vec4 _173 = uint16BitsToFloat16(_172);
    float _182 = (((float(_121.y) + uintBitsToFloat(_13[registers._m1]._m0[uint(UV.x)])) + float(_138.x)) + _154.x) + float(_173.x);
    float _183 = ((float(_138.y) + float(_99.x)) + _154.y) + float(_173.y);
    float _184 = (((uintBitsToFloat(_111) + float(_99.y)) + float(_138.z)) + _154.z) + float(_173.z);
    float _185 = ((float(_138.w) + float(_121.x)) + _154.w) + float(_173.w);
    _52[_165]._m0[1u] = uvec4(floatBitsToUint(_182), floatBitsToUint(_183), floatBitsToUint(_184), floatBitsToUint(_185));
    SV_Target.x = _182;
    SV_Target.y = _183;
    SV_Target.z = _184;
    SV_Target.w = _185;
}


#if 0
// SPIR-V disassembly
; SPIR-V
; Version: 1.3
; Generator: Unknown(30017); 21022
; Bound: 199
; Schema: 0
OpCapability Shader
OpCapability Float16
OpCapability Int16
OpCapability StorageBuffer16BitAccess
OpCapability DenormPreserve
OpCapability RuntimeDescriptorArray
OpCapability PhysicalStorageBufferAddresses
OpExtension "SPV_EXT_descriptor_indexing"
OpExtension "SPV_KHR_float_controls"
OpExtension "SPV_KHR_physical_storage_buffer"
OpMemoryModel PhysicalStorageBuffer64 GLSL450
OpEntryPoint Fragment %3 "main" %54 %58 %62
OpExecutionMode %3 OriginUpperLeft
OpExecutionMode %3 DenormPreserve 16
OpName %3 "main"
OpName %6 "RootConstants"
OpName %8 "registers"
OpName %10 "SSBO"
OpName %17 "SSBO"
OpName %23 "SSBO"
OpName %29 "SSBO"
OpName %34 "SSBO"
OpName %39 "SSBO"
OpName %44 "SSBO"
OpName %49 "SSBO"
OpName %54 "INDEX"
OpName %58 "UV"
OpName %62 "SV_Target"
OpDecorate %6 Block
OpMemberDecorate %6 0 Offset 0
OpMemberDecorate %6 1 Offset 4
OpMemberDecorate %6 2 Offset 8
OpMemberDecorate %6 3 Offset 12
OpMemberDecorate %6 4 Offset 16
OpMemberDecorate %6 5 Offset 20
OpMemberDecorate %6 6 Offset 24
OpMemberDecorate %6 7 Offset 28
OpDecorate %9 ArrayStride 4
OpMemberDecorate %10 0 Offset 0
OpDecorate %10 Block
OpDecorate %13 DescriptorSet 1
OpDecorate %13 Binding 0
OpDecorate %13 NonWritable
OpDecorate %13 Restrict
OpDecorate %16 ArrayStride 4
OpMemberDecorate %17 0 Offset 0
OpDecorate %17 Block
OpDecorate %20 DescriptorSet 1
OpDecorate %20 Binding 0
OpDecorate %20 NonWritable
OpDecorate %20 Restrict
OpDecorate %22 ArrayStride 8
OpMemberDecorate %23 0 Offset 0
OpDecorate %23 Block
OpDecorate %26 DescriptorSet 1
OpDecorate %26 Binding 0
OpDecorate %26 NonWritable
OpDecorate %26 Restrict
OpDecorate %28 ArrayStride 16
OpMemberDecorate %29 0 Offset 0
OpDecorate %29 Block
OpDecorate %32 DescriptorSet 1
OpDecorate %32 Binding 0
OpDecorate %32 NonWritable
OpDecorate %32 Restrict
OpDecorate %33 ArrayStride 4
OpMemberDecorate %34 0 Offset 0
OpDecorate %34 Block
OpDecorate %37 DescriptorSet 4
OpDecorate %37 Binding 0
OpDecorate %37 NonWritable
OpDecorate %38 ArrayStride 4
OpMemberDecorate %39 0 Offset 0
OpDecorate %39 Block
OpDecorate %42 DescriptorSet 4
OpDecorate %42 Binding 0
OpDecorate %42 NonWritable
OpDecorate %43 ArrayStride 8
OpMemberDecorate %44 0 Offset 0
OpDecorate %44 Block
OpDecorate %47 DescriptorSet 4
OpDecorate %47 Binding 0
OpDecorate %47 Aliased
OpDecorate %48 ArrayStride 16
OpMemberDecorate %49 0 Offset 0
OpDecorate %49 Block
OpDecorate %52 DescriptorSet 4
OpDecorate %52 Binding 0
OpDecorate %52 Aliased
OpDecorate %54 Flat
OpDecorate %54 Location 0
OpDecorate %58 Flat
OpDecorate %58 Location 1
OpDecorate %62 Location 0
%1 = OpTypeVoid
%2 = OpTypeFunction %1
%5 = OpTypeInt 32 0
%6 = OpTypeStruct %5 %5 %5 %5 %5 %5 %5 %5
%7 = OpTypePointer PushConstant %6
%8 = OpVariable %7 PushConstant
%9 = OpTypeRuntimeArray %5
%10 = OpTypeStruct %9
%11 = OpTypeRuntimeArray %10
%12 = OpTypePointer StorageBuffer %11
%13 = OpVariable %12 StorageBuffer
%14 = OpTypeInt 16 0
%15 = OpTypeVector %14 2
%16 = OpTypeRuntimeArray %15
%17 = OpTypeStruct %16
%18 = OpTypeRuntimeArray %17
%19 = OpTypePointer StorageBuffer %18
%20 = OpVariable %19 StorageBuffer
%21 = OpTypeVector %14 4
%22 = OpTypeRuntimeArray %21
%23 = OpTypeStruct %22
%24 = OpTypeRuntimeArray %23
%25 = OpTypePointer StorageBuffer %24
%26 = OpVariable %25 StorageBuffer
%27 = OpTypeVector %5 4
%28 = OpTypeRuntimeArray %27
%29 = OpTypeStruct %28
%30 = OpTypeRuntimeArray %29
%31 = OpTypePointer StorageBuffer %30
%32 = OpVariable %31 StorageBuffer
%33 = OpTypeRuntimeArray %5
%34 = OpTypeStruct %33
%35 = OpTypeRuntimeArray %34
%36 = OpTypePointer StorageBuffer %35
%37 = OpVariable %36 StorageBuffer
%38 = OpTypeRuntimeArray %15
%39 = OpTypeStruct %38
%40 = OpTypeRuntimeArray %39
%41 = OpTypePointer StorageBuffer %40
%42 = OpVariable %41 StorageBuffer
%43 = OpTypeRuntimeArray %21
%44 = OpTypeStruct %43
%45 = OpTypeRuntimeArray %44
%46 = OpTypePointer StorageBuffer %45
%47 = OpVariable %46 StorageBuffer
%48 = OpTypeRuntimeArray %27
%49 = OpTypeStruct %48
%50 = OpTypeRuntimeArray %49
%51 = OpTypePointer StorageBuffer %50
%52 = OpVariable %51 StorageBuffer
%53 = OpTypePointer Input %5
%54 = OpVariable %53 Input
%55 = OpTypeInt 32 1
%56 = OpTypeVector %55 4
%57 = OpTypePointer Input %56
%58 = OpVariable %57 Input
%59 = OpTypeFloat 32
%60 = OpTypeVector %59 4
%61 = OpTypePointer Output %60
%62 = OpVariable %61 Output
%63 = OpTypePointer Input %55
%65 = OpConstant %5 0
%69 = OpConstant %5 1
%73 = OpConstant %5 2
%77 = OpConstant %5 3
%80 = OpTypePointer PushConstant %5
%83 = OpTypePointer StorageBuffer %10
%85 = OpTypePointer StorageBuffer %5
%92 = OpTypePointer StorageBuffer %17
%94 = OpTypePointer StorageBuffer %15
%97 = OpTypeFloat 16
%98 = OpTypeVector %97 2
%105 = OpConstant %5 4
%108 = OpTypePointer StorageBuffer %34
%117 = OpTypePointer StorageBuffer %39
%130 = OpTypePointer StorageBuffer %23
%132 = OpTypePointer StorageBuffer %29
%134 = OpTypePointer StorageBuffer %21
%137 = OpTypeVector %97 4
%151 = OpTypePointer StorageBuffer %27
%166 = OpConstant %5 5
%167 = OpTypePointer StorageBuffer %44
%169 = OpTypePointer StorageBuffer %49
%192 = OpTypePointer Output %59
%3 = OpFunction %1 None %2
%4 = OpLabel
OpBranch %197
%197 = OpLabel
%64 = OpAccessChain %63 %58 %65
%66 = OpLoad %55 %64
%67 = OpBitcast %5 %66
%68 = OpAccessChain %63 %58 %69
%70 = OpLoad %55 %68
%71 = OpBitcast %5 %70
%72 = OpAccessChain %63 %58 %73
%74 = OpLoad %55 %72
%75 = OpBitcast %5 %74
%76 = OpAccessChain %63 %58 %77
%78 = OpLoad %55 %76
%79 = OpBitcast %5 %78
%81 = OpAccessChain %80 %8 %69
%82 = OpLoad %5 %81
%84 = OpAccessChain %83 %13 %82
%86 = OpAccessChain %85 %84 %65 %67
%87 = OpLoad %5 %86
%88 = OpBitcast %59 %87
%89 = OpAccessChain %80 %8 %69
%90 = OpLoad %5 %89
%91 = OpIAdd %5 %90 %69
%93 = OpAccessChain %92 %20 %91
%95 = OpAccessChain %94 %93 %65 %71
%96 = OpLoad %15 %95
%99 = OpBitcast %98 %96
%100 = OpCompositeExtract %97 %99 0
%101 = OpCompositeExtract %97 %99 1
%102 = OpFConvert %59 %100
%103 = OpFConvert %59 %101
%104 = OpAccessChain %80 %8 %105
%106 = OpLoad %5 %104
%107 = OpIAdd %5 %106 %73
%109 = OpAccessChain %108 %37 %107
%110 = OpAccessChain %85 %109 %65 %75
%111 = OpLoad %5 %110
%112 = OpBitcast %59 %111
%113 = OpFAdd %59 %112 %103
%114 = OpAccessChain %80 %8 %105
%115 = OpLoad %5 %114
%116 = OpIAdd %5 %115 %77
%118 = OpAccessChain %117 %42 %116
%119 = OpAccessChain %94 %118 %65 %79
%120 = OpLoad %15 %119
%121 = OpBitcast %98 %120
%122 = OpCompositeExtract %97 %121 0
%123 = OpCompositeExtract %97 %121 1
%124 = OpFConvert %59 %122
%125 = OpFConvert %59 %123
%126 = OpFAdd %59 %125 %88
%127 = OpAccessChain %80 %8 %69
%128 = OpLoad %5 %127
%129 = OpIAdd %5 %128 %105
%131 = OpAccessChain %130 %26 %129
%133 = OpAccessChain %132 %32 %129
%135 = OpAccessChain %134 %131 %65 %69
%136 = OpLoad %21 %135
%138 = OpBitcast %137 %136
%139 = OpCompositeExtract %97 %138 0
%140 = OpCompositeExtract %97 %138 1
%141 = OpCompositeExtract %97 %138 2
%142 = OpCompositeExtract %97 %138 3
%143 = OpFConvert %59 %139
%144 = OpFConvert %59 %140
%145 = OpFConvert %59 %141
%146 = OpFConvert %59 %142
%147 = OpFAdd %59 %126 %143
%148 = OpFAdd %59 %144 %102
%149 = OpFAdd %59 %113 %145
%150 = OpFAdd %59 %146 %124
%152 = OpAccessChain %151 %133 %65 %69
%153 = OpLoad %27 %152
%154 = OpBitcast %60 %153
%155 = OpCompositeExtract %59 %154 0
%156 = OpCompositeExtract %59 %154 1
%157 = OpCompositeExtract %59 %154 2
%158 = OpCompositeExtract %59 %154 3
%159 = OpFAdd %59 %147 %155
%160 = OpFAdd %59 %148 %156
%161 = OpFAdd %59 %149 %157
%162 = OpFAdd %59 %150 %158
%163 = OpAccessChain %80 %8 %105
%164 = OpLoad %5 %163
%165 = OpIAdd %5 %164 %166
%168 = OpAccessChain %167 %47 %165
%170 = OpAccessChain %169 %52 %165
%171 = OpAccessChain %134 %168 %65 %69
%172 = OpLoad %21 %171
%173 = OpBitcast %137 %172
%174 = OpCompositeExtract %97 %173 0
%175 = OpCompositeExtract %97 %173 1
%176 = OpCompositeExtract %97 %173 2
%177 = OpCompositeExtract %97 %173 3
%178 = OpFConvert %59 %174
%179 = OpFConvert %59 %175
%180 = OpFConvert %59 %176
%181 = OpFConvert %59 %177
%182 = OpFAdd %59 %159 %178
%183 = OpFAdd %59 %160 %179
%184 = OpFAdd %59 %161 %180
%185 = OpFAdd %59 %162 %181
%186 = OpBitcast %5 %182
%187 = OpBitcast %5 %183
%188 = OpBitcast %5 %184
%189 = OpBitcast %5 %185
%190 = OpCompositeConstruct %27 %186 %187 %188 %189
%191 = OpAccessChain %151 %170 %65 %69
OpStore %191 %190
%193 = OpAccessChain %192 %62 %65
OpStore %193 %182
%194 = OpAccessChain %192 %62 %69
OpStore %194 %183
%195 = OpAccessChain %192 %62 %73
OpStore %195 %184
%196 = OpAccessChain %192 %62 %77
OpStore %196 %185
OpReturn
OpFunctionEnd
#endif
