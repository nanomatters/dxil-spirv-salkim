/* Copyright (c) 2026 Erhan Bilgili
 * SPDX-License-Identifier: MIT
 */

#include "SpvBuilder.h"
#include "cfg_structurizer.hpp"
#include "node.hpp"
#include "node_pool.hpp"
#include "spirv-tools/libspirv.hpp"
#include "spirv-tools/optimizer.hpp"
#include "spirv_module.hpp"
#include <cstdio>
#include <cstdlib>
#include <unordered_map>
#include <vector>
using namespace dxil_spv;

static void require(bool condition, const char *message)
{
	if (!condition)
	{
		fprintf(stderr, "FAIL: %s\n", message);
		exit(1);
	}
}

struct Fixture
{
	CFGNodePool pool;
	SPIRVModule module;
	spv::Id uint_type, bool_type, value_type, output;
	explicit Fixture(bool vector = false)
	{
		auto &b = module.get_builder();
		module.emit_entry_point(spv::ExecutionModelGLCompute, "main", false, spv::MemoryModelGLSL450);
		b.addExecutionMode(module.get_entry_function(), spv::ExecutionModeLocalSize, 1, 1, 1);
		uint_type = b.makeUintType(32);
		bool_type = b.makeBoolType();
		value_type = vector ? b.makeVectorType(uint_type, 4) : uint_type;
		auto type = b.makeStructType({ value_type }, "Output");
		b.addDecoration(type, spv::DecorationBlock);
		b.addMemberDecoration(type, 0, spv::DecorationOffset, 0);
		output = b.createVariable(spv::StorageClassStorageBuffer, type, "out");
		b.addDecoration(output, spv::DecorationDescriptorSet, 0);
		b.addDecoration(output, spv::DecorationBinding, 0);
	}
	CFGNode *node(const char *name)
	{
		auto *n = pool.create_node();
		n->name = name;
		n->ir.terminator.type = Terminator::Type::Return;
		return n;
	}
	spv::Id constant(uint32_t x)
	{
		return module.get_builder().makeUintConstant(x);
	}
	void branch(CFGNode *a, CFGNode *z)
	{
		a->add_branch(z);
		a->ir.terminator.type = Terminator::Type::Branch;
		a->ir.terminator.direct_block = z;
	}
	void condition(CFGNode *a, CFGNode *t, CFGNode *f, spv::Id c)
	{
		a->add_branch(t);
		a->add_branch(f);
		a->ir.terminator.type = Terminator::Type::Condition;
		a->ir.terminator.true_block = t;
		a->ir.terminator.false_block = f;
		a->ir.terminator.conditional_id = c;
	}
	spv::Id binary(CFGNode *n, spv::Op opcode, spv::Id a, spv::Id b, spv::Id type = 0)
	{
		auto *o = module.allocate_op(opcode, module.allocate_id(), type ? type : uint_type);
		o->add_id(a);
		o->add_id(b);
		n->ir.operations.push_back(o);
		return o->id;
	}
	void write(CFGNode *n, spv::Id x)
	{
		auto *a = module.allocate_op(spv::OpAccessChain, module.allocate_id(),
		                             module.get_builder().makePointer(spv::StorageClassStorageBuffer, value_type));
		a->add_id(output);
		a->add_id(constant(0));
		n->ir.operations.push_back(a);
		auto *s = module.allocate_op(spv::OpStore);
		s->add_id(a->id);
		s->add_id(x);
		n->ir.operations.push_back(s);
	}
	void phi(CFGNode *n, spv::Id id, CFGNode *a, spv::Id av, CFGNode *b, spv::Id bv)
	{
		PHI p;
		p.id = id;
		p.type_id = value_type;
		p.incoming = { { a, av }, { b, bv } };
		n->ir.phi.push_back(p);
	}
};

// Interpret only the integer operations emitted by these small control-flow fixtures.
// Treat undefined data as poison so an accidental read in a defined recurrence fails.
struct Value
{
	uint32_t number = 0;
	bool defined = false;
};
struct Interpreter
{
	Fixture &f;
	std::unordered_map<spv::Id, Value> values, memory;
	std::unordered_map<spv::Id, spv::Id> pointers;
	std::vector<uint32_t> output;
	Value value(spv::Id id)
	{
		auto it = values.find(id);
		if (it != values.end())
			return it->second;
		auto *inst = f.module.get_builder().getInstruction(id);
		require(inst != nullptr, "unknown value");
		if (inst->getOpCode() == spv::OpUndef)
			return {};
		if (inst->getOpCode() == spv::OpConstant)
			return { inst->getImmediateOperand(0), true };
		if (inst->getOpCode() == spv::OpConstantTrue)
			return { 1, true };
		if (inst->getOpCode() == spv::OpConstantFalse)
			return { 0, true };
		require(false, "unsupported constant");
		return {};
	}
	uint32_t number(spv::Id id)
	{
		auto v = value(id);
		require(v.defined, "undefined value reached observable calculation");
		return v.number;
	}
	std::vector<uint32_t> run(CFGNode *node)
	{
		CFGNode *previous = nullptr;
		for (unsigned steps = 0; steps < 10000; steps++)
		{
			std::vector<std::pair<spv::Id, Value>> incoming;
			for (auto &phi : node->ir.phi)
			{
				bool found = false;
				for (auto &in : phi.incoming)
					if (in.block == previous)
					{
						incoming.push_back({ phi.id, value(in.id) });
						found = true;
						break;
					}
				require(found, "missing PHI predecessor");
			}
			for (auto &pair : incoming)
				values[pair.first] = pair.second;
			for (auto *op : node->ir.operations)
			{
				auto arg = [&](unsigned i) { return op->argument(i); };
				switch (op->op)
				{
				case spv::OpLoad:
					values[op->id] = memory[arg(0)];
					break;
				case spv::OpStore:
				{
					auto p = pointers.find(arg(0));
					if (p != pointers.end() && p->second == f.output)
						output.push_back(number(arg(1)));
					else
						memory[arg(0)] = value(arg(1));
					break;
				}
				case spv::OpAccessChain:
					pointers[op->id] = arg(0);
					break;
				case spv::OpIAdd:
					values[op->id] = { number(arg(0)) + number(arg(1)), true };
					break;
				case spv::OpIMul:
					values[op->id] = { number(arg(0)) * number(arg(1)), true };
					break;
				case spv::OpULessThan:
					values[op->id] = { number(arg(0)) < number(arg(1)), true };
					break;
				case spv::OpIEqual:
					values[op->id] = { number(arg(0)) == number(arg(1)), true };
					break;
				case spv::OpINotEqual:
					values[op->id] = { number(arg(0)) != number(arg(1)), true };
					break;
				case spv::OpLogicalNot:
					values[op->id] = { !number(arg(0)), true };
					break;
				case spv::OpSelect:
					values[op->id] = value(arg(number(arg(0)) ? 1 : 2));
					break;
				case spv::OpCopyObject:
					values[op->id] = value(arg(0));
					break;
				default:
					fprintf(stderr, "unsupported opcode %u\n", unsigned(op->op));
					exit(1);
				}
			}
			auto &t = node->ir.terminator;
			previous = node;
			if (t.type == Terminator::Type::Return)
				return output;
			if (t.type == Terminator::Type::Branch)
				node = t.direct_block;
			else if (t.type == Terminator::Type::Condition)
				node = number(t.conditional_id) ? t.true_block : t.false_block;
			else
			{
				require(false, "unsupported terminator");
			}
		}
		require(false, "loop did not terminate");
		return {};
	}
};

static unsigned count_undef_resets(Fixture &f, spv::Id expected = 0)
{
	unsigned resets = 0;
	f.pool.for_each_node([&](CFGNode &n) {
		for (auto *op : n.ir.operations)
			if (op->op == spv::OpStore)
				if (auto *value = f.module.get_builder().getInstruction(op->argument(1)))
					if (value->getOpCode() == spv::OpUndef)
					{
						require(!expected || op->argument(1) == expected, "reset did not reuse incoming undef");
						resets++;
					}
	});
	return resets;
}

static void test_feedback(unsigned mode, unsigned iterations)
{
	Fixture f;
	auto *entry = f.node("entry"), *header = f.node("header"), *tail = f.node("tail"), *exit = f.node("exit");
	auto count = f.module.allocate_id(), a = f.module.allocate_id(), b = f.module.allocate_id();
	f.branch(entry, header);
	f.branch(header, tail);
	auto next_count = f.binary(tail, spv::OpIAdd, count, f.constant(1));
	auto go = f.binary(tail, spv::OpULessThan, next_count, f.constant(iterations), f.bool_type);
	f.condition(tail, header, exit, go);
	f.phi(header, count, entry, f.constant(0), tail, next_count);
	if (mode == 0)
	{
		f.phi(header, a, entry, f.constant(7), tail, a);
		f.write(header, a);
	}
	else
	{
		auto next_a = mode == 1 ? b : f.binary(tail, spv::OpIAdd, a, b);
		f.phi(header, a, entry, f.constant(mode == 1 ? 3 : 1), tail, next_a);
		f.phi(header, b, entry, f.constant(mode == 1 ? 5 : 2), tail, a);
		auto encoded = f.binary(header, spv::OpIMul, a, f.constant(100));
		f.write(header, f.binary(header, spv::OpIAdd, encoded, b));
	}
	auto original = Interpreter{ f }.run(entry);
	CFGStructurizer cfg(entry, f.pool, f.module);
	require(cfg.run_structurizer(), "structurization failed");
	require(count_undef_resets(f) == 0, "fully-defined PHI gained an undef reset");
	auto transformed = Interpreter{ f }.run(cfg.get_entry_block());
	require(original.size() == iterations, "wrong reference iteration count");
	require(original == transformed, "defined loop feedback changed");
	printf("feedback mode=%u iterations=%u passed\n", mode, iterations);
}

static void test_loop_liveness(bool vector, bool critical_edge, bool global_undef)
{
	Fixture f(vector);
	auto &builder = f.module.get_builder();
	auto *entry = f.node("entry"), *header = f.node("header"), *defined = f.node("defined"),
	     *undefined = f.node("undefined"), *merge = f.node("merge"), *tail = f.node("tail"), *exit = f.node("exit");
	f.branch(entry, header);
	f.condition(header, defined, critical_edge ? merge : undefined, builder.createUndefinedConstant(f.bool_type));
	f.branch(defined, merge);
	if (!critical_edge)
		f.branch(undefined, merge);
	f.branch(merge, tail);
	f.condition(tail, header, exit, builder.createUndefinedConstant(f.bool_type));
	auto v = vector ? builder.makeCompositeConstant(
	                      f.value_type, { f.constant(40), f.constant(41), f.constant(42), f.constant(43) }) :
	                  f.constant(42);
	auto undef = global_undef ? builder.createUndefinedConstant(f.value_type) : builder.createUndefined(f.value_type);
	auto result = f.module.allocate_id();
	f.phi(merge, result, defined, v, critical_edge ? header : undefined, undef);
	f.write(merge, result);
	CFGStructurizer cfg(entry, f.pool, f.module);
	require(cfg.run_structurizer(), "structurization failed");
	require(count_undef_resets(f, undef) == 1, "loop PHI lost its reset");
	f.pool.for_each_node(
	    [](CFGNode &n)
	    {
		    n.id = 0;
		    n.userdata = nullptr;
	    });
	f.module.emit_entry_point_function_body(cfg);
	Vector<uint32_t> binary;
	require(f.module.finalize_spirv(binary), "SPIR-V finalization failed");
	spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_1);
	require(tools.Validate(binary.data(), binary.size()), "SPIR-V validation failed");
	spvtools::Optimizer optimizer(SPV_ENV_VULKAN_1_1);
	optimizer.RegisterPass(spvtools::CreateSSARewritePass());
	std::vector<uint32_t> ssa;
	require(optimizer.Run(binary.data(), binary.size(), &ssa), "SSA reconstruction failed");
	require(tools.Validate(ssa.data(), ssa.size()), "SSA SPIR-V validation failed");
	unsigned block_phis = 0, loops = 0;
	for (size_t i = 5; i < ssa.size();)
	{
		unsigned count = ssa[i] >> 16;
		auto opcode = spv::Op(ssa[i] & 0xffff);
		require(count > 0 && i + count <= ssa.size(), "invalid SPIR-V instruction");
		if (opcode == spv::OpLabel)
			block_phis = 0;
		else if (opcode == spv::OpPhi)
			block_phis++;
		else if (opcode == spv::OpLoopMerge)
		{
			// This value is defined inside the loop. No value crosses its back edge.
			require(block_phis == 0, "PHI temporary acquired an artificial loop-carried dependency");
			loops++;
		}
		i += count;
	}
	require(loops == 1, "fixture lost its loop");
	printf("loop liveness vector=%u critical_edge=%u global_undef=%u passed\n",
	       unsigned(vector), unsigned(critical_edge), unsigned(global_undef));
}

static void test_acyclic_liveness(bool preceding_loop)
{
	Fixture f;
	auto &builder = f.module.get_builder();
	auto *entry = f.node("entry"), *start = f.node("start"), *defined = f.node("defined"),
	     *undefined = f.node("undefined"), *merge = f.node("merge");
	if (preceding_loop)
	{
		auto *header = f.node("header"), *tail = f.node("tail");
		f.branch(entry, header);
		f.branch(header, tail);
		f.write(header, f.constant(7));
		f.condition(tail, header, start, builder.createUndefinedConstant(f.bool_type));
	}
	else
		f.branch(entry, start);
	f.condition(start, defined, undefined, builder.createUndefinedConstant(f.bool_type));
	f.branch(defined, merge);
	f.branch(undefined, merge);
	auto result = f.module.allocate_id();
	f.phi(merge, result, defined, f.constant(42), undefined, builder.createUndefinedConstant(f.value_type));
	f.write(merge, result);
	CFGStructurizer cfg(entry, f.pool, f.module);
	require(cfg.run_structurizer(), "structurization failed");
	// A loop elsewhere in the function conservatively retains the reset.
	require(count_undef_resets(f) == unsigned(preceding_loop), "unexpected acyclic PHI reset count");
	f.pool.for_each_node([](CFGNode &n) { n.id = 0; n.userdata = nullptr; });
	f.module.emit_entry_point_function_body(cfg);
	Vector<uint32_t> binary;
	require(f.module.finalize_spirv(binary), "SPIR-V finalization failed");
	spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_1);
	require(tools.Validate(binary.data(), binary.size()), "SPIR-V validation failed");
	printf("acyclic liveness preceding_loop=%u passed\n", unsigned(preceding_loop));
}

int main()
{
	begin_thread_allocator_context();
	for (unsigned mode = 0; mode < 3; mode++)
		for (unsigned n : { 1u, 2u, 3u, 7u, 17u })
			test_feedback(mode, n);
	for (bool vector : { false, true })
		for (bool critical_edge : { false, true })
			for (bool global_undef : { false, true })
				test_loop_liveness(vector, critical_edge, global_undef);
	for (bool preceding_loop : { false, true })
		test_acyclic_liveness(preceding_loop);
	end_thread_allocator_context();
	return 0;
}
