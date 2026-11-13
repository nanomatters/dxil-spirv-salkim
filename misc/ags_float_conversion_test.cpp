/* Copyright (c) 2026 Erhan Bilgili
 * SPDX-License-Identifier: MIT
 */

#include "opcodes/converter_impl.hpp"
#include "opcodes/dxil/dxil_ags.hpp"
#include "context.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using namespace dxil_spv;

static void check_at(bool result, unsigned line)
{
	if (!result)
	{
		std::fprintf(stderr, "AGS float conversion check failed at line %u.\n", line);
		std::exit(1);
	}
}
#define check(x) check_at((x), __LINE__)

static uint32_t float_bits(float value)
{
	uint32_t bits;
	std::memcpy(&bits, &value, sizeof(bits));
	return bits;
}

static std::vector<double> half_values()
{
	std::vector<double> values;
	for (unsigned h = 0; h < 0x7c00; h++)
	{
		unsigned exponent = h >> 10;
		unsigned mantissa = h & 0x3ff;
		values.push_back(exponent ? std::ldexp(double(1024 + mantissa), int(exponent) - 25) :
		                            std::ldexp(double(mantissa), -24));
	}
	return values;
}

static uint32_t reference(uint32_t bits, const std::vector<double> &values)
{
	uint32_t sign = (bits >> 16) & 0x8000;
	float f;
	std::memcpy(&f, &bits, sizeof(f));
	double value = std::fabs(double(f));
	if (std::isnan(value)) return sign | 0x7e00;
	if (value >= 65520.0) return sign | 0x7c00;
	auto upper = std::lower_bound(values.begin(), values.end(), value);
	if (upper == values.begin()) return sign;
	if (upper == values.end()) return sign | 0x7bff;
	unsigned index = unsigned(upper - values.begin());
	double down = value - *(upper - 1);
	double up = *upper - value;
	if (down < up || (down == up && (index & 1))) index--;
	return sign | index;
}

static void test_rounding(unsigned immediate)
{
	llvm::LLVMContext context;
	LLVMBCParser parser;
	SPIRVModule module;
	Converter::Impl impl(parser, nullptr, module);
	Vector<Operation *> block;
	impl.current_block = &block;
	auto *type = llvm::Type::getInt32Ty(context);
	llvm::ConstantInt dx_opcode(type, unsigned(DXIL::Op::AtomicCompareExchange));
	llvm::ConstantInt instruction(type, 0x5000001c | (immediate << 8));
	llvm::ConstantInt zero(type, 0);
	llvm::Argument input(type, 0, false), handle(type, 1, false);
	llvm::FunctionType function_type(context, type, { type, type, type, type, type, type, type });
	llvm::CallInst call(&function_type, nullptr, { &dx_opcode, &handle, &instruction, &zero, &zero, &input, &zero });
	check(emit_magic_ags_instruction(impl, &call) == (immediate == 1));
	if (immediate != 1)
	{
		check(block.empty());
		return;
	}

	auto &builder = impl.builder();
	check(!builder.hasCapability(spv::CapabilityFloat16));
	check(!builder.hasCapability(spv::CapabilityInt16));
	check(!impl.shader_analysis.require_wmma);
	check(impl.ags.num_instructions == 0);
	check(block.back()->id == impl.get_id_for_value(&call));

	Vector<uint32_t> declarations;
	builder.dump(declarations);
	std::vector<uint32_t> values(block.back()->id + 1);
	for (size_t i = 5; i < declarations.size(); i += declarations[i] >> 16)
		if (spv::Op(declarations[i] & 0xffff) == spv::OpConstant)
			values.at(declarations[i + 2]) = declarations[i + 3];
	const auto halves = half_values();
	unsigned count = 0;
	auto evaluate = [&](uint32_t bits) {
		values.at(impl.get_id_for_value(&input)) = bits;
		for (auto *op : block)
		{
			auto operand = [&](unsigned i) { return values.at(op->argument(i)); };
			uint32_t value = 0;
			switch (op->op)
			{
			case spv::OpBitwiseAnd: value = operand(0) & operand(1); break;
			case spv::OpBitwiseOr: value = operand(0) | operand(1); break;
			case spv::OpShiftRightLogical: check(operand(1) < 32); value = operand(0) >> operand(1); break;
			case spv::OpShiftLeftLogical: check(operand(1) < 32); value = operand(0) << operand(1); break;
			case spv::OpIAdd: value = operand(0) + operand(1); break;
			case spv::OpISub: value = operand(0) - operand(1); break;
			case spv::OpUGreaterThanEqual: value = operand(0) >= operand(1); break;
			case spv::OpUGreaterThan: value = operand(0) > operand(1); break;
			case spv::OpULessThan: value = operand(0) < operand(1); break;
			case spv::OpSelect: value = operand(0) ? operand(1) : operand(2); break;
			default: check(false);
			}
			values.at(op->id) = value;
		}
		uint32_t result = values.at(block.back()->id);
		uint32_t expected = reference(bits, halves);
		// AGS does not specify a NaN payload. Check its class, sign and zero upper half.
		if ((bits & 0x7fffffff) > 0x7f800000)
			check((result & ~0x3ffu) == (expected & ~0x3ffu) && (result & 0x3ff));
		else if (result != expected)
		{
			std::fprintf(stderr, "Input %08x: got %04x, expected %04x.\n", bits, result, expected);
			check(false);
		}
		count++;
	};

	// Every representable finite half, and both sides of every rounding boundary.
	for (size_t i = 0; i < halves.size(); i++)
	{
		uint32_t exact = float_bits(float(halves[i]));
		double next = i + 1 == halves.size() ? 65536.0 : halves[i + 1];
		uint32_t midpoint = float_bits(float((halves[i] + next) / 2.0));
		for (uint32_t sign : { 0u, 0x80000000u })
		{
			evaluate(exact | sign);
			evaluate((midpoint - 1) | sign);
			evaluate(midpoint | sign);
			evaluate((midpoint + 1) | sign);
		}
	}
	for (uint32_t bits : { 0x3f801800u, 0x7f800000u, 0xff800000u, 0x7f800001u, 0xff800001u,
	                       0x7fc00000u, 0xffffffffu, 0x00000001u, 0x80000001u, 0x007fffffu, 0x807fffffu })
		evaluate(bits);
	uint32_t random = 0x92314735;
	for (unsigned i = 0; i < 65536; i++)
	{
		random ^= random << 13;
		random ^= random >> 17;
		random ^= random << 5;
		evaluate(random);
	}
	std::printf("%u AGS nearest-half conversions passed.\n", count);
}

int main()
{
	begin_thread_allocator_context();
	for (unsigned mode : { 0u, 1u, 2u, 3u, 0xffffu }) test_rounding(mode);
	end_thread_allocator_context();
}
