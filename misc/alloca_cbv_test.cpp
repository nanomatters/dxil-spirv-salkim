/* Copyright (c) 2026 Erhan Bilgili
 * SPDX-License-Identifier: MIT
 */

#include "opcodes/converter_impl.hpp"
#include "opcodes/opcodes_llvm_builtins.hpp"
#include "context.hpp"
#include <cstdio>
#include <cstdlib>

using namespace dxil_spv;

static void test(const unsigned (&words)[6], bool forward, unsigned stride)
{
	llvm::LLVMContext context;
	llvm::Module llvm_module(context);
	LLVMBCParser parser;
	SPIRVModule module;
	Converter::Impl impl(parser, nullptr, module);
	auto *type = llvm::Type::getFloatTy(context);
	auto *uint_type = llvm::Type::getInt32Ty(context);
	auto *array = llvm::ArrayType::get(type, 6);
	auto *pointer = llvm::PointerType::get(type, 0);
	llvm::ConstantInt zero(uint_type, 0), one(uint_type, 1);
	llvm::AllocaInst allocation(llvm::PointerType::get(array, 0), array, &one);
	llvm::Argument handle(uint_type, 0);
	auto *result = llvm::StructType::get(context, { type, type, type, type }, "cbuffer");
	llvm::FunctionType function(context, result, { uint_type, uint_type, uint_type });
	llvm::Function load(&function, 1, llvm_module);
	llvm_module.add_value_name(1, "dx.op.cbufferLoadLegacy.f32");
	llvm::ConstantInt opcode(uint_type, unsigned(DXIL::Op::CBufferLoadLegacy));
	if (!analyze_alloca_instruction(impl, &allocation)) std::abort();
	for (unsigned i = 0; i < 6; i++)
	{
		auto *index = context.construct<llvm::ConstantInt>(uint_type, i);
		auto *row = context.construct<llvm::ConstantInt>(uint_type, words[i] / 4);
		auto *call = context.construct<llvm::CallInst>(&function, &load,
		    Vector<llvm::Value *>{ &opcode, &handle, row });
		auto *extract = context.construct<llvm::ExtractValueInst>(type, call, Vector<unsigned>{ words[i] % 4 });
		auto *gep = context.construct<llvm::GetElementPtrInst>(pointer,
		    Vector<llvm::Value *>{ &allocation, &zero, index }, true);
		llvm::StoreInst store(gep, extract);
		if (!analyze_store_instruction(impl, &store)) std::abort();
	}
	auto itr = impl.alloca_tracking.find(&allocation);
	if ((itr != impl.alloca_tracking.end()) != forward ||
	    (forward && itr->second.stride != stride))
	{
		std::fprintf(stderr, "Incorrect CBV forwarding decision.\n");
		std::exit(1);
	}
}

int main()
{
	begin_thread_allocator_context();
	test({44, 45, 46, 47, 49, 49}, false, 0);
	test({44, 45, 46, 47, 48, 49}, true, 1);
	test({44, 46, 48, 50, 52, 54}, true, 2);
	test({44, 46, 49, 50, 52, 54}, false, 0);
	end_thread_allocator_context();
	std::puts("4 CBV forwarding cases passed.");
}
