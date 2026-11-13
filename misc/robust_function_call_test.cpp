/* Copyright (c) 2026 Erhan Bilgili
 * SPDX-License-Identifier: MIT
 */

#include "opcodes/converter_impl.hpp"
#include "opcodes/opcodes_llvm_builtins.hpp"
#include "context.hpp"
#include "node.hpp"
#include "node_pool.hpp"
#include "spirv-tools/libspirv.hpp"
#include <cstdio>
#include <cstdlib>
#include <unordered_map>
#include <vector>

using namespace dxil_spv;

static void require(bool condition, const char *message)
{
	if (!condition)
	{
		std::fprintf(stderr, "Function pointer robustness check failed: %s\n", message);
		std::exit(1);
	}
}

struct Address
{
	spv::Id variable;
	unsigned index;
};

struct Interpreter
{
	SPIRVModule &module;
	unsigned width;
	std::unordered_map<spv::Id, std::vector<uint32_t>> values;
	std::unordered_map<spv::Id, Address> addresses;
	std::unordered_map<spv::Id, std::vector<std::vector<uint32_t>>> memory;
	std::vector<std::vector<uint32_t>> call_inputs;

	std::vector<uint32_t> value(spv::Id id)
	{
		auto itr = values.find(id);
		if (itr != values.end())
			return itr->second;
		auto *inst = module.get_builder().getInstruction(id);
		require(inst != nullptr, "missing value");
		if (inst->getOpCode() == spv::OpConstantNull)
			return std::vector<uint32_t>(width);
		require(inst->getOpCode() == spv::OpConstant, "unexpected constant");
		return { inst->getImmediateOperand(0) };
	}

	Address address(spv::Id id)
	{
		auto itr = addresses.find(id);
		return itr != addresses.end() ? itr->second : Address{ id, 0 };
	}

	std::vector<uint32_t> &location(spv::Id id)
	{
		auto ptr = address(id);
		auto itr = memory.find(ptr.variable);
		if (itr == memory.end())
			itr = memory.emplace(ptr.variable, std::vector<std::vector<uint32_t>>(1, std::vector<uint32_t>(width))).first;
		require(ptr.index < itr->second.size(), "out-of-bounds memory access escaped its guard");
		return itr->second[ptr.index];
	}

	void run(const Vector<Operation *> &operations)
	{
		for (auto *op : operations)
		{
			if (op->op == spv::OpULessThan)
				values[op->id] = { value(op->argument(0))[0] < value(op->argument(1))[0] };
			else if (op->op == spv::OpAccessChain || op->op == spv::OpInBoundsAccessChain)
				addresses[op->id] = { op->argument(0), value(op->argument(1))[0] };
			else if (op->op == spv::OpStore)
				location(op->argument(0)) = value(op->argument(1));
			else if (op->op == spv::OpCopyMemory || op->op == spv::PseudoOpMaskedCopyMemory)
			{
				if (op->op == spv::OpCopyMemory || value(op->argument(op->num_arguments() - 1))[0])
					location(op->argument(0)) = location(op->argument(1));
			}
			else if (op->op == spv::OpFunctionCall)
			{
				auto &data = location(op->argument(1));
				call_inputs.push_back(data);
				data[0] += 10;
			}
			else
				require(false, "unexpected emitted opcode");
		}
	}
};

static void test_call(unsigned kind, bool robust, unsigned index, bool direct)
{
	llvm::LLVMContext context;
	llvm::Module llvm_module(context);
	LLVMBCParser parser;
	SPIRVModule module;
	module.emit_entry_point(spv::ExecutionModelGLCompute, "main", false, spv::MemoryModelVulkan);
	Converter::Impl impl(parser, nullptr, module);
	impl.execution_model = spv::ExecutionModelGLCompute;
	impl.execution_mode_meta.memory_model = spv::MemoryModelVulkan;
	impl.options.extended_robustness.alloca = robust;
	auto &builder = module.get_builder();
	builder.addExecutionMode(module.get_entry_function(), spv::ExecutionModeLocalSize, 1, 1, 1);

	auto *u32 = llvm::Type::getInt32Ty(context);
	auto *type = kind == 1 ? static_cast<llvm::Type *>(llvm::VectorType::get(4, u32)) :
	             kind == 2 ? static_cast<llvm::Type *>(llvm::ArrayType::get(u32, 2)) : u32;
	unsigned width = kind == 1 ? 4 : kind == 2 ? 2 : 1;
	auto *pointer_type = llvm::PointerType::get(type, unsigned(DXIL::AddressSpace::Thread));
	llvm::FunctionType function_type(context, llvm::Type::getVoidTy(context), { pointer_type });
	llvm::Function function(&function_type, 1, llvm_module);
	llvm_module.add_value_name(1, "modify");
	llvm::Argument parameter(pointer_type, 0, true);
	function.add_argument(&parameter);

	spv::Id spv_type = impl.get_type_id(type);
	spv::Block *callee_entry;
	auto *callee = builder.makeFunctionEntry(spv::NoPrecision, builder.makeVoidType(), "modify",
	                                         { builder.makePointer(spv::StorageClassFunction, spv_type) }, {}, &callee_entry);
	auto *load = builder.addInstruction(spv_type, spv::OpLoad);
	load->addIdOperand(callee->getParamId(0));
	spv::Id first = load->getResultId();
	if (width != 1)
	{
		auto *extract = builder.addInstruction(builder.makeUintType(32), spv::OpCompositeExtract);
		extract->addIdOperand(first);
		extract->addImmediateOperand(0);
		first = extract->getResultId();
	}
	auto *sum = builder.addInstruction(builder.makeUintType(32), spv::OpIAdd);
	sum->addIdOperand(first);
	sum->addIdOperand(builder.makeUintConstant(10));
	spv::Id result = sum->getResultId();
	if (width != 1)
	{
		auto *insert = builder.addInstruction(spv_type, spv::OpCompositeInsert);
		insert->addIdOperand(result);
		insert->addIdOperand(load->getResultId());
		insert->addImmediateOperand(0);
		result = insert->getResultId();
	}
	builder.createStore(result, callee->getParamId(0));
	builder.makeReturn();
	impl.rewrite_value(&function, callee->getId());
	builder.setBuildPoint(module.get_entry_function()->getEntryBlock());

	auto *array_type = llvm::ArrayType::get(type, 4);
	auto *allocation_type = direct ? type : array_type;
	auto *allocation_pointer = llvm::PointerType::get(allocation_type, unsigned(DXIL::AddressSpace::Thread));
	llvm::ConstantInt zero(u32, 0), one(u32, 1);
	llvm::AllocaInst allocation(allocation_pointer, allocation_type, &one);
	spv::Id root = module.create_variable(spv::StorageClassFunction, impl.get_type_id(allocation_type));
	impl.rewrite_value(&allocation, root);
	impl.handle_to_storage_class[&allocation] = spv::StorageClassFunction;
	llvm::Argument dynamic_index(u32, 0, false);
	impl.rewrite_value(&dynamic_index, builder.makeUintConstant(index));
	llvm::GetElementPtrInst gep(pointer_type, { &allocation, &zero, &dynamic_index }, false);
	CFGNodePool pool;
	auto *node = pool.create_node();
	node->ir.terminator.type = Terminator::Type::Return;
	impl.current_block = &node->ir.operations;
	if (!direct)
	{
		require(emit_getelementptr_instruction(impl, &gep), "GEP emission failed");
		require(bool(impl.handle_to_robustness.count(&gep)) == robust, "GEP robustness setup failed");
	}
	impl.combined_image_sampler_cache.push_back({ 0, 0, 0, false });
	llvm::CallInst call(&function_type, &function, { direct ? static_cast<llvm::Value *>(&allocation) : &gep });
	require(emit_call_instruction(impl, call), "call emission failed");

	Interpreter interpreter{ module, width };
	interpreter.memory[root].resize(direct ? 1 : 4, std::vector<uint32_t>(width));
	for (unsigned i = 0; i < interpreter.memory[root].size(); i++)
		for (unsigned j = 0; j < width; j++)
			interpreter.memory[root][i][j] = 100 + 10 * i + j;
	auto original = interpreter.memory[root];
	bool valid = direct || index < 4;
	for (unsigned iteration = 0; iteration < 2; iteration++)
	{
		auto expected = valid ? interpreter.memory[root][direct ? 0 : index] : std::vector<uint32_t>(width);
		interpreter.run(node->ir.operations);
		require(interpreter.call_inputs.back() == expected, "callee did not receive the expected data");
		if (valid)
			require(interpreter.memory[root][direct ? 0 : index][0] == expected[0] + 10, "callee writes were not copied back");
		else
			require(interpreter.memory[root] == original, "out-of-bounds call changed source memory");
	}

	require(impl.combined_image_sampler_cache.empty() == (robust && !direct), "sampler cache does not match inserted control flow");

	CFGStructurizer cfg(node, pool, module);
	require(cfg.run_structurizer(), "structurization failed");
	module.emit_entry_point_function_body(cfg);
	Vector<uint32_t> spirv;
	require(module.finalize_spirv(spirv), "SPIR-V finalization failed");
	spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_3);
	require(tools.Validate(spirv.data(), spirv.size()), "SPIR-V validation failed");
	std::printf("kind=%u robust=%u index=%u direct=%u passed\n", kind, unsigned(robust), index, unsigned(direct));
}

int main()
{
	begin_thread_allocator_context();
	for (unsigned kind = 0; kind < 3; kind++)
	{
		test_call(kind, false, 1, false);
		test_call(kind, true, 99, false);
		test_call(kind, true, 1, false);
		test_call(kind, true, 0, true);
	}
	end_thread_allocator_context();
	return 0;
}
