/* Copyright (c) 2026 Erhan Bilgili
 * SPDX-License-Identifier: MIT
 */

#include "opcodes/converter_impl.hpp"
#include "opcodes/dxil/dxil_waveops.hpp"
#include "context.hpp"
#include <cstdio>
#include <cstdlib>
#include <limits>

using namespace dxil_spv;

static void check_at(bool result, unsigned line)
{
	if (!result)
	{
		std::fprintf(stderr, "Wave codegen check failed at line %u.\n", line);
		std::exit(1);
	}
}
#define check(x) check_at((x), __LINE__)

static void test_neutral_value(unsigned width, unsigned lanes, bool native, bool prefix, unsigned kind)
{
	llvm::LLVMContext context;
	LLVMBCParser parser;
	SPIRVModule module;
	Converter::Impl impl(parser, nullptr, module);
	Vector<Operation *> block;
	impl.current_block = &block;
	impl.execution_model = spv::ExecutionModelFragment;
	impl.options.min_precision_prefer_native_16bit = native;
	auto *scalar = width == 16 ? llvm::Type::getHalfTy(context) :
	               width == 32 ? llvm::Type::getFloatTy(context) : llvm::Type::getDoubleTy(context);
	llvm::Type *type = lanes == 1 ? scalar : llvm::VectorType::get(lanes, scalar);
	auto *uint_type = llvm::Type::getInt32Ty(context);
	llvm::Argument value(type, 0);
	llvm::ConstantInt opcode(uint_type, unsigned(prefix ? DXIL::Op::WavePrefixOp : DXIL::Op::WaveActiveOp));
	llvm::ConstantInt operation(uint_type, kind), sign(uint_type, 0);
	llvm::FunctionType function(context, type, { uint_type, type, uint_type, uint_type });
	llvm::CallInst call(&function, nullptr, { &opcode, &value, &operation, &sign });
	check(prefix ? emit_wave_prefix_op_instruction(impl, &call) : emit_wave_active_op_instruction(impl, &call));
	check(block.size() == 3);
	auto *select = block[1];
	check(select->op == spv::OpSelect);
	check(select->type_id == impl.get_type_id(type));
	auto &builder = impl.builder();
	spv::Id neutral;
	if (width == 16 && native)
	{
		const unsigned bits[] = { 0, 0x3c00, 0x7c00, 0xfc00 };
		neutral = builder.makeFloat16Constant(bits[kind]);
	}
	else
	{
		const double values[] = { 0.0, 1.0, std::numeric_limits<double>::infinity(),
		                          -std::numeric_limits<double>::infinity() };
		neutral = width == 64 ? builder.makeDoubleConstant(values[kind]) : builder.makeFloatConstant(float(values[kind]));
	}
	neutral = impl.build_splat_constant_vector(impl.get_type_id(scalar), neutral, lanes);
	check(select->argument(1) == neutral);
	check(builder.getTypeId(neutral) == select->type_id);
	check(block.back()->argument(2) == select->id);
}

static void test_uniform_analysis_budget()
{
	llvm::LLVMContext context;
	LLVMBCParser parser;
	SPIRVModule module;
	Converter::Impl impl(parser, nullptr, module);
	auto *type = llvm::Type::getInt32Ty(context);
	llvm::ConstantInt constant(type, 3);
	check(value_is_statically_wave_uniform(impl, &constant));
	llvm::Value *value = &constant;
	// No constant folding in this synthetic DAG. Without a shared budget this
	// visits the same nodes 2^128 times instead of returning conservatively.
	for (unsigned i = 0; i < 128; i++)
		value = context.construct<llvm::BinaryOperator>(value, value, llvm::Instruction::Mul);
	check(!value_is_statically_wave_uniform(impl, value));
	// The budget is per query; previous exhaustion must not affect later calls.
	check(value_is_statically_wave_uniform(impl, &constant));
}

int main()
{
	begin_thread_allocator_context();
	test_uniform_analysis_budget();
	unsigned cases = 0;
	for (unsigned width : { 16u, 32u, 64u })
	for (unsigned lanes : { 1u, 2u, 4u, 8u })
	for (bool native : { false, true })
	for (bool prefix : { false, true })
	for (unsigned kind = 0; kind < (prefix ? 2u : 4u); kind++)
	{
		test_neutral_value(width, lanes, native, prefix, kind);
		cases++;
	}
	end_thread_allocator_context();
	std::printf("%u wave codegen cases passed.\n", cases);
}
