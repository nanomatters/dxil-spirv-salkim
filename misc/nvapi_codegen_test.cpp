/* Copyright (c) 2026 Erhan Bilgili
 * SPDX-License-Identifier: MIT
 */

#include "opcodes/converter_impl.hpp"
#include "opcodes/dxil/dxil_nvapi.hpp"
#include "context.hpp"
#include <cstdio>
#include <cstdlib>
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

static void test_instance(bool custom_id)
{
	llvm::LLVMContext context;
	LLVMBCParser parser;
	SPIRVModule module;
	Converter::Impl impl(parser, nullptr, module);
	Vector<Operation *> block;
	impl.current_block = &block;
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

int main()
{
	begin_thread_allocator_context();
	test_instance(false);
	test_instance(true);
	for (bool half : { false, true })
	for (bool physical : { false, true })
	for (bool non_uniform : { false, true })
	for (unsigned offset = 0; offset < (physical ? 1u : 3u); offset++)
	for (unsigned address : { 0u, 4u, 8u, 12u, 16u })
		test_raw_atomic(half, physical, non_uniform, offset, address);
	end_thread_allocator_context();
	std::puts("NVAPI codegen cases passed.");
}
