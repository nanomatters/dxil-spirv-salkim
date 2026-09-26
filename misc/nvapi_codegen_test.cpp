/* Copyright (c) 2026 Erhan Bilgili
 * SPDX-License-Identifier: MIT
 */

#include "opcodes/converter_impl.hpp"
#include "opcodes/dxil/dxil_nvapi.hpp"
#include "context.hpp"
#include <cstdio>
#include <cstdlib>

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

int main()
{
	begin_thread_allocator_context();
	test_instance(false);
	test_instance(true);
	end_thread_allocator_context();
	std::puts("NVAPI codegen cases passed.");
}
