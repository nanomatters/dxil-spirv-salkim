; SPIR-V
; Version: 1.6
; Generator: Unknown(30017); 21022
; Bound: 55
; Schema: 0
OpCapability Shader
OpCapability VulkanMemoryModel
OpCapability FragmentShaderPixelInterlockEXT
OpCapability RawAccessChainsNV
OpExtension "SPV_EXT_fragment_shader_interlock"
OpExtension "SPV_NV_raw_access_chains"
OpMemoryModel Logical Vulkan
OpEntryPoint Fragment %3 "main" %9 %13 %17 %21
OpExecutionMode %3 OriginUpperLeft
OpExecutionMode %3 EarlyFragmentTests
OpExecutionMode %3 PixelInterlockOrderedEXT
OpName %3 "main"
OpName %7 "SSBO"
OpName %11 "SSBO"
OpName %15 "SSBO"
OpName %21 "SV_Position"
OpDecorate %6 ArrayStride 4
OpMemberDecorate %7 0 Offset 0
OpDecorate %7 Block
OpDecorate %9 DescriptorSet 0
OpDecorate %9 Binding 0
OpDecorate %10 ArrayStride 4
OpMemberDecorate %11 0 Offset 0
OpDecorate %11 Block
OpDecorate %13 DescriptorSet 0
OpDecorate %13 Binding 1
OpDecorate %14 ArrayStride 4
OpMemberDecorate %15 0 Offset 0
OpDecorate %15 Block
OpDecorate %17 DescriptorSet 0
OpDecorate %17 Binding 2
OpDecorate %17 NonReadable
OpDecorate %21 BuiltIn FragCoord
%1 = OpTypeVoid
%2 = OpTypeFunction %1
%5 = OpTypeInt 32 0
%6 = OpTypeRuntimeArray %5
%7 = OpTypeStruct %6
%8 = OpTypePointer StorageBuffer %7
%9 = OpVariable %8 StorageBuffer
%10 = OpTypeRuntimeArray %5
%11 = OpTypeStruct %10
%12 = OpTypePointer StorageBuffer %11
%13 = OpVariable %12 StorageBuffer
%14 = OpTypeRuntimeArray %5
%15 = OpTypeStruct %14
%16 = OpTypePointer StorageBuffer %15
%17 = OpVariable %16 StorageBuffer
%18 = OpTypeFloat 32
%19 = OpTypeVector %18 4
%20 = OpTypePointer Input %19
%21 = OpVariable %20 Input
%22 = OpTypePointer Input %18
%24 = OpConstant %5 0
%28 = OpConstant %5 1
%29 = OpTypeBool
%32 = OpConstant %5 2
%33 = OpTypePointer StorageBuffer %5
%36 = OpConstant %5 5
%39 = OpTypeVector %5 2
%40 = OpTypePointer StorageBuffer %39
%42 = OpConstant %5 8
%3 = OpFunction %1 None %2
%4 = OpLabel
OpBranch %51
%51 = OpLabel
%23 = OpAccessChain %22 %21 %24
%25 = OpLoad %18 %23
%26 = OpConvertFToU %5 %25
%27 = OpBitwiseAnd %5 %26 %28
%30 = OpIEqual %29 %27 %24
%31 = OpShiftLeftLogical %5 %26 %32
OpBeginInvocationInterlockEXT
OpSelectionMerge %53 None
OpBranchConditional %30 %53 %52
%52 = OpLabel
%34 = OpRawAccessChainNV %33 %9 %24 %24 %31 RobustnessPerComponentNV
%35 = OpLoad %5 %34 Aligned|MakePointerVisible|NonPrivatePointer 4 %36
%37 = OpIAdd %5 %35 %28
%38 = OpRawAccessChainNV %33 %9 %24 %24 %31 RobustnessPerComponentNV
OpStore %38 %37 Aligned|MakePointerAvailable|NonPrivatePointer 4 %36
OpBranch %53
%53 = OpLabel
OpBranch %54
%54 = OpLabel
%41 = OpRawAccessChainNV %40 %13 %42 %26 %24 RobustnessPerElementNV
%43 = OpLoad %39 %41 Aligned|MakePointerVisible|NonPrivatePointer 8 %36
%44 = OpCompositeExtract %5 %43 0
%45 = OpCompositeExtract %5 %43 1
%46 = OpIAdd %5 %44 %28
%47 = OpIAdd %5 %45 %32
%48 = OpRawAccessChainNV %40 %13 %42 %26 %24 RobustnessPerElementNV
%49 = OpCompositeConstruct %39 %46 %47
OpStore %48 %49 Aligned|MakePointerAvailable|NonPrivatePointer 8 %36
OpEndInvocationInterlockEXT
%50 = OpRawAccessChainNV %33 %17 %24 %24 %31 RobustnessPerComponentNV
OpStore %50 %26 Aligned|NonPrivatePointer 4
OpReturn
OpFunctionEnd

