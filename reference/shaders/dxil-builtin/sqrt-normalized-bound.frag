#version 460

layout(location = 0) in vec2 VALUE;
layout(location = 0) out vec2 SV_Target;

void main()
{
    float _24 = inversesqrt(dot(vec2(VALUE.x, VALUE.y), vec2(VALUE.x, VALUE.y))) * VALUE.x;
    SV_Target.x = sqrt(max(1.0 - _24, 0.0));
    SV_Target.y = sqrt(max(2.0 - (_24 * 2.0), 0.0));
}


#if 0
// SPIR-V disassembly
; SPIR-V
; Version: 1.3
; Generator: Unknown(30017); 21022
; Bound: 41
; Schema: 0
OpCapability Shader
%22 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %3 "main" %8 %10
OpExecutionMode %3 OriginUpperLeft
OpName %3 "main"
OpName %8 "VALUE"
OpName %10 "SV_Target"
OpDecorate %8 Location 0
OpDecorate %10 Location 0
%1 = OpTypeVoid
%2 = OpTypeFunction %1
%5 = OpTypeFloat 32
%6 = OpTypeVector %5 2
%7 = OpTypePointer Input %6
%8 = OpVariable %7 Input
%9 = OpTypePointer Output %6
%10 = OpVariable %9 Output
%11 = OpTypePointer Input %5
%13 = OpTypeInt 32 0
%14 = OpConstant %13 0
%17 = OpConstant %13 1
%26 = OpConstant %5 1
%29 = OpConstantNull %5
%31 = OpConstant %5 2
%35 = OpConstantNull %5
%36 = OpTypePointer Output %5
%3 = OpFunction %1 None %2
%4 = OpLabel
OpBranch %39
%39 = OpLabel
%12 = OpAccessChain %11 %8 %14
%15 = OpLoad %5 %12
%16 = OpAccessChain %11 %8 %17
%18 = OpLoad %5 %16
%20 = OpCompositeConstruct %6 %15 %18
%21 = OpCompositeConstruct %6 %15 %18
%19 = OpDot %5 %20 %21
%23 = OpExtInst %5 %22 InverseSqrt %19
%24 = OpFMul %5 %23 %15
%25 = OpFSub %5 %26 %24
%28 = OpExtInst %5 %22 FMax %25 %29
%27 = OpExtInst %5 %22 Sqrt %28
%30 = OpFMul %5 %24 %31
%32 = OpFSub %5 %31 %30
%34 = OpExtInst %5 %22 FMax %32 %35
%33 = OpExtInst %5 %22 Sqrt %34
%37 = OpAccessChain %36 %10 %14
OpStore %37 %27
%38 = OpAccessChain %36 %10 %17
OpStore %38 %33
OpReturn
OpFunctionEnd
#endif
