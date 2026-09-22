/* Copyright (c) 2026 Erhan Bilgili
 * SPDX-License-Identifier: MIT
 */

#include "opcodes/dxil/dxil_buffer.hpp"
#include "opcodes/dxil/dxil_common.hpp"
#include "context.hpp"
#include <cstdio>
#include <cstdlib>

static void check_at(bool condition, const char *expression, unsigned line)
{
	if (!condition)
	{
		std::fprintf(stderr, "Buffer alignment check failed at line %u: %s\n", line, expression);
		std::exit(1);
	}
}
#define check(condition) check_at((condition), #condition, __LINE__)

using Op = llvm::Instruction::BinaryOps;
using dxil_spv::get_known_trailing_zeros;

static void test_expressions()
{
	llvm::LLVMContext context;
	auto *type = llvm::Type::getInt32Ty(context);
	llvm::Argument x(type, 0), y(type, 1), condition(llvm::Type::getInt1Ty(context), 2);
	llvm::ConstantInt zero(type, 0), four(type, 4), twelve(type, 12), mask(type, 0xfffffff0u);
	llvm::BinaryOperator aligned(&x, &mask, Op::And);
	llvm::BinaryOperator reversed(&mask, &x, Op::And);
	check(get_known_trailing_zeros(&x) == 0);
	check(get_known_trailing_zeros(&zero) == 32);
	check(get_known_trailing_zeros(&aligned) == 4);
	check(get_known_trailing_zeros(&reversed) == 4);
	llvm::BinaryOperator shl(&aligned, &four, Op::Shl);
	llvm::BinaryOperator shr(&shl, &four, Op::LShr);
	llvm::BinaryOperator ashr(&shl, &four, Op::AShr);
	llvm::BinaryOperator dynamic_shift(&aligned, &y, Op::Shl);
	llvm::ConstantInt too_far(type, 32);
	llvm::BinaryOperator invalid_shift(&aligned, &too_far, Op::Shl);
	check(get_known_trailing_zeros(&shl) == 8);
	check(get_known_trailing_zeros(&shr) == 4);
	check(get_known_trailing_zeros(&ashr) == 4);
	check(get_known_trailing_zeros(&dynamic_shift) == 0);
	check(get_known_trailing_zeros(&invalid_shift) == 0);
	llvm::BinaryOperator sum(&aligned, &twelve, Op::Add);
	llvm::BinaryOperator sub(&twelve, &aligned, Op::Sub);
	llvm::BinaryOperator product(&aligned, &twelve, Op::Mul);
	llvm::BinaryOperator division(&aligned, &four, Op::UDiv);
	check(get_known_trailing_zeros(&sum) == 2);
	check(get_known_trailing_zeros(&sub) == 2);
	check(get_known_trailing_zeros(&product) == 6);
	check(get_known_trailing_zeros(&division) == 0);
	llvm::SelectInst good(&aligned, &shl, &condition), bad(&aligned, &y, &condition);
	check(get_known_trailing_zeros(&good) == 4);
	check(get_known_trailing_zeros(&bad) == 0);
	llvm::CastInst trunc(llvm::Type::getInt8Ty(context), &shl, llvm::Instruction::Trunc);
	llvm::CastInst zext(type, &trunc, llvm::Instruction::ZExt);
	llvm::CastInst sext(type, &trunc, llvm::Instruction::SExt);
	llvm::Argument floating(llvm::Type::getFloatTy(context), 3);
	llvm::CastInst bitcast(type, &floating, llvm::Instruction::BitCast);
	check(get_known_trailing_zeros(&trunc) == 8);
	check(get_known_trailing_zeros(&zext) >= 8);
	check(get_known_trailing_zeros(&sext) >= 8);
	check(get_known_trailing_zeros(&bitcast) == 0);
	llvm::PHINode phi(type, 1);
	phi.add_incoming(&phi, nullptr);
	check(get_known_trailing_zeros(&phi) == 0);

	// A shared DAG must not trigger exponential traversal or defeat the budget.
	llvm::Value *deep = &aligned;
	for (unsigned i = 0; i < 64; i++)
		deep = context.construct<llvm::BinaryOperator>(deep, deep, Op::Add);
	check(get_known_trailing_zeros(deep) == 0);
	llvm::BinaryOperator masked_deep(deep, &mask, Op::And);
	check(get_known_trailing_zeros(&masked_deep) == 4);
}

static void test_low_bits()
{
	for (unsigned width : { 8u, 16u, 32u, 64u })
	for (unsigned a_bits = 0; a_bits <= 8; a_bits++)
	for (unsigned b_bits = 0; b_bits <= 8; b_bits++)
	{
		llvm::LLVMContext context;
		auto *type = llvm::Type::getIntTy(context, width);
		uint64_t type_mask = UINT64_MAX >> (64 - width);
		uint64_t a_mask = (UINT64_MAX << a_bits) & type_mask;
		uint64_t b_mask = (UINT64_MAX << b_bits) & type_mask;
		llvm::Argument x(type, 0), y(type, 1);
		llvm::ConstantInt ca(type, a_mask), cb(type, b_mask);
		llvm::BinaryOperator a(&x, &ca, Op::And), b(&y, &cb, Op::And);
		for (Op opcode : { Op::And, Op::Or, Op::Xor, Op::Add, Op::Sub, Op::Mul })
		{
			llvm::BinaryOperator expression(&a, &b, opcode);
			unsigned zeros = get_known_trailing_zeros(&expression);
			check(zeros <= width);
			uint64_t low_mask = zeros == 64 ? UINT64_MAX : (uint64_t(1) << zeros) - 1;
			uint64_t random = 0x8e2599a1d72564abull;
			for (unsigned i = 0; i < 256; i++)
			{
				random = random * 6364136223846793005ull + 1;
				uint64_t av = (i < 128 ? uint64_t(i) : random) & a_mask;
				random = random * 6364136223846793005ull + 1;
				uint64_t bv = (i < 128 ? ~uint64_t(i) : random) & b_mask;
				uint64_t result = 0;
				switch (opcode)
				{
				case Op::And: result = av & bv; break;
				case Op::Or: result = av | bv; break;
				case Op::Xor: result = av ^ bv; break;
				case Op::Add: result = av + bv; break;
				case Op::Sub: result = av - bv; break;
				case Op::Mul: result = av * bv; break;
				default: std::abort();
				}
				check((result & type_mask & low_mask) == 0);
			}
		}
	}
}

static void test_vectorization_guards()
{
	llvm::LLVMContext context;
	dxil_spv::LLVMBCParser parser;
	dxil_spv::SPIRVModule module;
	dxil_spv::Converter::Impl impl(parser, nullptr, module);
	auto *type = llvm::Type::getInt32Ty(context);
	llvm::Argument offset(type, 0), index(type, 1);
	llvm::ConstantInt mask16(type, 0xfffffff0u), mask8(type, 0xfffffff8u), mask64(type, 0xffffffc0u);
	llvm::BinaryOperator aligned(&offset, &mask16, Op::And);
	llvm::BinaryOperator weaker(&offset, &mask8, Op::And);
	llvm::BinaryOperator stronger(&offset, &mask64, Op::And);
	using dxil_spv::raw_access_byte_address_can_vectorize;
	using dxil_spv::raw_access_structured_can_vectorize;
	check(raw_access_byte_address_can_vectorize(impl, type, &aligned, 4));
	check(!raw_access_byte_address_can_vectorize(impl, type, &weaker, 4));
	check(!raw_access_byte_address_can_vectorize(impl, type, &offset, 4));
	check(!raw_access_byte_address_can_vectorize(impl, type, &aligned, 3));
	check(!raw_access_byte_address_can_vectorize(impl, type, &stronger, 8));
	impl.options.scalar_block_layout = true;
	impl.options.supports_per_component_robustness = true;
	check(raw_access_byte_address_can_vectorize(impl, type, &stronger, 8));
	// Power-of-two alignment does not prove divisibility by a vec3 stride.
	check(!raw_access_byte_address_can_vectorize(impl, type, &aligned, 3));
	check(raw_access_structured_can_vectorize(impl, type, &index, 32, &aligned, 4));
	check(!raw_access_structured_can_vectorize(impl, type, &index, 12, &aligned, 4));
	check(!raw_access_structured_can_vectorize(impl, type, &index, 32, &weaker, 4));
	check(!raw_access_structured_can_vectorize(impl, type, &index, 32, &aligned, 3));
	impl.execution_mode_meta.native_16bit_operations = true;
	check(raw_access_byte_address_can_vectorize(impl, llvm::Type::getInt16Ty(context), &weaker, 4));
	check(!raw_access_byte_address_can_vectorize(impl, llvm::Type::getInt64Ty(context), &aligned, 4));
	check(raw_access_byte_address_can_vectorize(impl, llvm::Type::getInt64Ty(context), &stronger, 4));
}

int main()
{
	dxil_spv::begin_thread_allocator_context();
	test_expressions();
	test_low_bits();
	test_vectorization_guards();
	dxil_spv::end_thread_allocator_context();
	std::puts("Buffer alignment tests passed, including 497664 low-bit checks.");
}
