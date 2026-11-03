/* Copyright (c) 2026 Erhan Bilgili
 * SPDX-License-Identifier: MIT
 */

#include "opcodes/converter_impl.hpp"
#include "opcodes/dxil/dxil_common.hpp"
#include "opcodes/dxil/dxil_compute.hpp"
#include "context.hpp"
#include "node.hpp"
#include "node_pool.hpp"
#include "spirv-tools/libspirv.hpp"
#include <cstdio>
#include <cstdlib>

using namespace dxil_spv;

static void check_at(bool condition, unsigned line)
{
	if (!condition)
	{
		std::fprintf(stderr, "UAV coherence check failed at line %u.\n", line);
		std::exit(1);
	}
}
#define check(x) check_at((x), __LINE__)

struct HeapRemapper : ResourceRemappingInterface
{
	bool remap_uav(const D3DUAVBinding &, VulkanUAVBinding &binding) override
	{
		binding.buffer_binding = {};
		binding.buffer_binding.descriptor_type = VulkanDescriptorType::TexelBuffer;
		binding.buffer_binding.bindless.use_heap = true;
		return true;
	}
	bool remap_srv(const D3DBinding &, VulkanSRVBinding &) override { return false; }
	bool remap_sampler(const D3DBinding &, VulkanBinding &) override { return false; }
	bool remap_cbv(const D3DBinding &, VulkanCBVBinding &) override { return false; }
	bool remap_vertex_input(const D3DStageIO &, VulkanStageIO &) override { return false; }
	bool remap_stream_output(const D3DStreamOutput &, VulkanStreamOutput &) override { return false; }
	bool remap_stage_input(const D3DStageIO &, VulkanStageIO &) override { return false; }
	bool remap_stage_output(const D3DStageIO &, VulkanStageIO &) override { return false; }
	unsigned get_root_constant_word_count() override { return 0; }
	unsigned get_root_descriptor_count() override { return 0; }
	bool has_nontrivial_stage_input_remapping() override { return false; }
};

static void test_resource(spv::ExecutionModel stage, bool heap, bool coherent, bool barrier)
{
	llvm::LLVMContext context;
	LLVMBCParser parser;
	SPIRVModule module;
	module.set_override_spirv_version(0x10600);
	module.emit_entry_point(stage, "main", false, spv::MemoryModelVulkan);
	Converter::Impl impl(parser, nullptr, module);
	impl.execution_model = stage;
	impl.execution_mode_meta.memory_model = spv::MemoryModelVulkan;
	impl.shader_analysis.require_uav_thread_group_coherence = barrier;
	CFGNodePool pool;
	auto *node = pool.create_node();
	impl.current_block = &node->ir.operations;
	node->ir.terminator.type = Terminator::Type::Return;
	auto &builder = impl.builder();
	if (stage == spv::ExecutionModelGLCompute)
		builder.addExecutionMode(module.get_entry_function(), spv::ExecutionModeLocalSize, 1, 1, 1);

	AccessTracking tracking;
	tracking.has_read = tracking.has_written = true;
	tracking.add_accessed_vecsize(RawType::Integer, RawWidth::B32, 1);
	auto *u32 = llvm::Type::getInt32Ty(context);
	HeapRemapper remapper;
	const Converter::Impl::ResourceReference *reference;
	if (heap)
	{
		impl.resource_mapping_iface = &remapper;
		auto &annotation = impl.llvm_annotate_handle_uses[nullptr];
		annotation = {};
		annotation.resource_kind = DXIL::ResourceKind::RawBuffer;
		annotation.resource_type = DXIL::ResourceType::UAV;
		annotation.component_type = DXIL::ComponentType::U32;
		annotation.coherent = coherent;
		annotation.tracking = tracking;
		check(impl.emit_global_heaps());
		reference = &annotation.reference;
	}
	else
	{
		llvm::ConstantInt zero(u32, 0), one(u32, 1), kind(u32, unsigned(DXIL::ResourceKind::RawBuffer));
		llvm::ConstantInt coherent_value(u32, coherent);
		llvm::ConstantAsMetadata md_zero(nullptr, &zero), md_one(nullptr, &one), md_kind(nullptr, &kind);
		llvm::ConstantAsMetadata md_coherent(nullptr, &coherent_value);
		llvm::MDOperand none(nullptr);
		llvm::MDString name(nullptr, "buffer");
		llvm::MDNode declaration(nullptr, { &md_zero, &none, &name, &md_zero, &md_zero, &md_one,
		    &md_kind, &md_coherent, &md_zero, &md_zero });
		llvm::MDNode declarations(nullptr, { &declaration });
		impl.uav_access_tracking[0] = tracking;
		check(impl.emit_uavs(&declarations, nullptr));
		reference = &impl.uav_index_to_reference[0];
	}

	auto uint_type = builder.makeUintType(32);
	auto image_var = reference->var_id;
	auto image_type = builder.getDerefTypeId(image_var);
	if (heap)
	{
		image_type = builder.getContainedTypeId(image_type);
		auto *chain = impl.allocate(spv::OpAccessChain, builder.makePointer(spv::StorageClassUniformConstant, image_type));
		chain->add_ids({ image_var, builder.makeUintConstant(0) });
		impl.add(chain);
		image_var = chain->id;
	}
	auto *image = impl.allocate(spv::OpLoad, image_type);
	image->add_id(image_var);
	impl.add(image);
	auto *read = impl.allocate(spv::OpImageRead, builder.makeVectorType(uint_type, 4));
	read->add_ids({ image->id, builder.makeUintConstant(0) });
	add_vkmm_access_qualifiers(impl, read, reference->vkmm);
	impl.add(read);
	if (barrier)
	{
		llvm::ConstantInt opcode(u32, unsigned(DXIL::Op::Barrier)), flags(u32, DXIL::AccessUAVGlobal);
		llvm::FunctionType type(context, llvm::Type::getVoidTy(context), { u32, u32 });
		llvm::CallInst call(&type, nullptr, { &opcode, &flags });
		check(emit_barrier_instruction(impl, &call));
	}
	auto *write = impl.allocate(spv::OpImageWrite);
	write->add_ids({ image->id, builder.makeUintConstant(1), read->id });
	add_vkmm_access_qualifiers(impl, write, reference->vkmm);
	impl.add(write);

	CFGStructurizer structurizer(node, pool, module);
	check(structurizer.run_trivial());
	module.emit_entry_point_function_body(structurizer);
	Vector<uint32_t> spirv;
	check(module.finalize_spirv(spirv));
	spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_3);
	tools.SetMessageConsumer([](spv_message_level_t, const char *, const spv_position_t &, const char *message) {
		std::fprintf(stderr, "%s\n", message);
	});
	check(tools.Validate(spirv.data(), spirv.size()));
	bool local = barrier && stage == spv::ExecutionModelGLCompute && !coherent;
	check(reference->vkmm.non_private);
	check(reference->vkmm.auto_visibility == (coherent || local));
	if (reference->vkmm.auto_visibility)
		check(reference->vkmm.scope == (local ? spv::ScopeWorkgroup : spv::ScopeQueueFamily));
}

int main()
{
	begin_thread_allocator_context();
	for (auto stage : { spv::ExecutionModelFragment, spv::ExecutionModelVertex, spv::ExecutionModelGLCompute })
	for (bool heap : { false, true })
	for (bool coherent : { false, true })
	for (bool barrier : { false, true })
		test_resource(stage, heap, coherent, barrier);
	end_thread_allocator_context();
	std::puts("24 UAV coherence cases passed.");
}
