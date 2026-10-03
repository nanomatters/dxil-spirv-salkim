/* Copyright (c) 2026 Erhan Bilgili
 * SPDX-License-Identifier: MIT
 */

#include "opcodes/converter_impl.hpp"
#include "opcodes/dxil/dxil_nvapi.hpp"
#include "opcodes/dxil/dxil_buffer.hpp"
#include "context.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_map>

using namespace dxil_spv;

static void check_at(bool result, unsigned line)
{
	if (!result)
	{
		std::fprintf(stderr, "NVAPI codegen check failed at line %u.\n", line);
		std::exit(1);
	}
}
#define check(x) check_at((x), __LINE__)

static void test_instance(bool custom_id, bool lss)
{
	llvm::LLVMContext context;
	LLVMBCParser parser;
	SPIRVModule module;
	Converter::Impl impl(parser, nullptr, module);
	Vector<Operation *> block;
	impl.current_block = &block;
	impl.options.supports_ray_tracing_linear_swept_spheres = lss;
	auto *type = llvm::Type::getInt32Ty(context);
	// NVAPI GetInstanceID is 74; GetInstanceIndex is 75.
	llvm::ConstantInt opcode(type, custom_id ? 74 : 75);
	llvm::Argument object(type, 0);
	impl.nvapi.fake_doorbell_inputs[NVAPI_ARGUMENT_OPCODE] = &opcode;
	impl.nvapi.fake_doorbell_inputs[NVAPI_ARGUMENT_SRC0U] = &object;
	check(impl.nvapi.commit_opcode(impl, false));
	check(block.size() == 1);
	check(block[0]->op == (custom_id ? spv::OpHitObjectGetInstanceCustomIndexNV : spv::OpHitObjectGetInstanceIdNV));
	check(block[0]->argument(0) == impl.get_id_for_value(&object));
	check(impl.nvapi.fake_doorbell_outputs[0] == block[0]->id);
	check(!impl.builder().hasCapability(spv::CapabilityRayTracingLinearSweptSpheresGeometryNV));
}

static void test_lss(unsigned opcode, bool supported, bool analysis,
                     spv::ExecutionModel stage, unsigned query_mode, unsigned flags_mode)
{
	llvm::LLVMContext context;
	llvm::Module llvm_module(context);
	LLVMBCParser parser;
	SPIRVModule module;
	module.emit_entry_point(stage, "main", false, spv::MemoryModelGLSL450);
	Converter::Impl impl(parser, nullptr, module);
	Vector<Operation *> block;
	impl.current_block = &block;
	impl.execution_model = stage;
	check(!impl.options.supports_ray_tracing_linear_swept_spheres);
	impl.options.supports_ray_tracing_linear_swept_spheres = supported;
	auto &builder = impl.builder();
	auto *uint_type = llvm::Type::getInt32Ty(context);
	llvm::ConstantInt nv_opcode(uint_type, opcode);
	llvm::ConstantInt dx_opcode(uint_type, flags_mode == 1 ? 0 : unsigned(DXIL::Op::RayQuery_RayFlags));
	llvm::FunctionType function_type(context, uint_type, { uint_type, uint_type });
	llvm::ConstantInt alloc_opcode(uint_type, unsigned(DXIL::Op::AllocateRayQuery)), alloc_flags(uint_type, 0);
	llvm::CallInst handle(&function_type, nullptr, { &alloc_opcode, &alloc_flags });
	llvm::Function function(&function_type, 0, llvm_module);
	llvm_module.add_value_name(0, "dx.op.rayQuery_RayFlags.i32");
	llvm::CallInst flags(&function_type, &function, { &dx_opcode, &handle });
	llvm::CallInst short_flags(&function_type, &function, { &dx_opcode });
	llvm::CallInst anonymous_flags(&function_type, nullptr, { &dx_opcode, &handle });
	impl.nvapi.fake_doorbell_inputs[NVAPI_ARGUMENT_OPCODE] = &nv_opcode;
	impl.nvapi.fake_doorbell_inputs[NVAPI_ARGUMENT_SRC0U] = flags_mode == 2 ? static_cast<llvm::Value *>(&handle) :
	    flags_mode == 3 ? static_cast<llvm::Value *>(&short_flags) :
	    flags_mode == 4 ? static_cast<llvm::Value *>(&anonymous_flags) : static_cast<llvm::Value *>(&flags);
	check(impl.nvapi.can_commit_opcode());

	spv::Id query = builder.createVariable(spv::StorageClassPrivate, builder.makeRayQueryType());
	if (query_mode)
	{
		impl.ray_query.global_query_objects_id = query;
		if (query_mode == 2)
		{
			impl.ray_query.global_query_objects_id = builder.createVariable(spv::StorageClassPrivate,
			    builder.makeArrayType(builder.makeRayQueryType(), builder.makeUintConstant(2), 0));
			impl.shader_analysis.ray_query.uses_divergent_handles = true;
			impl.rewrite_value(&handle, builder.makeUintConstant(1));
		}
	}
	else
		impl.rewrite_value(&handle, query);

	bool valid_stage = stage == spv::ExecutionModelAnyHitKHR || stage == spv::ExecutionModelClosestHitKHR;
	bool expected = supported && (analysis || (opcode == 104 ? valid_stage : flags_mode == 0));
	check(impl.nvapi.commit_opcode(impl, analysis) == expected);
	check(builder.hasCapability(spv::CapabilityRayTracingLinearSweptSpheresGeometryNV) == (expected && !analysis));
	Vector<uint32_t> declarations;
	builder.dump(declarations);
	bool extension = false;
	for (size_t i = 5; i < declarations.size(); i += declarations[i] >> 16)
		if (spv::Op(declarations[i] & 0xffff) == spv::OpExtension &&
		    std::strcmp(reinterpret_cast<const char *>(&declarations[i + 1]), "SPV_NV_linear_swept_spheres") == 0)
			extension = true;
	check(extension == (expected && !analysis));
	if (!expected || analysis)
	{
		check(block.empty());
		for (auto id : impl.nvapi.fake_doorbell_outputs)
			check(id == 0);
		return;
	}

	check(impl.nvapi.num_expected_clock_outputs == (opcode == 114 ? 1u : 8u));
	unsigned first = 0;
	if (opcode != 104 && query_mode == 2)
	{
		check(block[0]->op == spv::OpInBoundsAccessChain);
		check(block[0]->argument(0) == impl.ray_query.global_query_objects_id);
		check(builder.getConstantScalar(block[0]->argument(1)) == 1);
		query = block[0]->id;
		first = 1;
	}
	if (opcode == 114)
	{
		check(block.size() == first + 2);
		check(block[first]->op == spv::OpRayQueryIsLSSHitNV);
		check(block[first]->argument(0) == query);
		check(builder.getConstantScalar(block[first]->argument(1)) == 1);
		check(block[first]->type_id == builder.makeBoolType());
		check(block[first + 1]->op == spv::OpSelect);
		check(block[first + 1]->argument(0) == block[first]->id);
		check(builder.getConstantScalar(block[first + 1]->argument(1)) == 1);
		check(builder.getConstantScalar(block[first + 1]->argument(2)) == 0);
		check(impl.nvapi.fake_doorbell_outputs[0] == block.back()->id);
	}
	else
	{
		check(block.size() == first + 18);
		check(block[first]->op == (opcode == 104 ? spv::OpLoad : spv::OpRayQueryGetIntersectionLSSPositionsNV));
		check(block[first + 1]->op == (opcode == 104 ? spv::OpLoad : spv::OpRayQueryGetIntersectionLSSRadiiNV));
		auto float_type = builder.makeFloatType(32);
		check(block[first]->type_id == builder.makeArrayType(builder.makeVectorType(float_type, 3),
		    builder.makeUintConstant(2), 0));
		check(block[first + 1]->type_id == builder.makeArrayType(float_type, builder.makeUintConstant(2), 0));
		if (opcode == 104)
		{
			check(block[first]->argument(0) == module.get_builtin_shader_input(spv::BuiltInHitLSSPositionsNV));
			check(block[first + 1]->argument(0) == module.get_builtin_shader_input(spv::BuiltInHitLSSRadiiNV));
		}
		else
			for (unsigned i = 0; i < 2; i++)
			{
				check(block[first + i]->argument(0) == query);
				check(builder.getConstantScalar(block[first + i]->argument(1)) == 1);
			}
		for (unsigned i = 0; i < 8; i++)
		{
			auto *extract = block[first + 2 + 2 * i];
			auto *cast = block[first + 3 + 2 * i];
			check(extract->op == spv::OpCompositeExtract);
			check(extract->argument(0) == block[first + (i % 4 == 3 ? 1 : 0)]->id);
			check(extract->argument(1) == i / 4);
			if (i % 4 != 3)
				check(extract->argument(2) == i % 4);
			check(cast->op == spv::OpBitcast && cast->argument(0) == extract->id);
			check(cast->type_id == builder.makeUintType(32));
			check(impl.nvapi.fake_doorbell_outputs[i] == cast->id);
		}
	}
}

static void test_raw_atomic(bool half, bool physical, bool non_uniform, unsigned offset_mode, unsigned address)
{
	llvm::LLVMContext context;
	LLVMBCParser parser;
	SPIRVModule module;
	Converter::Impl impl(parser, nullptr, module);
	Vector<Operation *> block;
	impl.current_block = &block;
	auto &builder = impl.builder();
	auto *type = llvm::Type::getInt32Ty(context);
	llvm::ConstantInt opcode(type, half ? 12 : 13), operation(type, 3), addr(type, address), value(type, 0);
	llvm::Argument handle(type, 0);
	auto uint_type = builder.makeUintType(32);
	auto storage = physical ? spv::StorageClassPhysicalStorageBuffer : spv::StorageClassStorageBuffer;
	spv::Id id = physical ? builder.makeNullConstant(builder.makeVectorType(uint_type, 2)) :
	    builder.createVariable(storage, builder.makeStructType({ builder.makeRuntimeArray(uint_type) }, "raw"));
	impl.rewrite_value(&handle, id);
	auto &meta = impl.handle_to_resource_meta[id];
	meta = {};
	meta.storage = storage;
	meta.kind = DXIL::ResourceKind::RawBuffer;
	meta.var_id = id;
	meta.non_uniform = non_uniform;
	meta.aliased = offset_mode == 2;
	if (offset_mode)
	{
		// Offset 8 bytes, size 12 bytes, pre-shifted only for non-aliased views.
		spv::Id bounds[] = { builder.makeUintConstant(meta.aliased ? 8 : 2),
		                     builder.makeUintConstant(meta.aliased ? 12 : 3) };
		meta.index_offset_id = impl.build_constant_vector(uint_type, bounds, 2);
	}
	impl.nvapi.marked_uav = &handle;
	impl.nvapi.fake_doorbell_inputs[NVAPI_ARGUMENT_OPCODE] = &opcode;
	impl.nvapi.fake_doorbell_inputs[NVAPI_ARGUMENT_SRC0U] = &addr;
	impl.nvapi.fake_doorbell_inputs[NVAPI_ARGUMENT_SRC1U] = &value;
	impl.nvapi.fake_doorbell_inputs[NVAPI_ARGUMENT_SRC2U] = &operation;
	check(impl.nvapi.commit_opcode(impl, false));

	std::unordered_map<spv::Id, Vector<uint32_t>> values;
	Vector<uint32_t> declarations;
	builder.dump(declarations);
	for (size_t i = 5; i < declarations.size(); i += declarations[i] >> 16)
	{
		auto op = spv::Op(declarations[i] & 0xffff);
		if (op == spv::OpConstant)
			values[declarations[i + 2]] = { declarations[i + 3] };
		else if (op == spv::OpConstantComposite)
			for (unsigned j = 3; j < (declarations[i] >> 16); j++)
				values[declarations[i + 2]].push_back(values.at(declarations[i + j])[0]);
	}
	Operation *pointer = nullptr;
	spv::Id atomic_pointer = 0;
	for (auto *op : block)
	{
		switch (op->op)
		{
		case spv::OpShiftRightLogical:
			for (size_t i = 0; i < values.at(op->argument(0)).size(); i++)
				values[op->id].push_back(values.at(op->argument(0))[i] >> values.at(op->argument(1))[i]);
			break;
		case spv::OpCompositeExtract:
			values[op->id] = { values.at(op->argument(0))[op->argument(1)] }; break;
		case spv::OpIAdd:
			values[op->id] = { values.at(op->argument(0))[0] + values.at(op->argument(1))[0] }; break;
		case spv::OpULessThan:
			values[op->id] = { uint32_t(values.at(op->argument(0))[0] < values.at(op->argument(1))[0]) }; break;
		case spv::OpSelect:
			values[op->id] = values.at(op->argument(values.at(op->argument(0))[0] ? 1 : 2)); break;
		case spv::OpAccessChain:
		case spv::OpUntypedAccessChainKHR:
			pointer = op; break;
		case spv::OpAtomicFAddEXT:
			atomic_pointer = op->argument(0); break;
		default: break;
		}
	}
	check(pointer && atomic_pointer == pointer->id);
	check(builder.hasDecoration(pointer->id, spv::DecorationNonUniform) == non_uniform);
	auto index = values.at(pointer->argument(physical ? 2 : 3))[0];
	unsigned expected = address / 4;
	if (offset_mode) expected = expected < 3 ? expected + 2 : 0x3ffffffc;
	check(index == expected);
}

static void test_lss_store_failure(bool raw, unsigned failure)
{
	llvm::LLVMContext context;
	LLVMBCParser parser;
	SPIRVModule module;
	Converter::Impl impl(parser, nullptr, module);
	Vector<Operation *> block;
	impl.current_block = &block;
	impl.options.supports_ray_tracing_linear_swept_spheres = failure != 0;
	impl.execution_model = failure == 1 ? spv::ExecutionModelGLCompute : spv::ExecutionModelClosestHitKHR;
	auto *type = llvm::Type::getInt32Ty(context);
	llvm::ConstantInt dx_opcode(type, unsigned(raw ? DXIL::Op::RawBufferStore : DXIL::Op::BufferStore));
	llvm::ConstantInt nv_opcode(type, failure == 2 ? 114 : 104), zero(type, 0), mask(type, 1);
	llvm::Argument handle(type, 0), counter(type, 1), not_ray_flags(type, 2);
	impl.nvapi.magic_ptr_id = impl.builder().makeUintConstant(1);
	impl.rewrite_value(&handle, impl.nvapi.magic_ptr_id);
	impl.nvapi.doorbell = &counter;
	if (failure == 2)
		impl.nvapi.fake_doorbell_inputs[NVAPI_ARGUMENT_SRC0U] = &not_ray_flags;
	Vector<llvm::Type *> types(raw ? 10 : 9, type);
	llvm::FunctionType function_type(context, llvm::Type::getVoidTy(context), std::move(types));
	Vector<llvm::Value *> args = { &dx_opcode, &handle, &counter, &zero, &nv_opcode,
	                              &zero, &zero, &zero, &mask };
	if (raw)
		args.push_back(&zero);
	llvm::CallInst store(&function_type, nullptr, std::move(args));
	// Rejection must reach the conversion caller, not fall through to a dummy UAV store.
	check(!(raw ? emit_raw_buffer_store_instruction(impl, &store, false) :
	              emit_buffer_store_instruction(impl, &store, false)));
	check(block.empty());
	for (auto id : impl.nvapi.fake_doorbell_outputs)
		check(id == 0);
	check(!impl.builder().hasCapability(spv::CapabilityRayTracingLinearSweptSpheresGeometryNV));
}

int main()
{
	begin_thread_allocator_context();
	for (bool lss : { false, true })
	{
		test_instance(false, lss);
		test_instance(true, lss);
	}
	for (unsigned opcode : { 104u, 106u, 114u })
	for (bool supported : { false, true })
	for (bool analysis : { false, true })
	for (auto stage : { spv::ExecutionModelClosestHitKHR, spv::ExecutionModelAnyHitKHR,
	                    spv::ExecutionModelRayGenerationKHR, spv::ExecutionModelGLCompute })
	for (unsigned query = 0; query < 3; query++)
	for (unsigned flags = 0; flags < 5; flags++)
		test_lss(opcode, supported, analysis, stage, query, flags);
	for (bool raw : { false, true })
	for (unsigned failure = 0; failure < 3; failure++)
		test_lss_store_failure(raw, failure);
	for (bool half : { false, true })
	for (bool physical : { false, true })
	for (bool non_uniform : { false, true })
	for (unsigned offset = 0; offset < (physical ? 1u : 3u); offset++)
	for (unsigned address : { 0u, 4u, 8u, 12u, 16u })
		test_raw_atomic(half, physical, non_uniform, offset, address);
	end_thread_allocator_context();
	std::puts("NVAPI codegen cases passed.");
}
