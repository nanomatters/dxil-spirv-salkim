/* Copyright (c) 2026 Erhan Bilgili
 * SPDX-License-Identifier: MIT
 */

#include "opcodes/converter_impl.hpp"
#include "opcodes/opcodes_llvm_builtins.hpp"
#include "context.hpp"
#include <cstdio>
#include <cstdlib>

using namespace dxil_spv;

static void check_at(bool result, unsigned line)
{
	if (!result)
	{
		std::fprintf(stderr, "Minimum-precision cast check failed at line %u.\n", line);
		std::exit(1);
	}
}
#define check(x) check_at((x), __LINE__)

static llvm::Type *float_type(llvm::LLVMContext &context, unsigned width, unsigned lanes)
{
	auto *scalar = width == 16 ? llvm::Type::getHalfTy(context) :
	               width == 32 ? llvm::Type::getFloatTy(context) : llvm::Type::getDoubleTy(context);
	return lanes == 1 ? scalar : llvm::VectorType::get(lanes, scalar);
}

static void test_cast(unsigned input_width, unsigned output_width, unsigned lanes, bool native, bool relaxed)
{
	llvm::LLVMContext context;
	LLVMBCParser parser;
	SPIRVModule module;
	Converter::Impl impl(parser, nullptr, module);
	Vector<Operation *> block;
	impl.current_block = &block;
	impl.options.min_precision_prefer_native_16bit = native;
	impl.options.arithmetic_relaxed_precision = relaxed;
	auto *input_type = float_type(context, input_width, lanes);
	auto *output_type = float_type(context, output_width, lanes);
	llvm::Argument input(input_type, 0);
	bool trunc = input_width > output_width;
	llvm::CastInst cast(output_type, &input, trunc ? llvm::Instruction::FPTrunc : llvm::Instruction::FPExt);
	check(emit_cast_instruction(impl, &cast));
	bool promoted_noop = !native && input_width <= 32 && output_width <= 32;

	if (promoted_noop && !(trunc && relaxed))
	{
		check(block.empty());
		check(impl.get_id_for_value(&cast) == impl.get_id_for_value(&input));
		check(impl.get_type_id(input_type) == impl.get_type_id(output_type));
	}
	else
	{
		check(block.size() == 1);
		auto *op = block.front();
		check(op->op == (promoted_noop ? spv::OpCopyObject : spv::OpFConvert));
		check(op->type_id == impl.get_type_id(output_type));
		check(op->argument(0) == impl.get_id_for_value(&input));
		check((impl.get_type_id(input_type) == impl.get_type_id(output_type)) == promoted_noop);
	}
}

int main()
{
	begin_thread_allocator_context();
	unsigned cases = 0;
	for (unsigned lanes : { 1u, 2u, 3u, 4u, 8u, 1024u })
	for (bool native : { false, true })
	for (bool relaxed : { false, true })
	for (unsigned input_width : { 16u, 32u, 64u })
	for (unsigned output_width : { 16u, 32u, 64u })
	{
		if (input_width == output_width)
			continue;
		test_cast(input_width, output_width, lanes, native, relaxed);
		cases++;
	}
	end_thread_allocator_context();
	std::printf("%u minimum-precision cast cases passed.\n", cases);
}
