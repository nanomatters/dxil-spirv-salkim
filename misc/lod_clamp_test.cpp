/* Copyright (c) 2026 Erhan Bilgili
 * SPDX-License-Identifier: MIT
 */

#include "opcodes/converter_impl.hpp"
#include "opcodes/dxil/dxil_sampling.hpp"
#include "context.hpp"
#include <cstdio>
#include <cstdlib>

using namespace dxil_spv;

static void check_at(bool condition, unsigned line)
{
	if (!condition)
	{
		std::fprintf(stderr, "LOD clamp check failed at line %u.\n", line);
		std::exit(1);
	}
}
#define check(condition) check_at((condition), __LINE__)

static void test_clamp(bool clamped)
{
	llvm::LLVMContext context;
	LLVMBCParser parser;
	SPIRVModule module;
	Converter::Impl impl(parser, nullptr, module);
	Vector<Operation *> block;
	impl.current_block = &block;
	impl.execution_model = spv::ExecutionModelGLCompute;
	impl.options.compute_shader_derivatives = false;
	auto &builder = impl.builder();
	auto f32 = builder.makeFloatType(32);
	auto image_type = builder.makeImageType(f32, spv::Dim2D, false, false, false, 1, spv::ImageFormatUnknown);
	auto *image = impl.allocate(spv::OpUndef, image_type);
	impl.add(image);
	impl.id_to_type[image->id] = image_type;

	auto *float_type = llvm::Type::getFloatTy(context);
	auto *int_type = llvm::Type::getInt32Ty(context);
	llvm::Argument image_arg(int_type, 0), sampler_arg(int_type, 1), u(float_type, 2), v(float_type, 3);
	llvm::ConstantInt opcode(int_type, unsigned(DXIL::Op::CalculateLOD)), clamp(int_type, clamped);
	llvm::UndefValue unused(float_type);
	impl.rewrite_value(&image_arg, image->id);
	llvm::FunctionType function(context, float_type, { int_type, int_type, int_type,
	                                               float_type, float_type, float_type, int_type });
	llvm::CallInst call(&function, nullptr, { &opcode, &image_arg, &sampler_arg, &u, &v, &unused, &clamp });
	check(emit_calculate_lod_instruction(impl, &call, false));

	spv::Id levels = 0, float_levels = 0, max_level = 0, min_result = 0;
	unsigned clamps = 0;
	for (auto *operation : block)
	{
		if (operation->op == spv::OpImageQueryLevels)
			levels = operation->id;
		else if (levels && operation->op == spv::OpConvertSToF && operation->argument(0) == levels)
			float_levels = operation->id;
		else if (float_levels && operation->op == spv::OpFSub && operation->argument(0) == float_levels)
		{
			// N levels have last valid index N - 1, including zero for a single level.
			check(operation->argument(1) == builder.makeFloatConstant(1.0f));
			max_level = operation->id;
		}
		else if (operation->op == spv::OpExtInst)
		{
			if (max_level && operation->argument(1) == GLSLstd450FMin && operation->argument(3) == max_level)
				min_result = operation->id;
			else if (min_result && operation->argument(1) == GLSLstd450FMax && operation->argument(2) == min_result)
			{
				// Min before max also maps the null-descriptor case (zero levels) to zero.
				check(operation->argument(3) == builder.makeFloatConstant(0.0f));
				check(impl.get_id_for_value(&call) == operation->id);
				clamps++;
			}
		}
	}
	check(bool(levels) == clamped);
	check(clamps == unsigned(clamped));
}

int main()
{
	begin_thread_allocator_context();
	test_clamp(false);
	test_clamp(true);
	end_thread_allocator_context();
	std::puts("2 LOD clamp cases passed.");
}
