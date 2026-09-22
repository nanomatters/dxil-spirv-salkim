/* Copyright (c) 2026 Erhan Bilgili
 * SPDX-License-Identifier: MIT
 */

#include "opcodes/dxil/dxil_common.hpp"
#include "context.hpp"
#include <cstdio>
#include <cstdlib>

static void check_at(bool condition, const char *expression, unsigned line)
{
	if (!condition)
	{
		std::fprintf(stderr, "Raw buffer address check failed at line %u: %s\n", line, expression);
		std::exit(1);
	}
}

#define check(condition) check_at((condition), #condition, __LINE__)

using Op = llvm::Instruction::BinaryOps;

static void test_bitwise_case(Op opcode, uint32_t scale, uint32_t bias, bool shift,
                             bool constant_lhs, bool can_split)
{
	llvm::LLVMContext context;
	auto *type = llvm::Type::getInt32Ty(context);
	llvm::Argument index(type, 0);
	unsigned shift_count = 0;
	if (shift)
	{
		check(scale != 0 && (scale & (scale - 1)) == 0);
		while ((uint32_t(1) << shift_count) != scale)
			shift_count++;
	}
	llvm::ConstantInt factor(type, shift ? shift_count : scale);
	llvm::ConstantInt constant(type, bias);
	llvm::BinaryOperator scaled(&index, &factor, shift ? Op::Shl : Op::Mul);
	llvm::BinaryOperator address(constant_lhs ? static_cast<llvm::Value *>(&constant) : &scaled,
	                             constant_lhs ? static_cast<llvm::Value *>(&scaled) : &constant, opcode);

	for (unsigned stride : { 1u, 4u, 16u })
	for (unsigned addr_shift : { 0u, 1u, 2u, 3u })
	for (unsigned vecsize : { 1u, 2u, 4u })
	{
		unsigned element_size = (1u << addr_shift) * vecsize;
		dxil_spv::RawBufferAccessSplit split = {};
		bool result = dxil_spv::extract_raw_buffer_access_split(&address, stride, addr_shift, vecsize, split);
		bool aligned = (uint64_t(scale) * stride) % element_size == 0 &&
		               (int64_t(int32_t(bias)) * stride) % element_size == 0;
		check(result == (can_split && aligned));
		if (!result)
			continue;
		check(split.dynamic_index == &index);

		// Include values where the scaled index overlaps the bias, and wrap boundaries.
		for (uint32_t input : { 0u, 1u, 2u, 3u, 7u, 15u, 16u, 31u, 32u, 63u,
		                        0x0fffffffu, 0x10000000u, 0x7fffffffu, 0x80000000u, 0xffffffffu })
		{
			uint32_t product = input * scale;
			uint32_t expected = (opcode == Op::Or ? product | bias : product ^ bias) * stride;
			uint32_t actual = uint32_t((split.scale * input + uint64_t(split.bias)) * element_size);
			check(actual == expected);
		}
	}
}

static void test_bitwise()
{
	for (Op opcode : { Op::Or, Op::Xor })
	for (bool constant_lhs : { false, true })
	{
		for (bool shift : { false, true })
		{
			// The scale's own bits do not describe all possible bits of the product.
			test_bitwise_case(opcode, 16, 32, shift, constant_lhs, false);
			test_bitwise_case(opcode, 16, 16, shift, constant_lhs, false);
			test_bitwise_case(opcode, 16, 0x80000000u, shift, constant_lhs, false);
			test_bitwise_case(opcode, 16, 0xffffffffu, shift, constant_lhs, false);
			// Proven low-bit biases must still be optimized.
			test_bitwise_case(opcode, 16, 0, shift, constant_lhs, true);
			test_bitwise_case(opcode, 16, 4, shift, constant_lhs, true);
			test_bitwise_case(opcode, 16, 8, shift, constant_lhs, true);
			test_bitwise_case(opcode, 16, 12, shift, constant_lhs, true);
			test_bitwise_case(opcode, 0x80000000u, 4, shift, constant_lhs, true);
		}
		// Non-power-of-two scales only guarantee their trailing zero bits.
		test_bitwise_case(opcode, 12, 16, false, constant_lhs, false);
		test_bitwise_case(opcode, 12, 2, false, constant_lhs, true);
		test_bitwise_case(opcode, 3, 4, false, constant_lhs, false);
		test_bitwise_case(opcode, 1, 2, false, constant_lhs, false);
		test_bitwise_case(opcode, 0, 32, false, constant_lhs, true);
	}
}

static void test_arithmetic()
{
	for (Op opcode : { Op::Add, Op::Sub })
	for (bool constant_lhs : { false, true })
	for (uint32_t scale : { 4u, 16u, 32u })
	for (uint32_t bias : { 0u, 16u, 256u, 0xfffffff0u })
	{
		llvm::LLVMContext context;
		auto *type = llvm::Type::getInt32Ty(context);
		llvm::Argument index(type, 0);
		llvm::ConstantInt factor(type, scale);
		llvm::ConstantInt constant(type, bias);
		llvm::BinaryOperator scaled(&index, &factor, Op::Mul);
		llvm::BinaryOperator address(constant_lhs ? static_cast<llvm::Value *>(&constant) : &scaled,
		                             constant_lhs ? static_cast<llvm::Value *>(&scaled) : &constant, opcode);
		bool reversed_sub = opcode == Op::Sub && constant_lhs;

		for (unsigned stride : { 1u, 4u, 16u })
		for (unsigned vecsize : { 1u, 2u, 4u })
		{
			unsigned element_size = 4 * vecsize;
			dxil_spv::RawBufferAccessSplit split = {};
			bool result = dxil_spv::extract_raw_buffer_access_split(&address, stride, 2, vecsize, split);
			bool aligned = reversed_sub ? stride % element_size == 0 :
			               (uint64_t(scale) * stride) % element_size == 0 &&
			               (int64_t(int32_t(bias)) * stride) % element_size == 0;
			check(result == aligned);
			if (!result)
				continue;
			// A constant on the left of subtraction must remain in the dynamic expression.
			check(split.dynamic_index == (reversed_sub ? static_cast<llvm::Value *>(&address) : &index));

			for (uint32_t input : { 0u, 1u, 2u, 15u, 16u, 17u, 31u, 0x10000000u,
			                        0x7fffffffu, 0x80000000u, 0xffffffffu })
			{
				uint32_t product = input * scale;
				uint32_t expected = opcode == Op::Add ? product + bias :
				                    constant_lhs ? bias - product : product - bias;
				uint32_t dynamic = reversed_sub ? expected : input;
				uint32_t actual = uint32_t((split.scale * dynamic + uint64_t(split.bias)) * element_size);
				check(actual == expected * stride);
			}
		}
	}
}

int main()
{
	dxil_spv::begin_thread_allocator_context();
	test_bitwise();
	test_arithmetic();
	dxil_spv::end_thread_allocator_context();
	std::puts("Raw buffer address tests passed.");
}
