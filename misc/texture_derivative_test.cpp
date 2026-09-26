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
		std::fprintf(stderr, "Texture derivative check failed at line %u.\n", line);
		std::exit(1);
	}
}
#define check(condition) check_at((condition), __LINE__)

static spv::Id operation_type(const Vector<Operation *> &block, spv::Id id)
{
	for (auto *operation : block)
		if (operation->id == id)
			return operation->type_id;
	check(false);
	return 0;
}

static void test_derivatives(unsigned dimensions, bool arrayed, DXIL::Op opcode)
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
	auto i32 = builder.makeIntType(32);
	auto gradient_type = impl.build_vector_type(f32, dimensions);
	auto dim = dimensions == 1 ? spv::Dim1D : dimensions == 2 ? spv::Dim2D : spv::Dim3D;
	auto image_type = builder.makeImageType(f32, dim, false, arrayed, false, 1, spv::ImageFormatUnknown);
	auto *image = impl.allocate(spv::OpUndef, image_type);
	auto *sampler = impl.allocate(spv::OpUndef, builder.makeSamplerType());
	impl.add(image);
	impl.add(sampler);
	impl.id_to_type[image->id] = image_type;
	impl.id_to_type[sampler->id] = sampler->type_id;
	impl.handle_to_resource_meta[image->id] = {};
	impl.handle_to_resource_meta[image->id].component_type = DXIL::ComponentType::F32;
	impl.handle_to_resource_meta[sampler->id] = {};

	auto *float_type = llvm::Type::getFloatTy(context);
	auto *int_type = llvm::Type::getInt32Ty(context);
	llvm::Argument image_arg(int_type, 0), sampler_arg(int_type, 1);
	llvm::Argument u(float_type, 2), v(float_type, 3), w(float_type, 4), bias(float_type, 5);
	llvm::ConstantInt op(int_type, unsigned(opcode)), zero(int_type, 0);
	llvm::UndefValue unused_float(float_type);
	impl.rewrite_value(&image_arg, image->id);
	impl.rewrite_value(&sampler_arg, sampler->id);
	for (auto *value : { &u, &v, &w, &bias })
		impl.add(impl.allocate(spv::OpUndef, value));

	bool lod = opcode == DXIL::Op::CalculateLOD;
	auto *result_type = lod ? float_type : llvm::VectorType::get(4, float_type);
	llvm::FunctionType function(context, result_type, {});
	Vector<llvm::Value *> arguments = { &op, &image_arg, &sampler_arg, &u, &v, &w };
	if (lod)
		arguments.push_back(&zero); // Unclamped, so this tests dimensions independently of the mip bound.
	else
	{
		arguments.push_back(&unused_float);
		arguments.push_back(&zero);
		arguments.push_back(&zero);
		arguments.push_back(&zero);
		arguments.push_back(opcode == DXIL::Op::SampleBias ? static_cast<llvm::Value *>(&bias) : &unused_float);
		if (opcode == DXIL::Op::SampleBias)
			arguments.push_back(&unused_float);
	}
	llvm::CallInst call(&function, nullptr, std::move(arguments));
	impl.llvm_composite_meta[&call].access_mask = 0xf;
	check(lod ? emit_calculate_lod_instruction(impl, &call, false) : emit_sample_instruction(opcode, impl, &call));

	unsigned quads = 0, dots = 0, samples = 0, queries = 0, extracts = 0, bias_scales = 0;
	for (auto *operation : block)
	{
		if (operation->op == spv::OpGroupNonUniformQuadSwap)
		{
			quads++;
			check(operation->type_id == gradient_type);
			check(operation_type(block, operation->argument(1)) == gradient_type);
		}
		if (operation->op == spv::OpDot)
		{
			dots++;
			check(dimensions > 1);
			check(operation_type(block, operation->argument(0)) == gradient_type);
		}
		if (operation->op == spv::OpImageQuerySizeLod)
		{
			queries++;
			check(operation->type_id == impl.build_vector_type(i32, dimensions + unsigned(arrayed)));
		}
		if (operation->op == spv::OpCompositeExtract && operation->type_id == i32)
		{
			extracts++;
			check(operation->argument(1) == 0);
		}
		if (!lod && (operation->op == spv::OpFMul || operation->op == spv::OpVectorTimesScalar))
		{
			bias_scales++;
			check(operation->op == (dimensions == 1 ? spv::OpFMul : spv::OpVectorTimesScalar));
			check(operation->type_id == gradient_type);
			check(operation_type(block, operation->argument(0)) == gradient_type);
			check(operation_type(block, operation->argument(1)) == f32);
		}
		if (operation->op == spv::OpImageSampleExplicitLod)
		{
			samples++;
			check(operation->argument(2) == spv::ImageOperandsGradMask);
			check(operation_type(block, operation->argument(3)) == gradient_type);
			check(operation_type(block, operation->argument(4)) == gradient_type);
		}
	}
	check(quads == 2);
	check(dots == (lod && dimensions > 1 ? 2u : 0u));
	check(queries == unsigned(lod));
	check(extracts == unsigned(lod && dimensions == 1 && arrayed));
	check(samples == unsigned(!lod));
	check(bias_scales == (opcode == DXIL::Op::SampleBias ? 2u : 0u));
}

int main()
{
	begin_thread_allocator_context();
	unsigned cases = 0;
	for (unsigned dimensions : { 1u, 2u, 3u })
	for (bool arrayed : { false, true })
	for (auto opcode : { DXIL::Op::Sample, DXIL::Op::SampleBias, DXIL::Op::CalculateLOD })
	{
		if (dimensions == 3 && arrayed)
			continue;
		test_derivatives(dimensions, arrayed, opcode);
		cases++;
	}
	end_thread_allocator_context();
	std::printf("%u texture derivative cases passed.\n", cases);
}
