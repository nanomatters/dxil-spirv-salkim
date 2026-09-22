/* Copyright (c) 2019-2022 Hans-Kristian Arntzen for Valve Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "thread_local_allocator.hpp"
#include <assert.h>
#include <stdint.h>
#include <array>
#include <memory>
#include <mutex>

namespace dxil_spv
{
static constexpr size_t BLOCK_SIZE = 64 * 1024;
static constexpr size_t CACHED_BLOCK_COUNT = 128;
static constexpr size_t CACHED_ALLOCATOR_COUNT = 4;

class ChainAllocator
{
public:
	bool reset(size_t max_blocks = SIZE_MAX);
	void *allocate(size_t size);

private:
	struct MallocDeleter
	{
		void operator()(void *ptr)
		{
			free(ptr);
		}
	};

	struct Block
	{
		explicit Block(size_t size);
		void *allocate(size_t size);

		std::unique_ptr<uint8_t, MallocDeleter> block;
		size_t offset = 0;
		size_t block_size = 0;
	};
	std::vector<Block> blocks;
	std::vector<Block> huge_blocks;
	unsigned block_index = 0;

	bool ensure_block();
	void *allocate_huge(size_t size);
};

ChainAllocator::Block::Block(size_t size)
	: block(static_cast<uint8_t *>(malloc(size))), block_size(size)
{
}

void *ChainAllocator::Block::allocate(size_t size)
{
	offset = (offset + 15) & ~size_t(15);
	if (offset + size <= block_size)
	{
		void *ret = block.get() + offset;
		offset += size;
		return ret;
	}
	else
		return nullptr;
}

static thread_local ChainAllocator *allocator;

// Bound idle memory across all threads, without thread-exit callbacks which
// can keep a compiler DLL loaded. Active contexts never share an allocator.
static std::mutex allocator_cache_mutex;
static std::array<std::unique_ptr<ChainAllocator>, CACHED_ALLOCATOR_COUNT> allocator_cache;
static size_t allocator_cache_count;

bool ChainAllocator::reset(size_t max_blocks)
{
	// Discard oversized arenas rather than allocate metadata while shrinking
	// them. Cleanup must not allocate, especially after an allocation failure.
	if (blocks.capacity() > max_blocks)
		return false;
	// A failed allocation must not leave an unusable block in the next context.
	if (!blocks.empty() && !blocks.back().block)
		blocks.pop_back();

	for (auto &block : blocks)
		block.offset = 0;
	block_index = 0;
	huge_blocks.clear();
	if (max_blocks != SIZE_MAX)
		std::vector<Block>().swap(huge_blocks);
	return true;
}

bool ChainAllocator::ensure_block()
{
	blocks.emplace_back(BLOCK_SIZE);
	return bool(blocks.back().block);
}

void *ChainAllocator::allocate_huge(size_t size)
{
	huge_blocks.emplace_back(size);
	return huge_blocks.back().block.get();
}

void *ChainAllocator::allocate(size_t size)
{
	if (size > BLOCK_SIZE)
		return allocate_huge(size);

	if (block_index >= blocks.size() && !ensure_block())
		return nullptr;

	void *ptr = blocks[block_index].allocate(size);
	if (ptr)
		return ptr;

	block_index++;
	if (block_index >= blocks.size() && !ensure_block())
		return nullptr;

	return blocks[block_index].allocate(size);
}

void *allocate_in_thread(size_t size)
{
	if (!allocator)
		return malloc(size);

	return allocator->allocate(size);
}

void free_in_thread(void *ptr)
{
	if (!allocator)
	{
		free(ptr);
		return;
	}

	// Don't bother freeing ...
}

void begin_thread_allocator_context()
{
	assert(!allocator);
	{
		std::lock_guard<std::mutex> holder(allocator_cache_mutex);
		if (allocator_cache_count)
			allocator = allocator_cache[--allocator_cache_count].release();
	}
	if (!allocator)
		allocator = new ChainAllocator;
}

void end_thread_allocator_context()
{
	assert(allocator);
	std::unique_ptr<ChainAllocator> retired(allocator);
	allocator = nullptr;
	if (!retired->reset(CACHED_BLOCK_COUNT))
		return;
	{
		std::lock_guard<std::mutex> holder(allocator_cache_mutex);
		if (allocator_cache_count < allocator_cache.size())
			allocator_cache[allocator_cache_count++] = std::move(retired);
	}
}

void reset_thread_allocator_context()
{
	assert(allocator);
	allocator->reset();
}
}
