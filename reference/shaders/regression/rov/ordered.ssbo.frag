#version 460
#ifdef GL_ARB_fragment_shader_interlock
#extension GL_ARB_fragment_shader_interlock : enable
#define SPIRV_Cross_beginInvocationInterlock() beginInvocationInterlockARB()
#define SPIRV_Cross_endInvocationInterlock() endInvocationInterlockARB()
#elif defined(GL_INTEL_fragment_shader_ordering)
#extension GL_INTEL_fragment_shader_ordering : enable
#define SPIRV_Cross_beginInvocationInterlock() beginFragmentShaderOrderingINTEL()
#define SPIRV_Cross_endInvocationInterlock()
#endif
#if defined(GL_ARB_fragment_shader_interlock)
layout(pixel_interlock_ordered) in;
#elif !defined(GL_INTEL_fragment_shader_ordering)
#error Fragment Shader Interlock/Ordering extension missing!
#endif
layout(early_fragment_tests) in;

layout(set = 0, binding = 0, std430) buffer SSBO
{
    uint _m0[];
} _9;

layout(set = 0, binding = 1, std430) buffer _12_14
{
    uvec2 _m0[];
} _14;

layout(set = 0, binding = 2, std430) writeonly buffer _16_18
{
    uint _m0[];
} _18;

uint ByteAddressMask(uint index, uint stride)
{
    return index & (4294967295u / stride);
}

void main()
{
    uint _27 = uint(gl_FragCoord.x);
    SPIRV_Cross_beginInvocationInterlock();
    if (!((_27 & 1u) == 0u))
    {
        _9._m0[ByteAddressMask(_27, 4u)]++;
    }
    _14._m0[_27] = uvec2(_14._m0[_27].x + 1u, _14._m0[_27].y + 2u);
    SPIRV_Cross_endInvocationInterlock();
    _18._m0[ByteAddressMask(_27, 4u)] = _27;
}


#if 0
// SPIR-V disassembly
; SPIR-V
; Version: 1.6
; Generator: Unknown(30017); 21022
; Bound: 66
; Schema: 0
OpCapability Shader
OpCapability VulkanMemoryModel
OpCapability FragmentShaderPixelInterlockEXT
OpExtension "SPV_EXT_fragment_shader_interlock"
OpMemoryModel Logical Vulkan
OpEntryPoint Fragment %3 "main" %9 %14 %18 %22
OpExecutionMode %3 OriginUpperLeft
OpExecutionMode %3 EarlyFragmentTests
OpExecutionMode %3 PixelInterlockOrderedEXT
OpName %3 "main"
OpName %7 "SSBO"
OpName %12 "SSBO"
OpName %16 "SSBO"
OpName %22 "SV_Position"
OpName %37 "ByteAddressMask"
OpName %35 "index"
OpName %36 "stride"
OpDecorate %6 ArrayStride 4
OpMemberDecorate %7 0 Offset 0
OpDecorate %7 Block
OpDecorate %9 DescriptorSet 0
OpDecorate %9 Binding 0
OpDecorate %11 ArrayStride 8
OpMemberDecorate %12 0 Offset 0
OpDecorate %12 Block
OpDecorate %14 DescriptorSet 0
OpDecorate %14 Binding 1
OpDecorate %15 ArrayStride 4
OpMemberDecorate %16 0 Offset 0
OpDecorate %16 Block
OpDecorate %18 DescriptorSet 0
OpDecorate %18 Binding 2
OpDecorate %18 NonReadable
OpDecorate %22 BuiltIn FragCoord
%1 = OpTypeVoid
%2 = OpTypeFunction %1
%5 = OpTypeInt 32 0
%6 = OpTypeRuntimeArray %5
%7 = OpTypeStruct %6
%8 = OpTypePointer StorageBuffer %7
%9 = OpVariable %8 StorageBuffer
%10 = OpTypeVector %5 2
%11 = OpTypeRuntimeArray %10
%12 = OpTypeStruct %11
%13 = OpTypePointer StorageBuffer %12
%14 = OpVariable %13 StorageBuffer
%15 = OpTypeRuntimeArray %5
%16 = OpTypeStruct %15
%17 = OpTypePointer StorageBuffer %16
%18 = OpVariable %17 StorageBuffer
%19 = OpTypeFloat 32
%20 = OpTypeVector %19 4
%21 = OpTypePointer Input %20
%22 = OpVariable %21 Input
%23 = OpTypePointer Input %19
%25 = OpConstant %5 0
%29 = OpConstant %5 1
%30 = OpTypeBool
%33 = OpConstant %5 2
%34 = OpTypeFunction %5 %5 %5
%40 = OpConstant %5 4294967295
%43 = OpConstant %5 4
%44 = OpTypePointer StorageBuffer %5
%47 = OpConstant %5 5
%51 = OpTypePointer StorageBuffer %10
%3 = OpFunction %1 None %2
%4 = OpLabel
OpBranch %62
%62 = OpLabel
%24 = OpAccessChain %23 %22 %25
%26 = OpLoad %19 %24
%27 = OpConvertFToU %5 %26
%28 = OpBitwiseAnd %5 %27 %29
%31 = OpIEqual %30 %28 %25
%32 = OpShiftLeftLogical %5 %27 %33
OpBeginInvocationInterlockEXT
OpSelectionMerge %64 None
OpBranchConditional %31 %64 %63
%63 = OpLabel
%42 = OpFunctionCall %5 %37 %27 %43
%45 = OpAccessChain %44 %9 %25 %42
%46 = OpLoad %5 %45 MakePointerVisible|NonPrivatePointer %47
%48 = OpIAdd %5 %46 %29
%49 = OpFunctionCall %5 %37 %27 %43
%50 = OpAccessChain %44 %9 %25 %49
OpStore %50 %48 MakePointerAvailable|NonPrivatePointer %47
OpBranch %64
%64 = OpLabel
OpBranch %65
%65 = OpLabel
%52 = OpAccessChain %51 %14 %25 %27
%53 = OpLoad %10 %52 MakePointerVisible|NonPrivatePointer %47
%54 = OpCompositeExtract %5 %53 0
%55 = OpCompositeExtract %5 %53 1
%56 = OpIAdd %5 %54 %29
%57 = OpIAdd %5 %55 %33
%58 = OpCompositeConstruct %10 %56 %57
%59 = OpAccessChain %51 %14 %25 %27
OpStore %59 %58 MakePointerAvailable|NonPrivatePointer %47
OpEndInvocationInterlockEXT
%60 = OpFunctionCall %5 %37 %27 %43
%61 = OpAccessChain %44 %18 %25 %60
OpStore %61 %27 NonPrivatePointer
OpReturn
OpFunctionEnd
%37 = OpFunction %5 None %34
%35 = OpFunctionParameter %5
%36 = OpFunctionParameter %5
%38 = OpLabel
%39 = OpUDiv %5 %40 %36
%41 = OpBitwiseAnd %5 %35 %39
OpReturnValue %41
OpFunctionEnd
#endif
