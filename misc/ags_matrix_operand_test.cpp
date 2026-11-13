/* Copyright (c) 2026 Erhan Bilgili
 * SPDX-License-Identifier: MIT
 */

#include "opcodes/converter_impl.hpp"
#include "opcodes/dxil/dxil_ags.hpp"
#include "context.hpp"
#include "node.hpp"
#include "node_pool.hpp"
#include "logging.hpp"
#include <cstdio>
#include <cstdlib>

using namespace dxil_spv;

static void check_at(bool result, unsigned line)
{
	if (!result)
	{
		std::fprintf(stderr, "AGS matrix operand check failed at line %u.\n", line);
		std::exit(1);
	}
}
#define check(x) check_at((x), __LINE__)

static void test_scalar_escapes(unsigned component)
{
	llvm::LLVMContext context;
	llvm::Module llvm_module(context);
	LLVMBCParser parser;
	SPIRVModule module;
	Converter::Impl impl(parser, nullptr, module);
	impl.options.eliminate_dead_code = false;
	CFGNodePool pool;
	auto &block = *pool.create_node();
	auto *uint_type = llvm::Type::getInt32Ty(context);
	llvm::Argument fragment(uint_type, 0, false), ordinary(uint_type, 1, false);
	llvm::ConstantInt zero(uint_type, 0), opcode(uint_type, unsigned(DXIL::Op::RawBufferStore));
	auto &builder = impl.builder();
	auto matrix_type = builder.makeCooperativeMatrixType(builder.makeFloatType(32),
	    builder.makeUintConstant(16), builder.makeUintConstant(16),
	    builder.makeUintConstant(spv::CooperativeMatrixUseMatrixAccumulatorKHR));
	impl.ags.coopmat_component_mapping[&fragment] = { matrix_type, component };
	impl.rewrite_value(&fragment, builder.createUndefinedConstant(matrix_type));
	impl.rewrite_value(&ordinary, builder.makeUintConstant(1));

	llvm::BinaryOperator xor_op(&fragment, &ordinary, llvm::Instruction::Xor);
	llvm::CastInst cast(llvm::Type::getFloatTy(context), &fragment, llvm::Instruction::BitCast);
	llvm::ICmpInst compare(llvm::Instruction::ICMP_EQ, &fragment, &ordinary);
	llvm::Argument condition(llvm::Type::getInt1Ty(context), 2, false);
	llvm::SelectInst select(&fragment, &ordinary, &condition);
	llvm::FunctionType type(context, llvm::Type::getVoidTy(context),
	    { uint_type, uint_type, uint_type, uint_type, uint_type, uint_type, uint_type, uint_type, uint_type, uint_type });
	llvm::Function function(&type, 0, llvm_module);
	llvm_module.add_value_name(0, "dx.op.rawBufferStore.i32");
	llvm::CallInst store(&type, &function,
	    { &opcode, &zero, &zero, &zero, &fragment, &zero, &zero, &zero, &zero, &zero });

	for (auto *instruction : Vector<llvm::Instruction *>{ &xor_op, &cast, &compare, &select, &store })
	{
		check(!impl.emit_instruction(&block, *instruction));
		check(block.ir.operations.empty());
	}

	impl.ags.coopmat_component_mapping.clear();
	impl.rewrite_value(&fragment, builder.makeUintConstant(2));
	check(impl.emit_instruction(&block, xor_op));
	check(block.ir.operations.size() == 1);
	check(block.ir.operations.front()->op == spv::OpBitwiseXor);
}

static void capture_error(void *userdata, LogLevel level, const char *message)
{
	if (level == LogLevel::Error)
		*static_cast<String *>(userdata) += message;
}

static void test_terminators(bool eliminate_dead_code)
{
	llvm::LLVMContext context;
	LLVMBCParser parser;
	SPIRVModule module;
	Converter::Impl impl(parser, nullptr, module);
	impl.options.eliminate_dead_code = eliminate_dead_code;
	CFGNodePool pool;
	auto *block = pool.create_node();
	auto *type = llvm::Type::getInt32Ty(context);
	llvm::Argument fragment(type, 0, false), ordinary(type, 1, false);
	llvm::Argument condition(llvm::Type::getInt1Ty(context), 2, false);
	llvm::Argument raw_condition(llvm::Type::getInt1Ty(context), 3, false);
	llvm::BasicBlock target(context);
	impl.ags.coopmat_component_mapping[&fragment] = { 1, 0 };
	impl.ags.coopmat_component_mapping[&raw_condition] = { 1, 0 };

	llvm::ReturnInst raw_return(&fragment), scalar_return(&ordinary), void_return(nullptr);
	llvm::SwitchInst raw_switch(&fragment, &target, 0), scalar_switch(&ordinary, &target, 0);
	llvm::BranchInst conditional(&target, &target, &condition), unconditional(&target);
	llvm::BranchInst raw_branch(&target, &target, &raw_condition);
	llvm::UnreachableInst unreachable;
	String errors;
	set_thread_log_callback(capture_error, &errors);
	check(!impl.emit_instruction(block, raw_return));
	check(errors.find("ret operand 0") != String::npos);
	errors.clear();
	check(!impl.emit_instruction(block, raw_switch));
	check(errors.find("switch operand 0") != String::npos);
	errors.clear();
	check(!impl.emit_instruction(block, raw_branch));
	check(errors.find("br operand 0") != String::npos);
	errors.clear();

	for (auto *instruction : Vector<llvm::Instruction *>{ &scalar_return, &void_return, &scalar_switch,
	                                                       &conditional, &unconditional, &unreachable })
		check(impl.emit_instruction(block, *instruction));
	check(errors.empty());
	check(block->ir.operations.empty());

	// Unused scalar operations remain removable, but terminators must always be checked.
	llvm::BinaryOperator dead_xor(&ordinary, &fragment, llvm::Instruction::Xor);
	check(impl.emit_instruction(block, dead_xor) == eliminate_dead_code);
	if (!eliminate_dead_code)
		check(errors.find("operand 1") != String::npos);
	errors.clear();
	impl.llvm_used_ssa_values.insert(&dead_xor);
	check(!impl.emit_instruction(block, dead_xor));
	check(errors.find("operand 1") != String::npos);
	check(block->ir.operations.empty());
	set_thread_log_callback(nullptr, nullptr);
}

static void test_matrix_intrinsics()
{
	llvm::LLVMContext context;
	llvm::Module llvm_module(context);
	LLVMBCParser parser;
	SPIRVModule module;
	Converter::Impl impl(parser, nullptr, module);
	auto *type = llvm::Type::getInt32Ty(context);
	llvm::Argument fragment(type, 0, false), handle(type, 1, false);
	llvm::ConstantInt opcode(type, unsigned(DXIL::Op::AtomicCompareExchange)), zero(type, 0);
	llvm::FunctionType function_type(context, type, { type, type, type, type, type, type, type });
	llvm::Function function(&function_type, 0, llvm_module);
	llvm_module.add_value_name(0, "dx.op.atomicCompareExchange.i32");
	impl.ags.coopmat_component_mapping[&fragment] = { 1, 0 };
	impl.llvm_value_to_uav_resource_index_map[&handle] = impl.ags.uav_magic_resource_type_index;

	for (unsigned op : {
	    AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixUavStore,
	    AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixLdsStore,
	    AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixCopy,
	    AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixMulAcc,
	    AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixElementFill,
	    AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixElementExtract,
	    AmdExtD3DShaderIntrinsicsOpcode_MatrixElementWiseArithmetic,
	    AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixFill,
	    AmdExtD3DShaderIntrinsicsOpcode_Readlane })
	{
		for (unsigned phase = 0; phase < 3; phase++)
		{
			uint32_t magic = (AmdExtD3DShaderIntrinsics_MagicCode << AmdExtD3DShaderIntrinsics_MagicCodeShift) |
			                 (phase << AmdExtD3DShaderIntrinsics_OpcodePhaseShift) | op;
			llvm::ConstantInt code(type, magic);
			for (unsigned operand : { 5u, 6u })
			{
				Vector<llvm::Value *> args = { &opcode, &handle, &code, &zero, &zero, &zero, &zero };
				args[operand] = &fragment;
				llvm::CallInst call(&function_type, &function, args);
				bool store = op == AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixUavStore ||
				             op == AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixLdsStore;
				bool scalar = op == AmdExtD3DShaderIntrinsicsOpcode_WaveMatrixFill ||
				              op == AmdExtD3DShaderIntrinsicsOpcode_Readlane;
				check(validate_ags_wmma_operands(impl, call) == (!scalar && phase == unsigned(store)));
			}
		}
	}

	// Scalar extraction results have no opaque matrix mapping.
	llvm::Argument extracted(type, 2, false);
	llvm::BinaryOperator add(&extracted, &zero, llvm::Instruction::Add);
	check(validate_ags_wmma_operands(impl, add));
}

int main()
{
	for (unsigned component = 0; component < 8; component++)
		test_scalar_escapes(component);
	test_terminators(false);
	test_terminators(true);
	test_matrix_intrinsics();
	std::puts("AGS matrix operand tests passed.");
}
