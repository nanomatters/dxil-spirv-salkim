/* Copyright (c) 2026 Erhan Bilgili
 * SPDX-License-Identifier: MIT
 */

#include "opcodes/converter_impl.hpp"
#include "opcodes/dxil/dxil_arithmetic.hpp"
#include "context.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <unordered_map>

using namespace dxil_spv;

static void check_at(bool result, unsigned line)
{
	if (!result)
	{
		std::fprintf(stderr, "Bit scan check failed at line %u.\n", line);
		std::exit(1);
	}
}
#define check(x) check_at((x), __LINE__)

static uint32_t reference(uint64_t value, GLSLstd450 opcode)
{
	if (opcode == GLSLstd450FindSMsb && (value >> 63)) value = ~value;
	if (!value) return UINT32_MAX;
	if (opcode == GLSLstd450FindILsb)
	{
		unsigned bit = 0;
		while (!(value & 1)) { value >>= 1; bit++; }
		return bit;
	}
	unsigned bit = 0;
	while (value >>= 1) bit++;
	return bit;
}

static void test_scan(unsigned lanes, GLSLstd450 opcode)
{
	llvm::LLVMContext context;
	LLVMBCParser parser;
	SPIRVModule module;
	Converter::Impl impl(parser, nullptr, module);
	Vector<Operation *> block;
	impl.current_block = &block;
	auto *scalar = llvm::Type::getInt64Ty(context);
	llvm::Type *type = lanes == 1 ? scalar : llvm::VectorType::get(lanes, scalar);
	llvm::Argument input(type, 0);
	spv::Id result = emit_native_bitscan(opcode, impl, nullptr, &input);
	auto &builder = impl.builder();
	std::unordered_map<spv::Id, uint64_t> values;
	for (unsigned shift : { 31u, 32u })
		values[impl.build_splat_constant_vector(builder.makeUintType(32), builder.makeUintConstant(shift), lanes)] = shift;

	for (const auto *op : block)
		check(unsigned(builder.getNumTypeComponents(op->type_id)) <= 1024);
	// The existing lowering is deliberately unchanged below this boundary.
	if (lanes <= 512)
		return;
	for (const auto *op : block)
		check(unsigned(builder.getNumTypeComponents(op->type_id)) == lanes);

	uint64_t random = 0xb5ad4eceda1ce2a9ull;
	for (unsigned sample = 0; sample < 8192; sample++)
	{
		uint64_t x;
		if (sample < 64) x = uint64_t(1) << sample;
		else if (sample < 128) x = ~(uint64_t(1) << (sample - 64));
		else if (sample < 192) x = (uint64_t(1) << (sample - 128)) - 1;
		else if (sample < 256) x = ~((uint64_t(1) << (sample - 192)) - 1);
		else { random = random * 6364136223846793005ull + 1; x = random; }
		values[impl.get_id_for_value(&input)] = x;
		// Interpret the emitted lane-wise integer IR, not a second copy of the lowering.
		for (const auto *op : block)
		{
			auto operand = [&](unsigned i) { check(values.count(op->argument(i))); return values[op->argument(i)]; };
			uint64_t value = 0;
			switch (op->op)
			{
			case spv::OpUConvert: value = uint32_t(operand(0)); break;
			case spv::OpShiftRightLogical: value = operand(0) >> operand(1); break;
			case spv::OpShiftRightArithmetic:
				value = (uint32_t(operand(0)) & 0x80000000u) ? UINT32_MAX : 0;
				check(operand(1) == 31);
				break;
			case spv::OpBitwiseXor: value = operand(0) ^ operand(1); break;
			case spv::OpBitwiseOr: value = operand(0) | operand(1); break;
			case spv::OpExtInst:
				if (op->argument(1) == GLSLstd450FindILsb || op->argument(1) == GLSLstd450FindUMsb)
					value = reference(uint32_t(operand(2)), GLSLstd450(op->argument(1)));
				else if (op->argument(1) == GLSLstd450UMin)
					value = std::min(uint32_t(operand(2)), uint32_t(operand(3)));
				else
				{
					check(op->argument(1) == GLSLstd450SMax);
					value = uint32_t(std::max(int32_t(operand(2)), int32_t(operand(3))));
				}
				break;
			default: check(false);
			}
			values[op->id] = value;
		}
		check(values[result] == reference(x, opcode));
	}
}

int main()
{
	begin_thread_allocator_context();
	for (unsigned lanes : { 1u, 2u, 3u, 4u, 8u, 512u, 513u, 768u, 1024u })
	for (auto opcode : { GLSLstd450FindILsb, GLSLstd450FindUMsb, GLSLstd450FindSMsb })
		test_scan(lanes, opcode);
	end_thread_allocator_context();
	std::puts("27 shapes and 73728 wide bit-scan evaluations passed.");
}
