/* Copyright (c) 2026 Erhan Bilgili
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "ir/ir_builder.h"
#include <cstddef>
#include <cstdint>
#include <memory>

namespace LLVMBC
{
// This cache is only for the fixed frontend options in module_dxbc_ir.cpp.
// Entries live only in this process and are never saved to disk.
class DxbcIrCache
{
public:
	using Compiler = std::unique_ptr<dxbc_spv::ir::Builder> (*)(const void *, size_t);
	struct Stats
	{
		uint64_t memory_hits = 0, misses = 0, compiles = 0, waits = 0;
		uint64_t validation_failures = 0, evictions = 0;
		size_t bytes = 0;
	};

	explicit DxbcIrCache(size_t max_bytes, bool validate = false);
	~DxbcIrCache();
	std::unique_ptr<dxbc_spv::ir::Builder> get(const void *data, size_t size, Compiler compile);
	Stats getStats();

private:
	struct Impl;
	std::unique_ptr<Impl> impl;
};

std::unique_ptr<dxbc_spv::ir::Builder> getCachedDxbcIr(const void *data, size_t size, DxbcIrCache::Compiler compile);
}
