/* Copyright (c) 2026 Erhan Bilgili
 * SPDX-License-Identifier: MIT
 */

#include "opcodes/converter_impl.hpp"
#include "opcodes/opcodes_dxil_builtins.hpp"
#include "context.hpp"
#include "GLSL.std.450.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace dxil_spv;

static void check_at(bool result, unsigned line)
{
	if (!result)
	{
		std::fprintf(stderr, "FMA codegen check failed at line %u.\n", line);
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

static void test_fma(unsigned width, unsigned lanes, bool native, unsigned features,
                     bool precise, bool force, bool float_controls2, unsigned operation)
{
	llvm::LLVMContext context;
	LLVMBCParser parser;
	SPIRVModule module;
	Converter::Impl impl(parser, nullptr, module);
	Vector<Operation *> block;
	impl.current_block = &block;
	impl.options.min_precision_prefer_native_16bit = native;
	impl.options.supports_fma_float16 = (features & 1) != 0;
	impl.options.supports_fma_float32 = (features & 2) != 0;
	impl.options.supports_fma_float64 = (features & 4) != 0;
	impl.options.force_precise = force;
	impl.options.quirks.precise_fma = operation == 2;
	impl.execution_mode_meta.float_controls2 = float_controls2;

	auto *type = float_type(context, width, lanes);
	auto *uint_type = llvm::Type::getInt32Ty(context);
	llvm::FunctionType function_type(context, type, { uint_type, type, type, type });
	llvm::ConstantInt opcode(uint_type, unsigned(operation ? DXIL::Op::FMad : DXIL::Op::Fma));
	llvm::Argument a(type, 0), b(type, 1), c(type, 2);
	llvm::CallInst call(&function_type, nullptr, { &opcode, &a, &b, &c });
	if (precise)
		call.setMetadata("dx.precise", nullptr);
	check(emit_dxil_instruction(impl, &call));

	unsigned emitted_width = width == 16 && !native ? 32 : width;
	unsigned feature = emitted_width == 16 ? 1 : emitted_width == 32 ? 2 : 4;
	bool use_fma = operation == 0 && (features & feature) != 0;
	bool split = operation == 1 && (precise || force);
	check(block.size() == (split ? 2u : 1u));
	check(impl.builder().hasCapability(spv::CapabilityFMAKHR) == use_fma);
	check(impl.builder().getScalarTypeWidth(impl.get_type_id(type)) == int(emitted_width));

	if (split)
	{
		check(block[0]->op == spv::OpFMul);
		check(block[1]->op == spv::OpFAdd);
		check(block[0]->argument(0) == impl.get_id_for_value(&a));
		check(block[0]->argument(1) == impl.get_id_for_value(&b));
		check(block[1]->argument(0) == block[0]->id);
		check(block[1]->argument(1) == impl.get_id_for_value(&c));
	}
	else
	{
		auto *op = block.front();
		check(op->op == (use_fma ? spv::OpFmaKHR : spv::OpExtInst));
		unsigned offset = use_fma ? 0 : 2;
		check(unsigned(op->end() - op->begin()) == offset + 3);
		if (!use_fma)
		{
			check(op->argument(0) == impl.glsl_std450_ext);
			check(op->argument(1) == GLSLstd450Fma);
		}
		check(op->argument(offset) == impl.get_id_for_value(&a));
		check(op->argument(offset + 1) == impl.get_id_for_value(&b));
		check(op->argument(offset + 2) == impl.get_id_for_value(&c));
	}

	for (auto *op : block)
	{
		check(op->type_id == impl.get_type_id(type));
		check(impl.builder().hasDecoration(op->id, spv::DecorationNoContraction) ==
		      ((precise || force) && !float_controls2));
		check(impl.builder().hasDecoration(op->id, spv::DecorationFPFastMathMode) ==
		      (precise && !force && float_controls2));
	}
	check(block.back()->id == impl.get_id_for_value(&call));

	Vector<uint32_t> spirv;
	impl.builder().dump(spirv);
	bool fma_extension = false;
	for (size_t i = 5; i < spirv.size(); i += spirv[i] >> 16)
	{
		auto op = spv::Op(spirv[i] & 0xffff);
		if (op == spv::OpExtension &&
		    std::strcmp(reinterpret_cast<const char *>(&spirv[i + 1]), "SPV_KHR_fma") == 0)
			fma_extension = true;
		if (op == spv::OpDecorate && spirv[i + 2] == spv::DecorationFPFastMathMode)
			check(spirv[i + 3] == spv::FPFastMathModeAllowRecipMask);
	}
	check(fma_extension == use_fma);
}

int main()
{
	begin_thread_allocator_context();
	unsigned cases = 0;
	for (unsigned width : { 16u, 32u, 64u })
	for (unsigned lanes : { 1u, 2u, 3u, 4u, 8u, 1024u })
	for (bool native : { false, true })
	for (unsigned features = 0; features < 8; features++)
	for (bool precise : { false, true })
	for (bool force : { false, true })
	for (bool float_controls2 : { false, true })
	for (unsigned operation = 0; operation < 3; operation++)
	{
		test_fma(width, lanes, native, features, precise, force, float_controls2, operation);
		cases++;
	}
	end_thread_allocator_context();
	std::printf("%u FMA codegen cases passed.\n", cases);
}
