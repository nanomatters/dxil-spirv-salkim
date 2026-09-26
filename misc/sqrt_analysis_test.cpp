/* Copyright (c) 2026 Erhan Bilgili
 * SPDX-License-Identifier: MIT
 */

#include "opcodes/converter_impl.hpp"
#include "opcodes/dxil/dxil_arithmetic.hpp"
#include "context.hpp"
#include <cstdio>
#include <cstdlib>

using namespace dxil_spv;

static void check_at(bool result, unsigned line)
{
	if (!result)
	{
		std::fprintf(stderr, "Sqrt analysis check failed at line %u.\n", line);
		std::exit(1);
	}
}
#define check(x) check_at((x), __LINE__)

static void check_sqrt(Converter::Impl &impl, llvm::Value *value, bool clamp, const char *name)
{
	impl.current_block->clear();
	auto *type = value->getType();
	auto *uint_type = llvm::Type::getInt32Ty(type->getContext());
	llvm::ConstantInt opcode(uint_type, unsigned(DXIL::Op::Sqrt));
	llvm::FunctionType function(type->getContext(), type, { uint_type, type });
	llvm::CallInst call(&function, nullptr, { &opcode, value });
	check(emit_dxil_std450_unary_instruction(GLSLstd450Sqrt, impl, &call));
	auto &block = *impl.current_block;
	if (block.size() != (clamp ? 2u : 1u))
		std::fprintf(stderr, "%s: expected %u operations, got %zu.\n", name, clamp ? 2u : 1u, block.size());
	check(block.size() == (clamp ? 2u : 1u));
	check(block.back()->op == spv::OpExtInst);
	check(block.back()->argument(1) == GLSLstd450Sqrt);
	if (clamp)
	{
		check(block[0]->op == spv::OpExtInst);
		check(block[0]->argument(1) == GLSLstd450FMax);
		check(block[0]->argument(2) == impl.get_id_for_value(value));
		check(impl.builder().getOpCode(block[0]->argument(3)) == spv::OpConstantNull);
		check(impl.builder().getTypeId(block[0]->argument(3)) == block[0]->type_id);
		check(block[1]->argument(2) == block[0]->id);
	}
	else
		check(block.back()->argument(2) == impl.get_id_for_value(value));
}

static void test_analysis()
{
	llvm::LLVMContext context;
	llvm::Module llvm_module(context);
	LLVMBCParser parser;
	SPIRVModule module;
	Converter::Impl impl(parser, nullptr, module);
	Vector<Operation *> block;
	impl.current_block = &block;
	auto *type = llvm::Type::getFloatTy(context);
	auto *uint_type = llvm::Type::getInt32Ty(context);
	llvm::Argument x(type, 0), y(type, 1);
	llvm::ConstantFP one(type, 0x3f800000), two(type, 0x40000000), negative_one(type, 0xbf800000);
	llvm::ConstantInt dot_opcode(uint_type, unsigned(DXIL::Op::Dot2));
	llvm::ConstantInt rsqrt_opcode(uint_type, unsigned(DXIL::Op::Rsqrt));
	llvm::FunctionType dot_function(context, type, { uint_type, type, type, type, type });
	llvm::FunctionType unary_function(context, type, { uint_type, type });
	llvm::Function dot_callee(&dot_function, 1, llvm_module);
	llvm::Function unary_callee(&unary_function, 2, llvm_module);
	llvm_module.add_value_name(1, "dx.op.dot2.f32");
	llvm_module.add_value_name(2, "dx.op.unary.f32");
	llvm::CallInst dot(&dot_function, &dot_callee, { &dot_opcode, &x, &y, &x, &y });
	llvm::CallInst rsqrt(&unary_function, &unary_callee, { &rsqrt_opcode, &dot });
	llvm::BinaryOperator normalized(&x, &rsqrt, llvm::Instruction::FMul);
	llvm::BinaryOperator bounded(&one, &normalized, llvm::Instruction::FSub);
	bounded.setFast(true);
	check_sqrt(impl, &bounded, true, "normalized");

	llvm::BinaryOperator scaled(&two, &normalized, llvm::Instruction::FMul);
	llvm::BinaryOperator bounded_scaled(&two, &scaled, llvm::Instruction::FSub);
	bounded_scaled.setFast(true);
	check_sqrt(impl, &bounded_scaled, true, "scaled normalized");
	llvm::BinaryOperator mismatched(&one, &scaled, llvm::Instruction::FSub);
	mismatched.setFast(true);
	check_sqrt(impl, &mismatched, false, "mismatched bound");
	bounded.setFast(false);
	check_sqrt(impl, &bounded, false, "precise subtraction");
	bounded.setFast(true);

	llvm::BinaryOperator negative_x(&negative_one, &x, llvm::Instruction::FMul);
	llvm::BinaryOperator negative_normalized(&negative_x, &rsqrt, llvm::Instruction::FMul);
	llvm::BinaryOperator negative_bound(&one, &negative_normalized, llvm::Instruction::FSub);
	negative_bound.setFast(true);
	check_sqrt(impl, &negative_bound, false, "negative scale");

	// Repeated squaring shares both operands. Without a shared work budget,
	// traversing this compact expression requires exponentially many visits.
	llvm::Value *dag = &x;
	for (unsigned i = 0; i < 128; i++)
		dag = context.construct<llvm::BinaryOperator>(dag, dag, llvm::Instruction::FMul);
	llvm::BinaryOperator unbounded(&one, dag, llvm::Instruction::FSub);
	unbounded.setFast(true);
	check_sqrt(impl, &unbounded, false, "shared DAG");

	// A deep constant chain must also fall back conservatively when exhausted.
	llvm::Value *chain = &normalized;
	for (unsigned i = 0; i < 128; i++)
		chain = context.construct<llvm::BinaryOperator>(&one, chain, llvm::Instruction::FMul);
	llvm::BinaryOperator deep_bounded(&one, chain, llvm::Instruction::FSub);
	deep_bounded.setFast(true);
	check_sqrt(impl, &deep_bounded, false, "deep constant chain");
	// A previous query exhausting its budget must not disable the workaround.
	check_sqrt(impl, &bounded, true, "fresh query budget");
}

int main()
{
	begin_thread_allocator_context();
	test_analysis();
	end_thread_allocator_context();
	std::puts("8 sqrt analysis cases passed.");
}
