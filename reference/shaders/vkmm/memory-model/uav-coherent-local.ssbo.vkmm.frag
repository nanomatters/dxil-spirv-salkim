#version 460

layout(set = 0, binding = 0, std430) buffer SSBO
{
    uint _m0[];
} _9;

layout(location = 0) out vec4 SV_Target;

uint ByteAddressMask(uint index, uint stride)
{
    return index & (4294967295u / stride);
}

void main()
{
    uint _23 = uint(gl_FragCoord.x);
    _9._m0[ByteAddressMask(_23, 4u)] = uint(gl_FragCoord.y);
    float _45 = float(_9._m0[ByteAddressMask(_23 + 1u, 4u)]);
    SV_Target.x = _45;
    SV_Target.y = _45;
    SV_Target.z = _45;
    SV_Target.w = _45;
}


#if 0
// SPIR-V disassembly
; SPIR-V
; Version: 1.6
; Generator: Unknown(30017); 21022
; Bound: 53
; Schema: 0
OpCapability Shader
OpCapability VulkanMemoryModel
OpMemoryModel Logical Vulkan
OpEntryPoint Fragment %3 "main" %9 %13 %15
OpExecutionMode %3 OriginUpperLeft
OpName %3 "main"
OpName %7 "SSBO"
OpName %13 "SV_Position"
OpName %15 "SV_Target"
OpName %30 "ByteAddressMask"
OpName %28 "index"
OpName %29 "stride"
OpDecorate %6 ArrayStride 4
OpMemberDecorate %7 0 Offset 0
OpDecorate %7 Block
OpDecorate %9 DescriptorSet 0
OpDecorate %9 Binding 0
OpDecorate %13 BuiltIn FragCoord
OpDecorate %15 Location 0
%1 = OpTypeVoid
%2 = OpTypeFunction %1
%5 = OpTypeInt 32 0
%6 = OpTypeRuntimeArray %5
%7 = OpTypeStruct %6
%8 = OpTypePointer StorageBuffer %7
%9 = OpVariable %8 StorageBuffer
%10 = OpTypeFloat 32
%11 = OpTypeVector %10 4
%12 = OpTypePointer Input %11
%13 = OpVariable %12 Input
%14 = OpTypePointer Output %11
%15 = OpVariable %14 Output
%16 = OpTypePointer Input %10
%18 = OpConstant %5 0
%21 = OpConstant %5 1
%25 = OpConstant %5 2
%27 = OpTypeFunction %5 %5 %5
%33 = OpConstant %5 4294967295
%36 = OpConstant %5 4
%37 = OpTypePointer StorageBuffer %5
%39 = OpConstant %5 2120
%46 = OpTypePointer Output %10
%51 = OpConstant %5 3
%3 = OpFunction %1 None %2
%4 = OpLabel
OpBranch %52
%52 = OpLabel
%17 = OpAccessChain %16 %13 %18
%19 = OpLoad %10 %17
%20 = OpAccessChain %16 %13 %21
%22 = OpLoad %10 %20
%23 = OpConvertFToU %5 %19
%24 = OpShiftLeftLogical %5 %23 %25
%26 = OpConvertFToU %5 %22
%35 = OpFunctionCall %5 %30 %23 %36
%38 = OpAccessChain %37 %9 %18 %35
OpStore %38 %26 NonPrivatePointer
%40 = OpIAdd %5 %24 %36
%41 = OpIAdd %5 %23 %21
%42 = OpFunctionCall %5 %30 %41 %36
%43 = OpAccessChain %37 %9 %18 %42
%44 = OpLoad %5 %43 NonPrivatePointer
%45 = OpConvertUToF %10 %44
%47 = OpAccessChain %46 %15 %18
OpStore %47 %45
%48 = OpAccessChain %46 %15 %21
OpStore %48 %45
%49 = OpAccessChain %46 %15 %25
OpStore %49 %45
%50 = OpAccessChain %46 %15 %51
OpStore %50 %45
OpReturn
OpFunctionEnd
%30 = OpFunction %5 None %27
%28 = OpFunctionParameter %5
%29 = OpFunctionParameter %5
%31 = OpLabel
%32 = OpUDiv %5 %33 %29
%34 = OpBitwiseAnd %5 %28 %32
OpReturnValue %34
OpFunctionEnd
#endif
