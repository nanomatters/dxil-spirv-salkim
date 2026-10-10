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

static void test_uniform_read(bool uniform, bool constant_lane)
{
	llvm::LLVMContext context;
	LLVMBCParser parser;
	SPIRVModule module;
	Converter::Impl impl(parser, nullptr, module);
	Vector<Operation *> block;
	impl.current_block = &block;
	impl.execution_model = spv::ExecutionModelGLCompute;
	auto *type = llvm::Type::getInt32Ty(context);
	llvm::ConstantInt opcode(type, unsigned(DXIL::Op::WaveReadLaneAt)), constant(type, 17), zero(type, 0);
	llvm::Argument dynamic(type, 0), lane(type, 1);
	llvm::Value *value = uniform ? static_cast<llvm::Value *>(&constant) : &dynamic;
	llvm::FunctionType function(context, type, { type, type, type });
	llvm::CallInst call(&function, nullptr, { &opcode, value, constant_lane ? static_cast<llvm::Value *>(&zero) : &lane });
	check(emit_wave_read_lane_at_instruction(impl, &call));
	if (uniform)
	{
		check(block.empty());
		check(impl.get_id_for_value(&call) == impl.get_id_for_value(value));
		check(!impl.shader_analysis.require_subgroup_shuffles);
	}
	else
	{
		check(block.size() == 1);
		check(block[0]->op == (constant_lane ? spv::OpGroupNonUniformBroadcast : spv::OpGroupNonUniformShuffle));
		check(impl.shader_analysis.require_subgroup_shuffles);
	}
}

static void test_uniform_analysis_budget()
{
	llvm::LLVMContext context;
	llvm::Module llvm_module(context);
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
	llvm::ConstantInt opcode(type, unsigned(DXIL::Op::WaveReadLaneFirst));
	llvm::FunctionType read_type(context, type, { type, type });
	llvm::Function read_function(&read_type, 1, llvm_module);
	llvm_module.add_value_name(1, "dx.op.waveReadLaneFirst.i32");
	value = &constant;
	for (unsigned i = 0; i < 128; i++)
		value = context.construct<llvm::CallInst>(&read_type, &read_function,
		                                        Vector<llvm::Value *>{ &opcode, value });
	check(!value_is_statically_wave_uniform(impl, value));
	// The budget is per query; previous exhaustion must not affect later calls.
	check(value_is_statically_wave_uniform(impl, &constant));
}

static void test_nested_read(unsigned value_kind, bool inner_first, bool outer_first, bool constant_lane)
{
	llvm::LLVMContext context;
	llvm::Module llvm_module(context);
	LLVMBCParser parser;
	SPIRVModule module;
	Converter::Impl impl(parser, nullptr, module);
	Vector<Operation *> block;
	impl.current_block = &block;
	impl.execution_model = spv::ExecutionModelGLCompute;
	auto *type = llvm::Type::getInt32Ty(context);
	llvm::ConstantInt constant(type, 17), zero(type, 0);
	llvm::Argument varying(type, 0), lane(type, 1);
	llvm::ConstantInt reduce_opcode(type, unsigned(DXIL::Op::WaveActiveOp));
	llvm::FunctionType reduce_type(context, type, { type, type, type, type });
	llvm::Function reduce_function(&reduce_type, 1, llvm_module);
	llvm_module.add_value_name(1, "dx.op.waveActiveOp.i32");
	llvm::CallInst reduction(&reduce_type, &reduce_function, { &reduce_opcode, &constant, &zero, &zero });
	llvm::Value *value = value_kind == 0 ? static_cast<llvm::Value *>(&constant) :
	                     value_kind == 1 ? static_cast<llvm::Value *>(&varying) : &reduction;
	llvm::Value *lane_value = constant_lane ? static_cast<llvm::Value *>(&zero) : &lane;
	llvm::ConstantInt first_opcode(type, unsigned(DXIL::Op::WaveReadLaneFirst));
	llvm::ConstantInt at_opcode(type, unsigned(DXIL::Op::WaveReadLaneAt));
	llvm::FunctionType first_type(context, type, { type, type });
	llvm::FunctionType at_type(context, type, { type, type, type });
	llvm::Function first_function(&first_type, 2, llvm_module), at_function(&at_type, 3, llvm_module);
	llvm_module.add_value_name(2, "dx.op.waveReadLaneFirst.i32");
	llvm_module.add_value_name(3, "dx.op.waveReadLaneAt.i32");
	llvm::CallInst inner(inner_first ? &first_type : &at_type, inner_first ? &first_function : &at_function,
	                    inner_first ? Vector<llvm::Value *>{ &first_opcode, value } :
	                                  Vector<llvm::Value *>{ &at_opcode, value, lane_value });
	llvm::CallInst outer(outer_first ? &first_type : &at_type, outer_first ? &first_function : &at_function,
	                    outer_first ? Vector<llvm::Value *>{ &first_opcode, &inner } :
	                                  Vector<llvm::Value *>{ &at_opcode, &inner, lane_value });
	// A nested read is context-free uniform only when its original input is.
	// Varying inputs and reductions must remain conservative across loop exits.
	check(value_is_statically_wave_uniform(impl, &inner) == (value_kind == 0));
	check(inner_first ? emit_wave_read_lane_first_instruction(impl, &inner) :
	                    emit_wave_read_lane_at_instruction(impl, &inner));
	check(outer_first ? emit_wave_read_lane_first_instruction(impl, &outer) :
	                    emit_wave_read_lane_at_instruction(impl, &outer));
	if (value_kind == 0)
	{
		check(block.empty());
		check(impl.get_id_for_value(&outer) == impl.get_id_for_value(value));
		check(!impl.shader_analysis.require_subgroup_shuffles);
	}
	else
	{
		check(block.size() == 2);
		check(block[0]->op == (inner_first ? spv::OpGroupNonUniformBroadcastFirst :
		                      constant_lane ? spv::OpGroupNonUniformBroadcast : spv::OpGroupNonUniformShuffle));
		check(block[1]->op == (outer_first ? spv::OpGroupNonUniformBroadcastFirst :
		                      constant_lane ? spv::OpGroupNonUniformBroadcast : spv::OpGroupNonUniformShuffle));
	}
}

static void test_active_wave_read(bool derived, bool first, bool constant_lane)
{
	llvm::LLVMContext context;
	llvm::Module llvm_module(context);
	LLVMBCParser parser;
	SPIRVModule module;
	Converter::Impl impl(parser, nullptr, module);
	Vector<Operation *> block;
	impl.current_block = &block;
	impl.execution_model = spv::ExecutionModelGLCompute;
	auto *type = llvm::Type::getInt32Ty(context);
	llvm::ConstantInt opcode(type, unsigned(DXIL::Op::WaveActiveOp));
	llvm::ConstantInt one(type, 1), zero(type, 0);
	llvm::FunctionType reduce_type(context, type, { type, type, type, type });
	llvm::Function reduce_function(&reduce_type, 1, llvm_module);
	llvm_module.add_value_name(1, "dx.op.waveActiveOp.i32");
	llvm::CallInst reduction(&reduce_type, &reduce_function, { &opcode, &one, &zero, &zero });
	llvm::BinaryOperator arithmetic(&reduction, &one, llvm::Instruction::Add);
	llvm::Value *value = derived ? static_cast<llvm::Value *>(&arithmetic) : &reduction;
	// No consumer/convergence information is available to this proof. In particular,
	// this result may come from different iterations of a divergent loop.
	check(!value_is_statically_wave_uniform(impl, value));
	llvm::ConstantInt read_opcode(type, unsigned(first ? DXIL::Op::WaveReadLaneFirst : DXIL::Op::WaveReadLaneAt));
	llvm::Argument lane(type, 0);
	llvm::FunctionType read_type(context, type, { type, type, type });
	llvm::CallInst read(&read_type, nullptr,
	                    { &read_opcode, value, constant_lane ? static_cast<llvm::Value *>(&zero) : &lane });
	check(first ? emit_wave_read_lane_first_instruction(impl, &read) : emit_wave_read_lane_at_instruction(impl, &read));
	check(block.size() == 1);
	check(block[0]->op == (first ? spv::OpGroupNonUniformBroadcastFirst :
	                      constant_lane ? spv::OpGroupNonUniformBroadcast : spv::OpGroupNonUniformShuffle));
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
	for (bool uniform : { false, true })
	for (bool constant_lane : { false, true })
	{
		test_uniform_read(uniform, constant_lane);
		cases++;
	}
	for (bool derived : { false, true })
	for (bool first : { false, true })
	for (bool constant_lane : { false, true })
	{
		test_active_wave_read(derived, first, constant_lane);
		cases++;
	}
	for (unsigned value_kind = 0; value_kind < 3; value_kind++)
	for (bool inner_first : { false, true })
	for (bool outer_first : { false, true })
	for (bool constant_lane : { false, true })
	{
		test_nested_read(value_kind, inner_first, outer_first, constant_lane);
		cases++;
	}
	end_thread_allocator_context();
	std::printf("%u wave codegen cases passed.\n", cases);
}
