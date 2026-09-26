#version 460
#extension GL_ARB_shader_stencil_export : require

layout(location = 0) out vec4 SV_Target;
layout(location = 1) out vec4 SV_Target_1;
layout(location = 2) out vec4 SV_Target_2;

void main()
{
    float _34 = 1.0 / gl_FragCoord.w;
    SV_Target.x = gl_FragCoord.x;
    SV_Target.y = gl_FragCoord.y;
    SV_Target.z = gl_FragCoord.z;
    SV_Target.w = _34;
    SV_Target_1.x = gl_FragCoord.x * 2.0;
    SV_Target_1.y = gl_FragCoord.y * 2.0;
    SV_Target_1.z = gl_FragCoord.z * 2.0;
    SV_Target_1.w = _34 * 2.0;
    SV_Target_2.x = gl_FragCoord.x * 3.0;
    SV_Target_2.y = gl_FragCoord.y * 3.0;
    SV_Target_2.z = gl_FragCoord.z * 3.0;
    SV_Target_2.w = _34 * 3.0;
    gl_FragDepth = gl_FragCoord.z;
    gl_SampleMask[0u] = int(uint(gl_FragCoord.x));
    gl_FragStencilRefARB = int(uint(gl_FragCoord.y));
}


#if 0
// SPIR-V disassembly
; SPIR-V
; Version: 1.3
; Generator: Unknown(30017); 21022
; Bound: 63
; Schema: 0
OpCapability Shader
OpCapability StencilExportEXT
OpExtension "SPV_EXT_shader_stencil_export"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %3 "main" %8 %10 %11 %12 %14 %19 %21
OpExecutionMode %3 OriginUpperLeft
OpExecutionMode %3 DepthReplacing
OpExecutionMode %3 StencilRefReplacingEXT
OpName %3 "main"
OpName %8 "SV_Position"
OpName %10 "SV_Target"
OpName %11 "SV_Target_1"
OpName %12 "SV_Target_2"
OpName %14 "SV_Depth"
OpName %19 "SV_Coverage"
OpName %21 "SV_StencilRef"
OpDecorate %8 BuiltIn FragCoord
OpDecorate %8 Centroid
OpDecorate %10 Location 0
OpDecorate %11 Location 1
OpDecorate %12 Location 2
OpDecorate %14 BuiltIn FragDepth
OpDecorate %19 BuiltIn SampleMask
OpDecorate %21 BuiltIn FragStencilRefEXT
%1 = OpTypeVoid
%2 = OpTypeFunction %1
%5 = OpTypeFloat 32
%6 = OpTypeVector %5 4
%7 = OpTypePointer Input %6
%8 = OpVariable %7 Input
%9 = OpTypePointer Output %6
%10 = OpVariable %9 Output
%11 = OpVariable %9 Output
%12 = OpVariable %9 Output
%13 = OpTypePointer Output %5
%14 = OpVariable %13 Output
%15 = OpTypeInt 32 0
%16 = OpConstant %15 1
%17 = OpTypeArray %15 %16
%18 = OpTypePointer Output %17
%19 = OpVariable %18 Output
%20 = OpTypePointer Output %15
%21 = OpVariable %20 Output
%22 = OpTypePointer Input %5
%24 = OpConstant %15 0
%29 = OpConstant %15 2
%32 = OpConstant %15 3
%35 = OpConstant %5 1
%37 = OpConstant %5 2
%42 = OpConstant %5 3
%3 = OpFunction %1 None %2
%4 = OpLabel
OpBranch %61
%61 = OpLabel
%23 = OpAccessChain %22 %8 %24
%25 = OpLoad %5 %23
%26 = OpAccessChain %22 %8 %16
%27 = OpLoad %5 %26
%28 = OpAccessChain %22 %8 %29
%30 = OpLoad %5 %28
%31 = OpAccessChain %22 %8 %32
%33 = OpLoad %5 %31
%34 = OpFDiv %5 %35 %33
%36 = OpFMul %5 %25 %37
%38 = OpFMul %5 %27 %37
%39 = OpFMul %5 %30 %37
%40 = OpFMul %5 %34 %37
%41 = OpFMul %5 %25 %42
%43 = OpFMul %5 %27 %42
%44 = OpFMul %5 %30 %42
%45 = OpFMul %5 %34 %42
%46 = OpConvertFToU %15 %25
%47 = OpConvertFToU %15 %27
%48 = OpAccessChain %13 %10 %24
OpStore %48 %25
%49 = OpAccessChain %13 %10 %16
OpStore %49 %27
%50 = OpAccessChain %13 %10 %29
OpStore %50 %30
%51 = OpAccessChain %13 %10 %32
OpStore %51 %34
%52 = OpAccessChain %13 %11 %24
OpStore %52 %36
%53 = OpAccessChain %13 %11 %16
OpStore %53 %38
%54 = OpAccessChain %13 %11 %29
OpStore %54 %39
%55 = OpAccessChain %13 %11 %32
OpStore %55 %40
%56 = OpAccessChain %13 %12 %24
OpStore %56 %41
%57 = OpAccessChain %13 %12 %16
OpStore %57 %43
%58 = OpAccessChain %13 %12 %29
OpStore %58 %44
%59 = OpAccessChain %13 %12 %32
OpStore %59 %45
OpStore %14 %30
%60 = OpAccessChain %20 %19 %24
OpStore %60 %46
OpStore %21 %47
OpReturn
OpFunctionEnd
#endif
