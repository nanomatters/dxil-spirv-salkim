/* Copyright (c) 2026 Erhan Bilgili
 * SPDX-License-Identifier: MIT
 */

#include "opcodes/converter_impl.hpp"
#include "opcodes/dxil/dxil_resources.hpp"
#include "context.hpp"
#include <cstdio>
#include <cstdlib>

using namespace dxil_spv;

static void check_at(bool condition, unsigned line)
{
	if (!condition)
	{
		std::fprintf(stderr, "Descriptor index check failed at line %u.\n", line);
		std::exit(1);
	}
}
#define check(condition) check_at((condition), __LINE__)

enum class IndexSource { Push, Inline, Record, Heap, Array };

static void test_handle(DXIL::ResourceType resource_type, IndexSource source,
                        bool non_uniform, bool representative, bool qa, bool robustness, bool counter = false)
{
	llvm::LLVMContext context;
	LLVMBCParser parser;
	SPIRVModule module;
	Converter::Impl impl(parser, nullptr, module);
	Vector<Operation *> block;
	impl.current_block = &block;
	impl.backend.skip_non_uniform_promotion = true;
	auto &builder = impl.builder();
	auto uint_type = builder.makeUintType(32);
	auto *type = llvm::Type::getInt32Ty(context);
	llvm::ConstantInt opcode(type, 57), resource_class(type, unsigned(resource_type)), range(type, 0);
	llvm::ConstantInt uniform(type, non_uniform);
	llvm::Argument index(type, 0);
	llvm::FunctionType function(context, type, { type, type, type, type, type });
	llvm::CallInst call(&function, nullptr, { &opcode, &resource_class, &range, &index, &uniform });
	llvm::CallInst other_call(&function, nullptr, { &opcode, &resource_class, &range, &index, &uniform });

	impl.srv_index_to_reference.resize(1);
	impl.uav_index_to_reference.resize(1);
	impl.cbv_index_to_reference.resize(1);
	impl.srv_index_to_offset.resize(1);
	impl.uav_index_to_offset.resize(1);
	auto &reference = resource_type == DXIL::ResourceType::SRV ? impl.srv_index_to_reference[0] :
	                  resource_type == DXIL::ResourceType::UAV ? impl.uav_index_to_reference[0] :
	                  impl.cbv_index_to_reference[0];
	reference.resource_kind = resource_type == DXIL::ResourceType::CBV ?
	                          DXIL::ResourceKind::CBuffer : DXIL::ResourceKind::RawBuffer;
	reference.base_resource_is_array = true;
	reference.bindless = source != IndexSource::Array;
	reference.base_offset = source == IndexSource::Heap ? 0 : 5;
	reference.push_constant_member = source == IndexSource::Heap ? UINT32_MAX : 0;
	reference.aliased = true;

	if (source == IndexSource::Push || source == IndexSource::Inline)
	{
		impl.root_constant_num_words = 4;
		impl.options.inline_ubo_enable = source == IndexSource::Inline;
		impl.root_constant_arrayed = source == IndexSource::Inline;
		auto storage = impl.options.inline_ubo_enable ? spv::StorageClassUniform : spv::StorageClassPushConstant;
		auto member = impl.root_constant_arrayed ? builder.makeArrayType(uint_type, builder.makeUintConstant(4), 4) : uint_type;
		impl.root_constant_id = builder.createVariable(storage, builder.makeStructType({ member }, "root"));
	}
	else if (source == IndexSource::Record)
	{
		reference.local_root_signature_entry = 0;
		impl.local_root_signature.resize(1);
		impl.local_root_signature[0].type = LocalRootSignatureType::Table;
		auto table = builder.makeVectorType(uint_type, 2);
		impl.shader_record_buffer_id = builder.createVariable(spv::StorageClassShaderRecordBufferKHR,
		    builder.makeStructType({ table }, "record"));
	}

	Vector<spv::Id> variables;
	for (unsigned i = 0; i < 4; i++)
	{
		auto element = i ? builder.makeVectorType(uint_type, i + 1) : uint_type;
		auto structure = builder.makeStructType({ builder.makeRuntimeArray(element) }, "buffer");
		auto storage = resource_type == DXIL::ResourceType::CBV ? spv::StorageClassUniform : spv::StorageClassStorageBuffer;
		auto variable = builder.createVariable(storage, builder.makeRuntimeArray(structure));
		variables.push_back(variable);
		auto &meta = impl.handle_to_resource_meta[variable];
		meta = {};
		meta.kind = reference.resource_kind;
		meta.storage = storage;
		if (!i && representative)
			reference.var_id = variable;
		else
			reference.var_alias_group.push_back({ {}, variable });
	}
	if (counter)
	{
		// Bindless SSBO counters use the parent's index, but a separate QA type.
		impl.uav_index_to_counter.resize(1);
		auto &counter_reference = impl.uav_index_to_counter[0];
		counter_reference.bindless = true;
		counter_reference.resource_kind = DXIL::ResourceKind::RawBuffer;
		auto structure = builder.makeStructType({ uint_type }, "counter");
		counter_reference.var_id = builder.createVariable(spv::StorageClassStorageBuffer,
		    builder.makeRuntimeArray(structure));
		variables.push_back(counter_reference.var_id);
		impl.handle_to_resource_meta[counter_reference.var_id].storage = spv::StorageClassStorageBuffer;
		impl.llvm_values_using_update_counter.insert(&call);
		impl.llvm_values_using_update_counter.insert(&other_call);
	}
	impl.options.descriptor_qa_enabled = qa;
	impl.options.descriptor_qa.version = Version;
	impl.options.descriptor_heap_robustness = robustness;
	if (robustness)
		impl.instrumentation.descriptor_heap_size_var_id = builder.createVariable(spv::StorageClassUniform,
		    builder.makeStructType({ uint_type }, "heap_size"));

	// Distinct calls and blocks must not inherit another handle's computed index.
	spv::Id previous_index = 0;
	Vector<Operation *> other_block;
	for (unsigned pass = 0; pass < 2; pass++)
	{
		auto &current_block = pass ? other_block : block;
		impl.current_block = &current_block;
		check(emit_create_handle_instruction(impl, pass ? &other_call : &call));
		unsigned table_loads = 0, descriptor_chains = 0, checks = 0, additions = 0;
		spv::Id table_pointer = 0, common_index = 0;
		Vector<spv::Id> checked_indices;
		for (const auto *op : current_block)
		{
			if (op->op == spv::OpAccessChain &&
			    (op->argument(0) == impl.root_constant_id || op->argument(0) == impl.shader_record_buffer_id))
				table_pointer = op->id;
			if (op->op == spv::OpLoad && op->argument(0) == table_pointer)
				table_loads++;
			if (op->op == spv::OpIAdd)
				additions++;
			if (op->op == spv::OpFunctionCall || (robustness && op->op == spv::OpExtInst))
			{
				spv::Id input = op->argument(op->op == spv::OpFunctionCall ? 1 : 2);
				if (!common_index) common_index = input;
				check(common_index == input);
				if (qa && counter && checks == 4)
					check(op->argument(2) == builder.makeUintConstant(DESCRIPTOR_QA_TYPE_RAW_VA_BIT));
				checked_indices.push_back(op->id);
				checks++;
			}
			if (op->op == spv::OpAccessChain)
			for (auto variable : variables)
			{
				if (op->argument(0) != variable) continue;
				spv::Id input = op->argument(1);
				if ((qa || robustness) && reference.bindless)
					check(input == checked_indices[descriptor_chains]);
				else
				{
					if (!common_index) common_index = input;
					check(input == common_index);
				}
				descriptor_chains++;
			}
		}
		bool has_table = source == IndexSource::Push || source == IndexSource::Inline || source == IndexSource::Record;
		check(table_loads == unsigned(has_table));
		check(descriptor_chains == 4 + unsigned(counter));
		check(additions == (has_table ? 2u : source == IndexSource::Array ? 1u : 0u));
		check(checks == ((qa || robustness) && reference.bindless ? 4u + unsigned(counter) : 0u));
		if (pass && source != IndexSource::Heap) check(common_index != previous_index);
		previous_index = common_index;
	}
	check(impl.descriptor_qa_counter == (qa && reference.bindless ? 2u * (4u + unsigned(counter)) : 0u));

	if (source == IndexSource::Push || source == IndexSource::Inline)
	{
		// An invalid table must fail before adding any descriptor access or QA call.
		reference.push_constant_member = impl.root_constant_num_words;
		impl.current_block = &block;
		block.clear();
		check(!emit_create_handle_instruction(impl, &call));
		check(block.empty());
	}
}

int main()
{
	begin_thread_allocator_context();
	for (auto type : { DXIL::ResourceType::SRV, DXIL::ResourceType::UAV, DXIL::ResourceType::CBV })
	for (auto source : { IndexSource::Push, IndexSource::Inline, IndexSource::Record, IndexSource::Heap, IndexSource::Array })
	for (bool non_uniform : { false, true })
	for (bool representative : { false, true })
	for (unsigned check_mode = 0; check_mode < 3; check_mode++)
		test_handle(type, source, non_uniform, representative, check_mode == 1, check_mode == 2);
	for (auto source : { IndexSource::Push, IndexSource::Inline, IndexSource::Record, IndexSource::Heap })
	for (bool non_uniform : { false, true })
	for (unsigned check_mode = 0; check_mode < 3; check_mode++)
		test_handle(DXIL::ResourceType::UAV, source, non_uniform, false, check_mode == 1, check_mode == 2, true);
	end_thread_allocator_context();
	std::puts("204 descriptor-index cases passed.");
}
