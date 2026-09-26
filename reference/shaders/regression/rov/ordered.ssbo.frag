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

layout(set = 0, binding = 0, std430) coherent buffer SSBO
{
    uint _m0[];
} _9;

layout(set = 0, binding = 1, std430) coherent buffer _12_14
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
; Version: 1.3
; Generator: Unknown(30017); 21022
; Bound: 66
; Schema: 0
OpCapability Shader
OpCapability FragmentShaderPixelInterlockEXT
OpExtension "SPV_EXT_fragment_shader_interlock"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %3 "main" %22
OpExecutionMode %3 OriginUpperLeft
OpExecutionMode %3 EarlyFragmentTests
OpExecutionMode %3 PixelInterlockOrderedEXT
OpName %3 "main"
OpName %7 "SSBO"
OpName %12 "SSBO"
OpName %16 "SSBO"
OpName %22 "SV_Position"
OpName %46 "ByteAddressMask"
OpName %44 "index"
OpName %45 "stride"
OpDecorate %6 ArrayStride 4
OpMemberDecorate %7 0 Offset 0
OpDecorate %7 Block
OpDecorate %9 DescriptorSet 0
OpDecorate %9 Binding 0
OpDecorate %9 Coherent
OpDecorate %11 ArrayStride 8
OpMemberDecorate %12 0 Offset 0
OpDecorate %12 Block
OpDecorate %14 DescriptorSet 0
OpDecorate %14 Binding 1
OpDecorate %14 Coherent
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
%34 = OpTypePointer StorageBuffer %10
%43 = OpTypeFunction %5 %5 %5
%49 = OpConstant %5 4294967295
%53 = OpConstant %5 4
%54 = OpTypePointer StorageBuffer %5
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
%56 = OpFunctionCall %5 %46 %27 %53
%57 = OpAccessChain %54 %9 %25 %56
%58 = OpLoad %5 %57
%59 = OpIAdd %5 %58 %29
%60 = OpFunctionCall %5 %46 %27 %53
%61 = OpAccessChain %54 %9 %25 %60
OpStore %61 %59
OpBranch %64
%64 = OpLabel
%35 = OpAccessChain %34 %14 %25 %27
%36 = OpLoad %10 %35
%37 = OpCompositeExtract %5 %36 0
%38 = OpCompositeExtract %5 %36 1
%39 = OpIAdd %5 %37 %29
%40 = OpIAdd %5 %38 %33
%41 = OpCompositeConstruct %10 %39 %40
%42 = OpAccessChain %34 %14 %25 %27
OpStore %42 %41
OpEndInvocationInterlockEXT
%52 = OpFunctionCall %5 %46 %27 %53
%55 = OpAccessChain %54 %18 %25 %52
OpStore %55 %27
OpReturn
OpFunctionEnd
%46 = OpFunction %5 None %43
%44 = OpFunctionParameter %5
%45 = OpFunctionParameter %5
%47 = OpLabel
%48 = OpUDiv %5 %49 %45
%50 = OpBitwiseAnd %5 %44 %48
OpReturnValue %50
OpFunctionEnd
#endif
