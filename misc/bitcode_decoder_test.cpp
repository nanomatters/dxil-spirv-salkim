/* Copyright (c) 2026 Erhan Bilgili
 * SPDX-License-Identifier: MIT
 */

#include "llvm_decoder.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

using LLVMBC::BlockOrRecord;

static void check_at(bool condition, const char *expression, unsigned line)
{
	if (!condition)
	{
		std::fprintf(stderr, "Bitcode decoder check failed at line %u: %s\n", line, expression);
		std::abort();
	}
}

#define check(condition) check_at((condition), #condition, __LINE__)

// A small encoder for the bitstream constructs used below. All blocks use
// three-bit abbreviation IDs, except the implicit two-bit top-level block.
struct Bitstream
{
	std::vector<uint8_t> bytes;
	size_t bits = 0;
	unsigned width = 2;
	std::vector<size_t> lengths;

	Bitstream() { fixed(0xdec04342, 32); }

	void fixed(uint64_t value, unsigned count)
	{
		for (unsigned i = 0; i < count; i++, bits++)
		{
			if (!(bits & 7))
				bytes.push_back(0);
			bytes[bits / 8] |= uint8_t(((value >> i) & 1) << (bits & 7));
		}
	}

	void vbr(uint64_t value, unsigned count)
	{
		const uint64_t continuation = uint64_t(1) << (count - 1);
		do
		{
			auto group = value & (continuation - 1);
			value >>= count - 1;
			fixed(group | (value ? continuation : 0), count);
		} while (value);
	}

	void align() { while (bits & 31) fixed(0, 1); }
	void code(unsigned id) { fixed(id, width); }
	void literal(unsigned value) { fixed(1, 1); vbr(value, 8); }
	void encoding(unsigned value) { fixed(0, 1); fixed(value, 3); }

	void begin(unsigned id)
	{
		code(1);
		vbr(id, 8);
		vbr(3, 4);
		align();
		lengths.push_back(bytes.size());
		fixed(0, 32);
		width = 3;
	}

	void end()
	{
		code(0);
		align();
		auto offset = lengths.back();
		lengths.pop_back();
		auto words = (bytes.size() - offset - 4) / 4;
		for (unsigned i = 0; i < 4; i++)
			bytes[offset + i] = uint8_t(words >> (8 * i));
		width = lengths.empty() ? 2 : 3;
	}

	void record(unsigned id, const std::vector<uint64_t> &ops)
	{
		code(3);
		vbr(id, 6);
		vbr(ops.size(), 6);
		for (auto op : ops) vbr(op, 6);
	}

	size_t blob(const std::vector<uint8_t> &data)
	{
		vbr(data.size(), 6);
		align();
		auto offset = bytes.size();
		for (auto byte : data) fixed(byte, 8);
		align();
		return offset;
	}
};

static void check_record(const BlockOrRecord &record, unsigned id, const std::vector<uint64_t> &ops)
{
	check(record.IsRecord() && record.id == id && record.children.empty());
	check(record.ops.size() == ops.size());
	for (size_t i = 0; i < ops.size(); i++) check(record.ops[i] == ops[i]);
}

static BlockOrRecord read(const Bitstream &stream)
{
	LLVMBC::BitcodeReader reader(stream.bytes.data(), stream.bytes.size());
	auto result = reader.ReadToplevelBlock();
	check(reader.AtEndOfStream());
	return result;
}

static void test_records_and_blocks()
{
	Bitstream stream;
	stream.begin(8);
	stream.record(7, {});
	const std::vector<uint64_t> values = { 0, 31, 32, 4096, std::numeric_limits<uint64_t>::max() };
	stream.record(8, values);

	// Abbreviation 4: literal record ID, fixed, VBR and Char6 operands.
	stream.code(2); stream.vbr(4, 5); stream.literal(9);
	stream.encoding(1); stream.vbr(5, 5);
	stream.encoding(2); stream.vbr(6, 5);
	stream.encoding(4);
	stream.code(4); stream.fixed(27, 5); stream.vbr(4096, 6); stream.fixed(51, 6);

	// Abbreviation 5: empty and growing arrays.
	stream.code(2); stream.vbr(3, 5); stream.literal(10);
	stream.encoding(3); stream.encoding(2); stream.vbr(6, 5);
	stream.code(5); stream.vbr(0, 6);
	stream.code(5); stream.vbr(257, 6);
	std::vector<uint64_t> array;
	for (unsigned i = 0; i < 257; i++) { array.push_back(i * 37); stream.vbr(i * 37, 6); }

	// Abbreviation 6: blobs borrow the input storage, including embedded NULs.
	stream.code(2); stream.vbr(2, 5); stream.literal(11); stream.encoding(5);
	stream.code(6);
	const std::vector<uint8_t> payload = { 0, 255, 'a', 0, 'b' };
	auto blob_offset = stream.blob(payload);
	stream.code(6);
	auto empty_blob_offset = stream.blob({});

	stream.begin(12); stream.end();
	for (unsigned i = 0; i < 8; i++) stream.begin(20 + i);
	stream.record(77, values);
	for (unsigned i = 0; i < 8; i++) stream.end();
	// Force repeated child-vector growth after the nested subtree and blobs.
	for (unsigned i = 0; i < 128; i++) stream.record(1000 + i, { i });
	stream.end();

	auto root = read(stream); // Check the tree after the reader has been destroyed.
	check(root.IsBlock() && root.id == 8 && root.children.size() == 137);
	check(root.blockDwordLength == (stream.bytes.size() - 12) / 4);
	check_record(root.children[0], 7, {});
	check_record(root.children[1], 8, values);
	check_record(root.children[2], 9, { 27, 4096, 'Z' });
	check_record(root.children[3], 10, {});
	check_record(root.children[4], 10, array);
	check_record(root.children[5], 11, {});
	check(root.children[5].blob == stream.bytes.data() + blob_offset);
	check(root.children[5].blobLength == payload.size());
	check(!memcmp(root.children[5].blob, payload.data(), payload.size()));
	check_record(root.children[6], 11, {});
	check(root.children[6].blob == stream.bytes.data() + empty_blob_offset);
	check(root.children[6].blobLength == 0);
	check(root.children[7].id == 12 && root.children[7].IsBlock() && root.children[7].children.empty());
	auto *node = &root.children[8];
	for (unsigned i = 0; i < 8; i++)
	{
		check(node->id == 20 + i && node->IsBlock() && node->children.size() == 1);
		node = &node->children[0];
	}
	check_record(*node, 77, values);
	for (unsigned i = 0; i < 128; i++) check_record(root.children[9 + i], 1000 + i, { i });
}

static void test_shared_abbreviations()
{
	Bitstream stream;
	stream.begin(8);
	stream.begin(0); // BLOCKINFO defines an abbreviation for block 42.
	stream.record(1, { 42 }); // SETBID must be consumed before moving its record.
	stream.code(2); stream.vbr(2, 5); stream.literal(15);
	stream.encoding(2); stream.vbr(6, 5);
	stream.end();
	for (unsigned i = 0; i < 64; i++)
	{
		stream.begin(42);
		stream.code(4); stream.vbr(100 + i, 6);
		stream.end();
	}
	stream.end();
	auto root = read(stream);
	check(root.children.size() == 65);
	check_record(root.children[0].children[0], 1, { 42 });
	for (unsigned i = 0; i < 64; i++)
	{
		const auto &child = root.children[i + 1];
		check(child.id == 42 && child.IsBlock() && child.children.size() == 1);
		check_record(child.children[0], 15, { 100 + i });
	}
}

static void test_move_ownership()
{
	static_assert(std::is_nothrow_move_constructible<BlockOrRecord>::value,
	              "Child-vector growth must move nodes without copying their subtrees.");
	const uint8_t blob[] = { 1, 2, 3 };
	dxil_spv::Vector<BlockOrRecord> nodes;
	const uint64_t *ops;
	const BlockOrRecord *children;
	{
		BlockOrRecord node;
		node.id = 8;
		node.blockDwordLength = 1;
		node.children.resize(1);
		node.children[0].id = 9;
		node.children[0].ops = { 10, 20 };
		node.children[0].blob = blob;
		node.children[0].blobLength = sizeof(blob);
		ops = node.children[0].ops.data();
		children = node.children.data();
		nodes.push_back(std::move(node));
	}
	for (unsigned i = 0; i < 128; i++) { BlockOrRecord node; node.id = i; nodes.push_back(std::move(node)); }
	check(nodes[0].children.data() == children && nodes[0].children[0].ops.data() == ops);
	check_record(nodes[0].children[0], 9, { 10, 20 });
	check(nodes[0].children[0].blob == blob && nodes[0].children[0].blobLength == sizeof(blob));
}

int main()
{
	for (unsigned arena = 0; arena < 2; arena++)
	{
		if (arena) dxil_spv::begin_thread_allocator_context();
		for (unsigned repeat = 0; repeat < 4; repeat++)
		{
			test_records_and_blocks();
			test_shared_abbreviations();
			test_move_ownership();
			// All nodes have been destroyed before their backing arena is reset.
			if (arena) dxil_spv::reset_thread_allocator_context();
		}
		if (arena) dxil_spv::end_thread_allocator_context();
	}
	std::puts("PASS: records, blocks, abbreviations, blobs and moves with malloc and arena allocation.");
}
