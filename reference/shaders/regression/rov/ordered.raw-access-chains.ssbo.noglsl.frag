; SPIR-V
; Version: 1.3
; Generator: Unknown(30017); 21022
; Bound: 54
; Schema: 0
OpCapability Shader
OpCapability FragmentShaderPixelInterlockEXT
OpCapability RawAccessChainsNV
OpExtension "SPV_EXT_fragment_shader_interlock"
OpExtension "SPV_NV_raw_access_chains"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %3 "main" %21
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
OpDecorate %9 Coherent
OpDecorate %10 ArrayStride 4
OpMemberDecorate %11 0 Offset 0
OpDecorate %11 Block
OpDecorate %13 DescriptorSet 0
OpDecorate %13 Binding 1
OpDecorate %13 Coherent
OpDecorate %14 ArrayStride 4
OpMemberDecorate %15 0 Offset 0
OpDecorate %15 Block
OpDecorate %17 DescriptorSet 0
OpDecorate %17 Binding 2
OpDecorate %17 NonReadable
OpDecorate %21 BuiltIn FragCoord
OpDecorate %35 Coherent
OpDecorate %42 Coherent
OpDecorate %46 Coherent
OpDecorate %49 Coherent
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
%33 = OpTypeVector %5 2
%34 = OpTypePointer StorageBuffer %33
%36 = OpConstant %5 8
%44 = OpTypePointer StorageBuffer %5
%3 = OpFunction %1 None %2
%4 = OpLabel
OpBranch %50
%50 = OpLabel
%23 = OpAccessChain %22 %21 %24
%25 = OpLoad %18 %23
%26 = OpConvertFToU %5 %25
%27 = OpBitwiseAnd %5 %26 %28
%30 = OpIEqual %29 %27 %24
%31 = OpShiftLeftLogical %5 %26 %32
OpBeginInvocationInterlockEXT
OpSelectionMerge %52 None
OpBranchConditional %30 %52 %51
%51 = OpLabel
%46 = OpRawAccessChainNV %44 %9 %24 %24 %31 RobustnessPerComponentNV
%47 = OpLoad %5 %46 Aligned 4
%48 = OpIAdd %5 %47 %28
%49 = OpRawAccessChainNV %44 %9 %24 %24 %31 RobustnessPerComponentNV
OpStore %49 %48 Aligned 4
OpBranch %52
%52 = OpLabel
%35 = OpRawAccessChainNV %34 %13 %36 %26 %24 RobustnessPerElementNV
%37 = OpLoad %33 %35 Aligned 8
%38 = OpCompositeExtract %5 %37 0
%39 = OpCompositeExtract %5 %37 1
%40 = OpIAdd %5 %38 %28
%41 = OpIAdd %5 %39 %32
%42 = OpRawAccessChainNV %34 %13 %36 %26 %24 RobustnessPerElementNV
%43 = OpCompositeConstruct %33 %40 %41
OpStore %42 %43 Aligned 8
OpEndInvocationInterlockEXT
%45 = OpRawAccessChainNV %44 %17 %24 %24 %31 RobustnessPerComponentNV
OpStore %45 %26 Aligned 4
OpReturn
OpFunctionEnd

