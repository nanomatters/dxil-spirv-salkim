/* Copyright (c) 2026 Erhan Bilgili
 * SPDX-License-Identifier: MIT
 */

#include "opcodes/converter_impl.hpp"
#include "opcodes/dxil/dxil_arithmetic.hpp"
#include "context.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <unordered_map>

using namespace dxil_spv;

static void check_at(bool result, unsigned line)
{
	if (!result)
	{
		std::fprintf(stderr, "Exp/log codegen check failed at line %u.\n", line);
		std::exit(1);
	}
}
#define check(x) check_at((x), __LINE__)

static void test_composition(bool outer_exp, bool inner_exp, unsigned precise)
{
	llvm::LLVMContext context;
	llvm::Module llvm_module(context);
	LLVMBCParser parser;
	SPIRVModule module;
	Converter::Impl impl(parser, nullptr, module);
	Vector<Operation *> block;
	impl.current_block = &block;
	impl.options.force_precise = precise == 1;
	auto *type = llvm::Type::getFloatTy(context);
	auto *uint_type = llvm::Type::getInt32Ty(context);
	llvm::Argument input(type, 0);
	llvm::ConstantInt inner_opcode(uint_type, unsigned(inner_exp ? DXIL::Op::Exp : DXIL::Op::Log));
	llvm::ConstantInt outer_opcode(uint_type, unsigned(outer_exp ? DXIL::Op::Exp : DXIL::Op::Log));
	llvm::FunctionType function(context, type, { uint_type, type });
	llvm::Function unary(&function, 1, llvm_module);
	llvm_module.add_value_name(1, "dx.op.unary.f32");
	llvm::CallInst inner(&function, &unary, { &inner_opcode, &input });
	llvm::CallInst outer(&function, &unary, { &outer_opcode, &inner });
	llvm::MDNode metadata(nullptr, {});
	if (precise == 2)
	{
		inner.setMetadata("dx.precise", &metadata);
		outer.setMetadata("dx.precise", &metadata);
	}
	check(emit_dxil_std450_unary_instruction(inner_exp ? GLSLstd450Exp2 : GLSLstd450Log2, impl, &inner));
	check(emit_dxil_std450_unary_instruction(outer_exp ? GLSLstd450Exp2 : GLSLstd450Log2, impl, &outer));
	bool folded = !precise && outer_exp != inner_exp;
	check(block.size() == (folded ? 1u : 2u));
	if (folded)
		check(impl.get_id_for_value(&outer) == impl.get_id_for_value(&input));
	else
		check(block.back()->argument(2) == impl.get_id_for_value(&inner));

	// Interpret the emitted operations, including the rewrite's actual result ID.
	for (double x : { 1.0, 2.0, 4.0 })
	{
		std::unordered_map<spv::Id, double> values;
		values[impl.get_id_for_value(&input)] = x;
		for (const auto *op : block)
		{
			check(op->op == spv::OpExtInst);
			check(values.count(op->argument(2)));
			double operand = values[op->argument(2)];
			check(op->argument(1) == GLSLstd450Exp2 || op->argument(1) == GLSLstd450Log2);
			values[op->id] = op->argument(1) == GLSLstd450Exp2 ? std::exp2(operand) : std::log2(operand);
		}
		double expected = inner_exp ? std::exp2(x) : std::log2(x);
		expected = outer_exp ? std::exp2(expected) : std::log2(expected);
		double actual = values.at(impl.get_id_for_value(&outer));
		check(actual == expected);
	}
}

int main()
{
	begin_thread_allocator_context();
	for (bool outer_exp : { false, true })
	for (bool inner_exp : { false, true })
	for (unsigned precise = 0; precise < 3; precise++)
		test_composition(outer_exp, inner_exp, precise);
	end_thread_allocator_context();
	std::puts("12 exp/log composition cases passed.");
}
