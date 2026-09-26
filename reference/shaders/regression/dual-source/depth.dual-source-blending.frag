#version 460
#extension GL_ARB_shader_stencil_export : require

layout(location = 0, index = 0) out vec4 SV_Target;
layout(location = 0, index = 1) out vec4 SV_Target_1;

void main()
{
    float _33 = 1.0 / gl_FragCoord.w;
    SV_Target.x = gl_FragCoord.x;
    SV_Target.y = gl_FragCoord.y;
    SV_Target.z = gl_FragCoord.z;
    SV_Target.w = _33;
    SV_Target_1.x = gl_FragCoord.x * 2.0;
    SV_Target_1.y = gl_FragCoord.y * 2.0;
    SV_Target_1.z = gl_FragCoord.z * 2.0;
    SV_Target_1.w = _33 * 2.0;
    gl_FragDepth = gl_FragCoord.z;
    gl_SampleMask[0u] = int(uint(gl_FragCoord.x));
    gl_FragStencilRefARB = int(uint(gl_FragCoord.y));
}


#if 0
// SPIR-V disassembly
; SPIR-V
; Version: 1.3
; Generator: Unknown(30017); 21022
; Bound: 58
; Schema: 0
OpCapability Shader
OpCapability StencilExportEXT
OpExtension "SPV_EXT_shader_stencil_export"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %3 "main" %8 %10 %11 %13 %18 %20
OpExecutionMode %3 OriginUpperLeft
OpExecutionMode %3 DepthReplacing
OpExecutionMode %3 StencilRefReplacingEXT
OpName %3 "main"
OpName %8 "SV_Position"
OpName %10 "SV_Target"
OpName %11 "SV_Target_1"
OpName %13 "SV_Depth"
OpName %18 "SV_Coverage"
OpName %20 "SV_StencilRef"
OpDecorate %8 BuiltIn FragCoord
OpDecorate %8 Centroid
OpDecorate %10 Location 0
OpDecorate %10 Index 0
OpDecorate %11 Location 0
OpDecorate %11 Index 1
OpDecorate %13 BuiltIn FragDepth
OpDecorate %18 BuiltIn SampleMask
OpDecorate %20 BuiltIn FragStencilRefEXT
%1 = OpTypeVoid
%2 = OpTypeFunction %1
%5 = OpTypeFloat 32
%6 = OpTypeVector %5 4
%7 = OpTypePointer Input %6
%8 = OpVariable %7 Input
%9 = OpTypePointer Output %6
%10 = OpVariable %9 Output
%11 = OpVariable %9 Output
%12 = OpTypePointer Output %5
%13 = OpVariable %12 Output
%14 = OpTypeInt 32 0
%15 = OpConstant %14 1
%16 = OpTypeArray %14 %15
%17 = OpTypePointer Output %16
%18 = OpVariable %17 Output
%19 = OpTypePointer Output %14
%20 = OpVariable %19 Output
%21 = OpTypePointer Input %5
%23 = OpConstant %14 0
%28 = OpConstant %14 2
%31 = OpConstant %14 3
%34 = OpConstant %5 1
%36 = OpConstant %5 2
%41 = OpConstant %5 3
%3 = OpFunction %1 None %2
%4 = OpLabel
OpBranch %56
%56 = OpLabel
%22 = OpAccessChain %21 %8 %23
%24 = OpLoad %5 %22
%25 = OpAccessChain %21 %8 %15
%26 = OpLoad %5 %25
%27 = OpAccessChain %21 %8 %28
%29 = OpLoad %5 %27
%30 = OpAccessChain %21 %8 %31
%32 = OpLoad %5 %30
%33 = OpFDiv %5 %34 %32
%35 = OpFMul %5 %24 %36
%37 = OpFMul %5 %26 %36
%38 = OpFMul %5 %29 %36
%39 = OpFMul %5 %33 %36
%40 = OpFMul %5 %24 %41
%42 = OpFMul %5 %26 %41
%43 = OpFMul %5 %29 %41
%44 = OpFMul %5 %33 %41
%45 = OpConvertFToU %14 %24
%46 = OpConvertFToU %14 %26
%47 = OpAccessChain %12 %10 %23
OpStore %47 %24
%48 = OpAccessChain %12 %10 %15
OpStore %48 %26
%49 = OpAccessChain %12 %10 %28
OpStore %49 %29
%50 = OpAccessChain %12 %10 %31
OpStore %50 %33
%51 = OpAccessChain %12 %11 %23
OpStore %51 %35
%52 = OpAccessChain %12 %11 %15
OpStore %52 %37
%53 = OpAccessChain %12 %11 %28
OpStore %53 %38
%54 = OpAccessChain %12 %11 %31
OpStore %54 %39
OpStore %13 %29
%55 = OpAccessChain %19 %18 %23
OpStore %55 %45
OpStore %20 %46
OpReturn
OpFunctionEnd
#endif
