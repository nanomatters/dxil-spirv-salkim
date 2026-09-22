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
#extension GL_KHR_shader_subgroup_ballot : require

layout(set = 15, binding = 0, std430) restrict readonly buffer SSBO_Offsets
{
    uvec2 _m0[];
} _13;

layout(set = 1, binding = 0, std430) restrict readonly buffer SSBO
{
    uint _m0[];
} _18[];

layout(set = 1, binding = 0, std430) restrict readonly buffer _22_25
{
    u16vec2 _m0[];
} _25[];

layout(set = 1, binding = 0, std430) restrict readonly buffer _28_31
{
    u16vec4 _m0[];
} _31[];

layout(set = 1, binding = 0, std430) restrict readonly buffer _34_37
{
    uvec4 _m0[];
} _37[];

layout(set = 4, binding = 0, std430) readonly buffer _39_42
{
    uint _m0[];
} _42[];

layout(set = 4, binding = 0, std430) readonly buffer _44_47
{
    u16vec2 _m0[];
} _47[];

layout(set = 4, binding = 0, std430) buffer _49_52
{
    u16vec4 _m0[];
} _52[];

layout(set = 4, binding = 0, std430) buffer _54_57
{
    uvec4 _m0[];
} _57[];

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
    uint _72 = uint(UV.x);
    uint _76 = uint(UV.y);
    uint _80 = uint(UV.z);
    uint _84 = uint(UV.w);
    uvec2 _94 = _13._m0[subgroupBroadcastFirst(registers._m1)] >> uvec2(2u);
    uint _109 = registers._m1 + 1u;
    uvec2 _115 = _13._m0[subgroupBroadcastFirst(_109)] >> uvec2(2u);
    f16vec2 _127 = uint16BitsToFloat16(_25[_109]._m0[(_76 < _115.y) ? (_76 + _115.x) : 1073741823u]);
    uint _135 = registers._m4 + 2u;
    uvec2 _141 = _13._m0[subgroupBroadcastFirst(_135)] >> uvec2(2u);
    uint _148 = _42[_135]._m0[(_80 < _141.y) ? (_80 + _141.x) : 1073741820u];
    uint _153 = registers._m4 + 3u;
    uvec2 _159 = _13._m0[subgroupBroadcastFirst(_153)] >> uvec2(2u);
    u16vec2 _166 = _47[_153]._m0[(_84 < _159.y) ? (_84 + _159.x) : 1073741823u];
    f16vec2 _167 = uint16BitsToFloat16(_166);
    uint _175 = registers._m1 + 4u;
    uint _180 = subgroupBroadcastFirst(_175);
    uvec2 _183 = _13._m0[_180] >> uvec2(3u);
    f16vec4 _195 = uint16BitsToFloat16(_31[_175]._m0[(1u < _183.y) ? (1u + _183.x) : 536870911u]);
    uvec2 _208 = _13._m0[_180] >> uvec2(4u);
    vec4 _219 = uintBitsToFloat(_37[_175]._m0[(1u < _208.y) ? (1u + _208.x) : 268435455u]);
    uint _230 = registers._m4 + 5u;
    uint _236 = subgroupBroadcastFirst(_230);
    uvec2 _239 = _13._m0[_236] >> uvec2(3u);
    u16vec4 _246 = _52[_230]._m0[(1u < _239.y) ? (1u + _239.x) : 536870911u];
    f16vec4 _247 = uint16BitsToFloat16(_246);
    float _256 = (((float(_167.y) + uintBitsToFloat(_18[registers._m1]._m0[(_72 < _94.y) ? (_72 + _94.x) : 1073741820u])) + float(_195.x)) + _219.x) + float(_247.x);
    float _257 = ((float(_195.y) + float(_127.x)) + _219.y) + float(_247.y);
    float _258 = (((uintBitsToFloat(_148) + float(_127.y)) + float(_195.z)) + _219.z) + float(_247.z);
    float _259 = ((float(_195.w) + float(_167.x)) + _219.w) + float(_247.w);
    uvec2 _260 = _13._m0[_236] >> uvec2(4u);
    _57[_230]._m0[(1u < _260.y) ? (1u + _260.x) : 268435455u] = uvec4(floatBitsToUint(_256), floatBitsToUint(_257), floatBitsToUint(_258), floatBitsToUint(_259));
    SV_Target.x = _256;
    SV_Target.y = _257;
    SV_Target.z = _258;
    SV_Target.w = _259;
}


#if 0
// SPIR-V disassembly
; SPIR-V
; Version: 1.3
; Generator: Unknown(30017); 21022
; Bound: 279
; Schema: 0
OpCapability Shader
OpCapability Float16
OpCapability Int16
OpCapability GroupNonUniformBallot
OpCapability StorageBuffer16BitAccess
OpCapability DenormPreserve
OpCapability RuntimeDescriptorArray
OpCapability PhysicalStorageBufferAddresses
OpExtension "SPV_EXT_descriptor_indexing"
OpExtension "SPV_KHR_float_controls"
OpExtension "SPV_KHR_physical_storage_buffer"
OpMemoryModel PhysicalStorageBuffer64 GLSL450
OpEntryPoint Fragment %3 "main" %59 %63 %67
OpExecutionMode %3 OriginUpperLeft
OpExecutionMode %3 DenormPreserve 16
OpName %3 "main"
OpName %6 "RootConstants"
OpName %8 "registers"
OpName %11 "SSBO_Offsets"
OpName %15 "SSBO"
OpName %22 "SSBO"
OpName %28 "SSBO"
OpName %34 "SSBO"
OpName %39 "SSBO"
OpName %44 "SSBO"
OpName %49 "SSBO"
OpName %54 "SSBO"
OpName %59 "INDEX"
OpName %63 "UV"
OpName %67 "SV_Target"
OpDecorate %6 Block
OpMemberDecorate %6 0 Offset 0
OpMemberDecorate %6 1 Offset 4
OpMemberDecorate %6 2 Offset 8
OpMemberDecorate %6 3 Offset 12
OpMemberDecorate %6 4 Offset 16
OpMemberDecorate %6 5 Offset 20
OpMemberDecorate %6 6 Offset 24
OpMemberDecorate %6 7 Offset 28
OpDecorate %10 ArrayStride 8
OpMemberDecorate %11 0 Offset 0
OpDecorate %11 Block
OpDecorate %13 DescriptorSet 15
OpDecorate %13 Binding 0
OpDecorate %13 NonWritable
OpDecorate %13 Restrict
OpDecorate %14 ArrayStride 4
OpMemberDecorate %15 0 Offset 0
OpDecorate %15 Block
OpDecorate %18 DescriptorSet 1
OpDecorate %18 Binding 0
OpDecorate %18 NonWritable
OpDecorate %18 Restrict
OpDecorate %21 ArrayStride 4
OpMemberDecorate %22 0 Offset 0
OpDecorate %22 Block
OpDecorate %25 DescriptorSet 1
OpDecorate %25 Binding 0
OpDecorate %25 NonWritable
OpDecorate %25 Restrict
OpDecorate %27 ArrayStride 8
OpMemberDecorate %28 0 Offset 0
OpDecorate %28 Block
OpDecorate %31 DescriptorSet 1
OpDecorate %31 Binding 0
OpDecorate %31 NonWritable
OpDecorate %31 Restrict
OpDecorate %33 ArrayStride 16
OpMemberDecorate %34 0 Offset 0
OpDecorate %34 Block
OpDecorate %37 DescriptorSet 1
OpDecorate %37 Binding 0
OpDecorate %37 NonWritable
OpDecorate %37 Restrict
OpDecorate %38 ArrayStride 4
OpMemberDecorate %39 0 Offset 0
OpDecorate %39 Block
OpDecorate %42 DescriptorSet 4
OpDecorate %42 Binding 0
OpDecorate %42 NonWritable
OpDecorate %43 ArrayStride 4
OpMemberDecorate %44 0 Offset 0
OpDecorate %44 Block
OpDecorate %47 DescriptorSet 4
OpDecorate %47 Binding 0
OpDecorate %47 NonWritable
OpDecorate %48 ArrayStride 8
OpMemberDecorate %49 0 Offset 0
OpDecorate %49 Block
OpDecorate %52 DescriptorSet 4
OpDecorate %52 Binding 0
OpDecorate %52 Aliased
OpDecorate %53 ArrayStride 16
OpMemberDecorate %54 0 Offset 0
OpDecorate %54 Block
OpDecorate %57 DescriptorSet 4
OpDecorate %57 Binding 0
OpDecorate %57 Aliased
OpDecorate %59 Flat
OpDecorate %59 Location 0
OpDecorate %63 Flat
OpDecorate %63 Location 1
OpDecorate %67 Location 0
%1 = OpTypeVoid
%2 = OpTypeFunction %1
%5 = OpTypeInt 32 0
%6 = OpTypeStruct %5 %5 %5 %5 %5 %5 %5 %5
%7 = OpTypePointer PushConstant %6
%8 = OpVariable %7 PushConstant
%9 = OpTypeVector %5 2
%10 = OpTypeRuntimeArray %9
%11 = OpTypeStruct %10
%12 = OpTypePointer StorageBuffer %11
%13 = OpVariable %12 StorageBuffer
%14 = OpTypeRuntimeArray %5
%15 = OpTypeStruct %14
%16 = OpTypeRuntimeArray %15
%17 = OpTypePointer StorageBuffer %16
%18 = OpVariable %17 StorageBuffer
%19 = OpTypeInt 16 0
%20 = OpTypeVector %19 2
%21 = OpTypeRuntimeArray %20
%22 = OpTypeStruct %21
%23 = OpTypeRuntimeArray %22
%24 = OpTypePointer StorageBuffer %23
%25 = OpVariable %24 StorageBuffer
%26 = OpTypeVector %19 4
%27 = OpTypeRuntimeArray %26
%28 = OpTypeStruct %27
%29 = OpTypeRuntimeArray %28
%30 = OpTypePointer StorageBuffer %29
%31 = OpVariable %30 StorageBuffer
%32 = OpTypeVector %5 4
%33 = OpTypeRuntimeArray %32
%34 = OpTypeStruct %33
%35 = OpTypeRuntimeArray %34
%36 = OpTypePointer StorageBuffer %35
%37 = OpVariable %36 StorageBuffer
%38 = OpTypeRuntimeArray %5
%39 = OpTypeStruct %38
%40 = OpTypeRuntimeArray %39
%41 = OpTypePointer StorageBuffer %40
%42 = OpVariable %41 StorageBuffer
%43 = OpTypeRuntimeArray %20
%44 = OpTypeStruct %43
%45 = OpTypeRuntimeArray %44
%46 = OpTypePointer StorageBuffer %45
%47 = OpVariable %46 StorageBuffer
%48 = OpTypeRuntimeArray %26
%49 = OpTypeStruct %48
%50 = OpTypeRuntimeArray %49
%51 = OpTypePointer StorageBuffer %50
%52 = OpVariable %51 StorageBuffer
%53 = OpTypeRuntimeArray %32
%54 = OpTypeStruct %53
%55 = OpTypeRuntimeArray %54
%56 = OpTypePointer StorageBuffer %55
%57 = OpVariable %56 StorageBuffer
%58 = OpTypePointer Input %5
%59 = OpVariable %58 Input
%60 = OpTypeInt 32 1
%61 = OpTypeVector %60 4
%62 = OpTypePointer Input %61
%63 = OpVariable %62 Input
%64 = OpTypeFloat 32
%65 = OpTypeVector %64 4
%66 = OpTypePointer Output %65
%67 = OpVariable %66 Output
%68 = OpTypePointer Input %60
%70 = OpConstant %5 0
%74 = OpConstant %5 1
%78 = OpConstant %5 2
%82 = OpConstant %5 3
%85 = OpTypePointer PushConstant %5
%88 = OpTypePointer StorageBuffer %15
%91 = OpTypePointer StorageBuffer %9
%95 = OpConstantComposite %9 %78 %78
%99 = OpTypeBool
%102 = OpConstant %5 1073741820
%103 = OpTypePointer StorageBuffer %5
%110 = OpTypePointer StorageBuffer %22
%121 = OpConstant %5 1073741823
%122 = OpTypePointer StorageBuffer %20
%125 = OpTypeFloat 16
%126 = OpTypeVector %125 2
%133 = OpConstant %5 4
%136 = OpTypePointer StorageBuffer %39
%154 = OpTypePointer StorageBuffer %44
%176 = OpTypePointer StorageBuffer %28
%178 = OpTypePointer StorageBuffer %34
%184 = OpConstantComposite %9 %82 %82
%190 = OpConstant %5 536870911
%191 = OpTypePointer StorageBuffer %26
%194 = OpTypeVector %125 4
%209 = OpConstantComposite %9 %133 %133
%215 = OpConstant %5 268435455
%216 = OpTypePointer StorageBuffer %32
%231 = OpConstant %5 5
%232 = OpTypePointer StorageBuffer %49
%234 = OpTypePointer StorageBuffer %54
%272 = OpTypePointer Output %64
%3 = OpFunction %1 None %2
%4 = OpLabel
OpBranch %277
%277 = OpLabel
%69 = OpAccessChain %68 %63 %70
%71 = OpLoad %60 %69
%72 = OpBitcast %5 %71
%73 = OpAccessChain %68 %63 %74
%75 = OpLoad %60 %73
%76 = OpBitcast %5 %75
%77 = OpAccessChain %68 %63 %78
%79 = OpLoad %60 %77
%80 = OpBitcast %5 %79
%81 = OpAccessChain %68 %63 %82
%83 = OpLoad %60 %81
%84 = OpBitcast %5 %83
%86 = OpAccessChain %85 %8 %74
%87 = OpLoad %5 %86
%89 = OpAccessChain %88 %18 %87
%90 = OpGroupNonUniformBroadcastFirst %5 %82 %87
%92 = OpAccessChain %91 %13 %70 %90
%93 = OpLoad %9 %92
%94 = OpShiftRightLogical %9 %93 %95
%96 = OpCompositeExtract %5 %94 0
%97 = OpCompositeExtract %5 %94 1
%98 = OpIAdd %5 %72 %96
%100 = OpULessThan %99 %72 %97
%101 = OpSelect %5 %100 %98 %102
%104 = OpAccessChain %103 %89 %70 %101
%105 = OpLoad %5 %104
%106 = OpBitcast %64 %105
%107 = OpAccessChain %85 %8 %74
%108 = OpLoad %5 %107
%109 = OpIAdd %5 %108 %74
%111 = OpAccessChain %110 %25 %109
%112 = OpGroupNonUniformBroadcastFirst %5 %82 %109
%113 = OpAccessChain %91 %13 %70 %112
%114 = OpLoad %9 %113
%115 = OpShiftRightLogical %9 %114 %95
%116 = OpCompositeExtract %5 %115 0
%117 = OpCompositeExtract %5 %115 1
%118 = OpIAdd %5 %76 %116
%119 = OpULessThan %99 %76 %117
%120 = OpSelect %5 %119 %118 %121
%123 = OpAccessChain %122 %111 %70 %120
%124 = OpLoad %20 %123
%127 = OpBitcast %126 %124
%128 = OpCompositeExtract %125 %127 0
%129 = OpCompositeExtract %125 %127 1
%130 = OpFConvert %64 %128
%131 = OpFConvert %64 %129
%132 = OpAccessChain %85 %8 %133
%134 = OpLoad %5 %132
%135 = OpIAdd %5 %134 %78
%137 = OpAccessChain %136 %42 %135
%138 = OpGroupNonUniformBroadcastFirst %5 %82 %135
%139 = OpAccessChain %91 %13 %70 %138
%140 = OpLoad %9 %139
%141 = OpShiftRightLogical %9 %140 %95
%142 = OpCompositeExtract %5 %141 0
%143 = OpCompositeExtract %5 %141 1
%144 = OpIAdd %5 %80 %142
%145 = OpULessThan %99 %80 %143
%146 = OpSelect %5 %145 %144 %102
%147 = OpAccessChain %103 %137 %70 %146
%148 = OpLoad %5 %147
%149 = OpBitcast %64 %148
%150 = OpFAdd %64 %149 %131
%151 = OpAccessChain %85 %8 %133
%152 = OpLoad %5 %151
%153 = OpIAdd %5 %152 %82
%155 = OpAccessChain %154 %47 %153
%156 = OpGroupNonUniformBroadcastFirst %5 %82 %153
%157 = OpAccessChain %91 %13 %70 %156
%158 = OpLoad %9 %157
%159 = OpShiftRightLogical %9 %158 %95
%160 = OpCompositeExtract %5 %159 0
%161 = OpCompositeExtract %5 %159 1
%162 = OpIAdd %5 %84 %160
%163 = OpULessThan %99 %84 %161
%164 = OpSelect %5 %163 %162 %121
%165 = OpAccessChain %122 %155 %70 %164
%166 = OpLoad %20 %165
%167 = OpBitcast %126 %166
%168 = OpCompositeExtract %125 %167 0
%169 = OpCompositeExtract %125 %167 1
%170 = OpFConvert %64 %168
%171 = OpFConvert %64 %169
%172 = OpFAdd %64 %171 %106
%173 = OpAccessChain %85 %8 %74
%174 = OpLoad %5 %173
%175 = OpIAdd %5 %174 %133
%177 = OpAccessChain %176 %31 %175
%179 = OpAccessChain %178 %37 %175
%180 = OpGroupNonUniformBroadcastFirst %5 %82 %175
%181 = OpAccessChain %91 %13 %70 %180
%182 = OpLoad %9 %181
%183 = OpShiftRightLogical %9 %182 %184
%185 = OpCompositeExtract %5 %183 0
%186 = OpCompositeExtract %5 %183 1
%187 = OpIAdd %5 %74 %185
%188 = OpULessThan %99 %74 %186
%189 = OpSelect %5 %188 %187 %190
%192 = OpAccessChain %191 %177 %70 %189
%193 = OpLoad %26 %192
%195 = OpBitcast %194 %193
%196 = OpCompositeExtract %125 %195 0
%197 = OpCompositeExtract %125 %195 1
%198 = OpCompositeExtract %125 %195 2
%199 = OpCompositeExtract %125 %195 3
%200 = OpFConvert %64 %196
%201 = OpFConvert %64 %197
%202 = OpFConvert %64 %198
%203 = OpFConvert %64 %199
%204 = OpFAdd %64 %172 %200
%205 = OpFAdd %64 %201 %130
%206 = OpFAdd %64 %150 %202
%207 = OpFAdd %64 %203 %170
%208 = OpShiftRightLogical %9 %182 %209
%210 = OpCompositeExtract %5 %208 0
%211 = OpCompositeExtract %5 %208 1
%212 = OpIAdd %5 %74 %210
%213 = OpULessThan %99 %74 %211
%214 = OpSelect %5 %213 %212 %215
%217 = OpAccessChain %216 %179 %70 %214
%218 = OpLoad %32 %217
%219 = OpBitcast %65 %218
%220 = OpCompositeExtract %64 %219 0
%221 = OpCompositeExtract %64 %219 1
%222 = OpCompositeExtract %64 %219 2
%223 = OpCompositeExtract %64 %219 3
%224 = OpFAdd %64 %204 %220
%225 = OpFAdd %64 %205 %221
%226 = OpFAdd %64 %206 %222
%227 = OpFAdd %64 %207 %223
%228 = OpAccessChain %85 %8 %133
%229 = OpLoad %5 %228
%230 = OpIAdd %5 %229 %231
%233 = OpAccessChain %232 %52 %230
%235 = OpAccessChain %234 %57 %230
%236 = OpGroupNonUniformBroadcastFirst %5 %82 %230
%237 = OpAccessChain %91 %13 %70 %236
%238 = OpLoad %9 %237
%239 = OpShiftRightLogical %9 %238 %184
%240 = OpCompositeExtract %5 %239 0
%241 = OpCompositeExtract %5 %239 1
%242 = OpIAdd %5 %74 %240
%243 = OpULessThan %99 %74 %241
%244 = OpSelect %5 %243 %242 %190
%245 = OpAccessChain %191 %233 %70 %244
%246 = OpLoad %26 %245
%247 = OpBitcast %194 %246
%248 = OpCompositeExtract %125 %247 0
%249 = OpCompositeExtract %125 %247 1
%250 = OpCompositeExtract %125 %247 2
%251 = OpCompositeExtract %125 %247 3
%252 = OpFConvert %64 %248
%253 = OpFConvert %64 %249
%254 = OpFConvert %64 %250
%255 = OpFConvert %64 %251
%256 = OpFAdd %64 %224 %252
%257 = OpFAdd %64 %225 %253
%258 = OpFAdd %64 %226 %254
%259 = OpFAdd %64 %227 %255
%260 = OpShiftRightLogical %9 %238 %209
%261 = OpCompositeExtract %5 %260 0
%262 = OpCompositeExtract %5 %260 1
%263 = OpIAdd %5 %74 %261
%264 = OpULessThan %99 %74 %262
%265 = OpSelect %5 %264 %263 %215
%266 = OpBitcast %5 %256
%267 = OpBitcast %5 %257
%268 = OpBitcast %5 %258
%269 = OpBitcast %5 %259
%270 = OpCompositeConstruct %32 %266 %267 %268 %269
%271 = OpAccessChain %216 %235 %70 %265
OpStore %271 %270
%273 = OpAccessChain %272 %67 %70
OpStore %273 %256
%274 = OpAccessChain %272 %67 %74
OpStore %274 %257
%275 = OpAccessChain %272 %67 %78
OpStore %275 %258
%276 = OpAccessChain %272 %67 %82
OpStore %276 %259
OpReturn
OpFunctionEnd
#endif
