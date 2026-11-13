/* Copyright (c) 2026 Erhan Bilgili
 * SPDX-License-Identifier: MIT
 */

#include "cfg_structurizer.hpp"
#include "node.hpp"
#include "node_pool.hpp"
#include "spirv_module.hpp"
#include "SpvBuilder.h"
#include "spirv-tools/libspirv.hpp"
#include <cstdio>
#include <cstdlib>
#include <unordered_set>

using namespace dxil_spv;

static void check(bool condition, const char *message)
{
	if (!condition)
	{
		std::fprintf(stderr, "%s\n", message);
		std::exit(1);
	}
}

static void test_rematerialization(unsigned consumers, bool non_uniform)
{
	CFGNodePool pool;
	SPIRVModule module;
	auto &builder = module.get_builder();
	module.emit_entry_point(spv::ExecutionModelGLCompute, "main", false, spv::MemoryModelGLSL450);
	builder.addExecutionMode(module.get_entry_function(), spv::ExecutionModeLocalSize, 1, 1, 1);

	auto f32 = builder.makeFloatType(32);
	auto image_type = builder.makeImageType(f32, spv::Dim2D, false, false, false, 1, spv::ImageFormatUnknown);
	auto sampler_type = builder.makeSamplerType();
	auto sampled_type = builder.makeSampledImageType(image_type);
	auto image = builder.createVariable(spv::StorageClassUniformConstant, image_type, "image");
	auto sampler = builder.createVariable(spv::StorageClassUniformConstant, sampler_type, "sampler");
	builder.addDecoration(image, spv::DecorationDescriptorSet, 0);
	builder.addDecoration(image, spv::DecorationBinding, 0);
	builder.addDecoration(sampler, spv::DecorationDescriptorSet, 0);
	builder.addDecoration(sampler, spv::DecorationBinding, 1);

	auto make_node = [&](const char *name) {
		auto *node = pool.create_node();
		node->name = name;
		node->ir.terminator.type = Terminator::Type::Return;
		return node;
	};
	auto *entry = make_node("entry");
	auto *left = make_node("left");
	auto *right = make_node("right");
	auto *merge = make_node("merge");
	entry->add_branch(left);
	entry->add_branch(right);
	entry->ir.terminator.type = Terminator::Type::Condition;
	entry->ir.terminator.true_block = left;
	entry->ir.terminator.false_block = right;
	entry->ir.terminator.conditional_id = builder.createUndefinedConstant(builder.makeBoolType());
	for (auto *node : { left, right })
	{
		node->add_branch(merge);
		node->ir.terminator.type = Terminator::Type::Branch;
		node->ir.terminator.direct_block = merge;
	}

	auto load = [&](spv::Id type, spv::Id variable) {
		auto *op = module.allocate_op(spv::OpLoad, module.allocate_id(), type);
		op->add_id(variable);
		entry->ir.operations.push_back(op);
		return op->id;
	};
	auto image_id = load(image_type, image);
	auto sampler_id = load(sampler_type, sampler);
	auto *sampled = module.allocate_op(spv::OpSampledImage, module.allocate_id(), sampled_type);
	sampled->add_id(image_id);
	sampled->add_id(sampler_id);
	entry->ir.operations.push_back(sampled);
	if (non_uniform)
	{
		builder.addExtension("SPV_EXT_descriptor_indexing");
		builder.addCapability(spv::CapabilityShaderNonUniformEXT);
		builder.addDecoration(sampled->id, spv::DecorationNonUniform);
	}

	for (unsigned i = 0; i < consumers; i++)
	{
		auto *op = module.allocate_op(spv::OpImage, module.allocate_id(), image_type);
		op->add_id(sampled->id);
		(i == 0 ? left : right)->ir.operations.push_back(op);
	}

	CFGStructurizer cfg(entry, pool, module);
	check(cfg.run_structurizer(), "Structurization failed.");
	pool.for_each_node([](CFGNode &node) { node.id = 0; node.userdata = nullptr; });
	module.emit_entry_point_function_body(cfg);
	Vector<uint32_t> binary;
	check(module.finalize_spirv(binary), "SPIR-V finalization failed.");
	spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_1);
	tools.SetMessageConsumer([](spv_message_level_t, const char *, const spv_position_t &, const char *message) {
		std::fprintf(stderr, "%s\n", message);
	});
	check(tools.Validate(binary.data(), binary.size()), "Rematerialized SPIR-V is invalid.");

	std::unordered_set<spv::Id> decorated, sampled_ids, consumed_ids;
	unsigned uses = 0;
	for (size_t i = 5; i < binary.size();)
	{
		unsigned count = binary[i] >> 16;
		auto opcode = spv::Op(binary[i] & 0xffff);
		check(count && i + count <= binary.size(), "Invalid SPIR-V instruction.");
		if (opcode == spv::OpDecorate && binary[i + 2] == spv::DecorationNonUniform)
			decorated.insert(binary[i + 1]);
		else if (opcode == spv::OpSampledImage)
			check(sampled_ids.insert(binary[i + 2]).second, "Duplicate sampled-image result ID.");
		else if (opcode == spv::OpImage)
		{
			check(consumed_ids.insert(binary[i + 3]).second, "Consumers share a sampled-image result ID.");
			uses++;
		}
		i += count;
	}
	check(uses == consumers, "A sampled-image consumer was lost.");
	for (auto id : consumed_ids)
	{
		check(sampled_ids.count(id), "Missing rematerialized sampled image.");
		check(bool(decorated.count(id)) == non_uniform, "NonUniform decoration changed.");
	}
	std::printf("sampled-image consumers=%u non-uniform=%u passed.\n", consumers, unsigned(non_uniform));
}

int main()
{
	begin_thread_allocator_context();
	for (unsigned consumers : { 1u, 2u })
		for (bool non_uniform : { false, true })
			test_rematerialization(consumers, non_uniform);
	end_thread_allocator_context();
	return 0;
}
