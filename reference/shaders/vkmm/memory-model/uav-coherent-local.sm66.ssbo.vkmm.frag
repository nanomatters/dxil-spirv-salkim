#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0, std430) buffer SSBO
{
    uint _m0[];
} _10[];

layout(location = 0) out vec4 SV_Target;

uint ByteAddressMask(uint index, uint stride)
{
    return index & (4294967295u / stride);
}

void main()
{
    uint _26 = uint(gl_FragCoord.x);
    _10[0u]._m0[ByteAddressMask(_26, 4u)] = uint(gl_FragCoord.y);
    float _48 = float(_10[0u]._m0[ByteAddressMask(_26 + 1u, 4u)]);
    SV_Target.x = _48;
    SV_Target.y = _48;
    SV_Target.z = _48;
    SV_Target.w = _48;
}


#if 0
// SPIR-V disassembly
; SPIR-V
; Version: 1.6
; Generator: Unknown(30017); 21022
; Bound: 56
; Schema: 0
OpCapability Shader
OpCapability RuntimeDescriptorArray
OpCapability VulkanMemoryModel
OpExtension "SPV_EXT_descriptor_indexing"
OpMemoryModel Logical Vulkan
OpEntryPoint Fragment %3 "main" %10 %14 %16
OpExecutionMode %3 OriginUpperLeft
OpName %3 "main"
OpName %7 "SSBO"
OpName %14 "SV_Position"
OpName %16 "SV_Target"
OpName %33 "ByteAddressMask"
OpName %31 "index"
OpName %32 "stride"
OpDecorate %6 ArrayStride 4
OpMemberDecorate %7 0 Offset 0
OpDecorate %7 Block
OpDecorate %10 DescriptorSet 0
OpDecorate %10 Binding 0
OpDecorate %14 BuiltIn FragCoord
OpDecorate %16 Location 0
%1 = OpTypeVoid
%2 = OpTypeFunction %1
%5 = OpTypeInt 32 0
%6 = OpTypeRuntimeArray %5
%7 = OpTypeStruct %6
%8 = OpTypeRuntimeArray %7
%9 = OpTypePointer StorageBuffer %8
%10 = OpVariable %9 StorageBuffer
%11 = OpTypeFloat 32
%12 = OpTypeVector %11 4
%13 = OpTypePointer Input %12
%14 = OpVariable %13 Input
%15 = OpTypePointer Output %12
%16 = OpVariable %15 Output
%17 = OpTypePointer Input %11
%19 = OpConstant %5 0
%22 = OpConstant %5 1
%24 = OpTypePointer StorageBuffer %7
%28 = OpConstant %5 2
%30 = OpTypeFunction %5 %5 %5
%36 = OpConstant %5 4294967295
%39 = OpConstant %5 4
%40 = OpTypePointer StorageBuffer %5
%42 = OpConstant %5 2120
%49 = OpTypePointer Output %11
%54 = OpConstant %5 3
%3 = OpFunction %1 None %2
%4 = OpLabel
OpBranch %55
%55 = OpLabel
%18 = OpAccessChain %17 %14 %19
%20 = OpLoad %11 %18
%21 = OpAccessChain %17 %14 %22
%23 = OpLoad %11 %21
%25 = OpAccessChain %24 %10 %19
%26 = OpConvertFToU %5 %20
%27 = OpShiftLeftLogical %5 %26 %28
%29 = OpConvertFToU %5 %23
%38 = OpFunctionCall %5 %33 %26 %39
%41 = OpAccessChain %40 %25 %19 %38
OpStore %41 %29 NonPrivatePointer
%43 = OpIAdd %5 %27 %39
%44 = OpIAdd %5 %26 %22
%45 = OpFunctionCall %5 %33 %44 %39
%46 = OpAccessChain %40 %25 %19 %45
%47 = OpLoad %5 %46 NonPrivatePointer
%48 = OpConvertUToF %11 %47
%50 = OpAccessChain %49 %16 %19
OpStore %50 %48
%51 = OpAccessChain %49 %16 %22
OpStore %51 %48
%52 = OpAccessChain %49 %16 %28
OpStore %52 %48
%53 = OpAccessChain %49 %16 %54
OpStore %53 %48
OpReturn
OpFunctionEnd
%33 = OpFunction %5 None %30
%31 = OpFunctionParameter %5
%32 = OpFunctionParameter %5
%34 = OpLabel
%35 = OpUDiv %5 %36 %32
%37 = OpBitwiseAnd %5 %31 %35
OpReturnValue %37
OpFunctionEnd
#endif
