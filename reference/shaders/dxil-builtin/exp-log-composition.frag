#version 460

layout(location = 0) in float X;
layout(location = 0) out vec4 SV_Target;

void main()
{
    SV_Target.x = exp2(exp2(X));
    SV_Target.y = log2(log2(X));
    SV_Target.z = X;
    SV_Target.w = X;
}


#if 0
// SPIR-V disassembly
; SPIR-V
; Version: 1.3
; Generator: Unknown(30017); 21022
; Bound: 29
; Schema: 0
OpCapability Shader
%12 = OpExtInstImport "GLSL.std.450"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %3 "main" %7 %10
OpExecutionMode %3 OriginUpperLeft
OpName %3 "main"
OpName %7 "X"
OpName %10 "SV_Target"
OpDecorate %7 Location 0
OpDecorate %10 Location 0
%1 = OpTypeVoid
%2 = OpTypeFunction %1
%5 = OpTypeFloat 32
%6 = OpTypePointer Input %5
%7 = OpVariable %6 Input
%8 = OpTypeVector %5 4
%9 = OpTypePointer Output %8
%10 = OpVariable %9 Output
%17 = OpTypePointer Output %5
%19 = OpTypeInt 32 0
%20 = OpConstant %19 0
%22 = OpConstant %19 1
%24 = OpConstant %19 2
%26 = OpConstant %19 3
%3 = OpFunction %1 None %2
%4 = OpLabel
OpBranch %27
%27 = OpLabel
%11 = OpLoad %5 %7
%13 = OpExtInst %5 %12 Exp2 %11
%14 = OpExtInst %5 %12 Exp2 %13
%15 = OpExtInst %5 %12 Log2 %11
%16 = OpExtInst %5 %12 Log2 %15
%18 = OpAccessChain %17 %10 %20
OpStore %18 %14
%21 = OpAccessChain %17 %10 %22
OpStore %21 %16
%23 = OpAccessChain %17 %10 %24
OpStore %23 %11
%25 = OpAccessChain %17 %10 %26
OpStore %25 %11
OpReturn
OpFunctionEnd
#endif
