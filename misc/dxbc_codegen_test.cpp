/* Copyright (c) 2026 Erhan Bilgili
 * SPDX-License-Identifier: MIT
 */

#include "dxil_converter.hpp"
#include "cfg_structurizer.hpp"
#include "thread_local_allocator.hpp"
#include "api/test_api_common.h"
#include "spirv-tools/libspirv.hpp"
#include <cstdio>
#include <cstdlib>
#include <unordered_map>

using namespace dxil_spv;
using namespace dxbc_spv;

static void check_at(bool result, unsigned line)
{
	if (!result)
	{
		std::fprintf(stderr, "DXBC codegen check failed at line %u.\n", line);
		std::exit(1);
	}
}
#define check(x) check_at((x), __LINE__)

static Vector<uint32_t> convert(ir::Builder &builder)
{
	LLVMBCParser parser;
	check(parser.parseDXBC(builder));
	SPIRVModule module;
	Converter converter(parser, nullptr, module);
	auto entry = converter.convert_entry_point();
	check(entry.entry.entry);
	CFGStructurizer structurizer(entry.entry.entry, *entry.node_pool, module);
	if (entry.entry.is_structured)
		structurizer.run_trivial();
	else
		structurizer.run();
	module.emit_entry_point_function_body(structurizer);
	check(entry.leaf_functions.empty());
	Vector<uint32_t> spirv;
	check(module.finalize_spirv(spirv));
	spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_3);
	tools.SetMessageConsumer([](spv_message_level_t, const char *, const spv_position_t &, const char *message) {
		std::fprintf(stderr, "%s\n", message);
	});
	check(tools.Validate(spirv.data(), spirv.size()));
	return spirv;
}

static void test_interpolation()
{
	using namespace ir;
	InterpolationModes modes[] = { {}, InterpolationMode::eFlat, InterpolationMode::eCentroid,
		InterpolationMode::eSample, InterpolationMode::eNoPerspective,
		InterpolationMode::eNoPerspective | InterpolationMode::eCentroid,
		InterpolationMode::eNoPerspective | InterpolationMode::eSample };
	Builder b;
	auto ep = test_api::setupTestFunction(b, ir::ShaderStage::ePixel);
	b.add(Op::Label());
	for (unsigned i = 0; i < 7; i++)
	{
		auto input = b.add(Op::DclInput(ScalarType::eF32, ep, i, 0u, modes[i]));
		b.add(Op::Semantic(input, i, "TEXCOORD"));
		auto output = b.add(Op::DclOutput(ScalarType::eF32, ep, i, 0u));
		b.add(Op::Semantic(output, i, "SV_TARGET"));
		b.add(Op::OutputStore(output, SsaDef(), b.add(Op::InputLoad(ScalarType::eF32, input, SsaDef()))));
	}
	b.add(Op::Return());
	auto spirv = convert(b);
	std::unordered_map<uint32_t, unsigned> locations, decorations;
	for (size_t i = 5; i < spirv.size(); i += spirv[i] >> 16)
	{
		if (spv::Op(spirv[i] & 0xffff) != spv::OpDecorate) continue;
		auto id = spirv[i + 1];
		switch (spv::Decoration(spirv[i + 2]))
		{
		case spv::DecorationLocation: locations[id] = spirv[i + 3]; break;
		case spv::DecorationFlat: decorations[id] |= 1; break;
		case spv::DecorationCentroid: decorations[id] |= 2; break;
		case spv::DecorationSample: decorations[id] |= 4; break;
		case spv::DecorationNoPerspective: decorations[id] |= 8; break;
		default: break;
		}
	}
	unsigned inputs = 0;
	const unsigned expected[] = { 0, 1, 2, 4, 8, 10, 12 };
	for (size_t i = 5; i < spirv.size(); i += spirv[i] >> 16)
		if (spv::Op(spirv[i] & 0xffff) == spv::OpVariable && spirv[i + 3] == spv::StorageClassInput)
		{
			auto id = spirv[i + 2];
			check(locations.count(id) && locations[id] < 7);
			check(decorations[id] == expected[locations[id]]);
			inputs++;
		}
	check(inputs == 7);
}

static void test_lds_atomic(unsigned result_mode)
{
	using namespace ir;
	Builder b;
	auto ep = test_api::setupTestFunction(b, ir::ShaderStage::eCompute);
	b.add(Op::SetCsWorkgroupSize(ep, 1u, 1u, 1u));
	b.add(Op::Label());
	auto lds = b.add(Op::DclLds(Type(ScalarType::eU32).addArrayDimension(1u), ep));
	auto args = b.add(Op::CompositeConstruct(BasicType(ScalarType::eU32, 2u),
	    b.makeConstant(0u), b.makeConstant(1u)));
	auto result = b.add(Op::LdsAtomic(AtomicOp::eCompareExchange,
	    result_mode ? ScalarType::eU32 : ScalarType::eVoid, lds, b.makeConstant(0u), args));
	if (result_mode == 2)
		b.add(Op::LdsStore(lds, b.makeConstant(0u), result));
	b.add(Op::Return());
	auto spirv = convert(b);
	unsigned atomics = 0;
	for (size_t i = 5; i < spirv.size(); i += spirv[i] >> 16)
		atomics += spv::Op(spirv[i] & 0xffff) == spv::OpAtomicCompareExchange;
	check(atomics == 1);
}

int main()
{
	begin_thread_allocator_context();
	test_interpolation();
	for (unsigned mode = 0; mode < 3; mode++)
		test_lds_atomic(mode);
	end_thread_allocator_context();
	std::puts("DXBC codegen cases passed.");
}
