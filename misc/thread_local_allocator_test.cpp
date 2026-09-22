/* Copyright (c) 2026 Erhan Bilgili
 * SPDX-License-Identifier: MIT
 */

#include "thread_local_allocator.hpp"
#include <array>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <memory>
#include <mutex>
#include <thread>

static void check_at(bool condition, const char *expression, unsigned line)
{
	if (!condition)
	{
		std::fprintf(stderr, "Allocator check failed at line %u: %s\n", line, expression);
		std::abort();
	}
}
#define check(condition) check_at((condition), #condition, __LINE__)

struct AllocationTracker
{
	std::mutex mutex;
	std::unordered_map<void *, size_t> live;
	size_t calls = 0;
	size_t fail_size = 0;

	~AllocationTracker()
	{
		check(live.empty());
		std::puts("PASS: cached storage released at library teardown.");
	}
};

// Construct before the allocator cache, so teardown checks run after it.
static AllocationTracker tracker;

static void *tracked_malloc(size_t size)
{
	std::lock_guard<std::mutex> holder(tracker.mutex);
	tracker.calls++;
	if (tracker.fail_size && size == tracker.fail_size)
	{
		tracker.fail_size = 0;
		return nullptr;
	}
	void *ptr = std::malloc(size);
	check(ptr != nullptr);
	check(tracker.live.emplace(ptr, size).second);
	return ptr;
}

static void tracked_free(void *ptr)
{
	if (!ptr) return;
	std::lock_guard<std::mutex> holder(tracker.mutex);
	check(tracker.live.erase(ptr) == 1);
	std::free(ptr);
}

// Instrument the actual implementation without a production statistics API.
// Standard headers are already included, so their allocators are not replaced.
#define malloc tracked_malloc
#define free tracked_free
#include "../util/thread_local_allocator.cpp"
#undef malloc
#undef free

using namespace dxil_spv;

static size_t allocation_calls()
{
	std::lock_guard<std::mutex> holder(tracker.mutex);
	return tracker.calls;
}

static size_t live_bytes()
{
	std::lock_guard<std::mutex> holder(tracker.mutex);
	size_t bytes = 0;
	for (auto &entry : tracker.live) bytes += entry.second;
	return bytes;
}

static bool is_live(void *ptr)
{
	std::lock_guard<std::mutex> holder(tracker.mutex);
	return tracker.live.count(ptr) != 0;
}

static void check_block_alignment(void *ptr)
{
	std::lock_guard<std::mutex> holder(tracker.mutex);
	auto address = reinterpret_cast<uintptr_t>(ptr);
	for (auto &entry : tracker.live)
	{
		auto base = reinterpret_cast<uintptr_t>(entry.first);
		if (address >= base && address - base < entry.second)
		{
			// Offsets are aligned to 16. The block base keeps malloc's native
			// alignment, which can be 8 bytes with the 32-bit Windows CRT.
			check(!((address - base) & 15));
			return;
		}
	}
	check(false);
}

static void test_heap_and_reuse()
{
	auto *heap = static_cast<unsigned char *>(allocate_in_thread(73));
	std::memset(heap, 0x4a, 73);
	begin_thread_allocator_context();
	for (unsigned i = 0; i < 4; i++) check(allocate_in_thread(BLOCK_SIZE) != nullptr);
	end_thread_allocator_context();
	check(heap[0] == 0x4a && heap[72] == 0x4a);
	free_in_thread(heap);
	check(!is_live(heap));

	auto calls = allocation_calls();
	// Cache storage can be handed to a different thread after end().
	std::thread thread([] {
		begin_thread_allocator_context();
		for (unsigned i = 0; i < 4; i++) check(allocate_in_thread(BLOCK_SIZE) != nullptr);
		end_thread_allocator_context();
	});
	thread.join();
	check(allocation_calls() == calls);

	heap = static_cast<unsigned char *>(allocate_in_thread(51));
	check(allocation_calls() == calls + 1);
	free_in_thread(heap);
}

static void test_reset_and_huge_allocations()
{
	begin_thread_allocator_context();
	auto *first = allocate_in_thread(1024);
	auto *huge = allocate_in_thread(BLOCK_SIZE + 17);
	check(is_live(huge));
	reset_thread_allocator_context();
	check(!is_live(huge));
	auto calls = allocation_calls();
	check(allocate_in_thread(1024) == first);
	check(allocation_calls() == calls);
	for (unsigned size = 1; size < 4096; size += 17)
		check_block_alignment(allocate_in_thread(size));
	huge = allocate_in_thread(BLOCK_SIZE + 17);
	end_thread_allocator_context();
	check(!is_live(huge));
}

static void test_retention_limit()
{
	begin_thread_allocator_context();
	for (size_t i = 0; i < CACHED_BLOCK_COUNT + 33; i++)
		check(allocate_in_thread(BLOCK_SIZE) != nullptr);
	end_thread_allocator_context();
	check(live_bytes() == 0);
	{
		std::lock_guard<std::mutex> holder(allocator_cache_mutex);
		check(allocator_cache_count == 0);
	}
	begin_thread_allocator_context();
	for (size_t i = 0; i < CACHED_BLOCK_COUNT; i++) check(allocate_in_thread(BLOCK_SIZE) != nullptr);
	end_thread_allocator_context();
	check(live_bytes() == CACHED_BLOCK_COUNT * BLOCK_SIZE);
	auto calls = allocation_calls();
	begin_thread_allocator_context();
	for (size_t i = 0; i < CACHED_BLOCK_COUNT; i++) check(allocate_in_thread(BLOCK_SIZE) != nullptr);
	end_thread_allocator_context();
	check(allocation_calls() == calls);
}

static void test_failed_allocations()
{
	begin_thread_allocator_context();
	for (size_t i = 0; i < CACHED_BLOCK_COUNT; i++) check(allocate_in_thread(BLOCK_SIZE) != nullptr);
	tracker.fail_size = BLOCK_SIZE;
	check(allocate_in_thread(BLOCK_SIZE) == nullptr);
	end_thread_allocator_context();

	begin_thread_allocator_context();
	for (size_t i = 0; i <= CACHED_BLOCK_COUNT; i++) check(allocate_in_thread(BLOCK_SIZE) != nullptr);
	tracker.fail_size = BLOCK_SIZE + 1;
	check(allocate_in_thread(BLOCK_SIZE + 1) == nullptr);
	end_thread_allocator_context();
	begin_thread_allocator_context();
	check(allocate_in_thread(BLOCK_SIZE + 1) != nullptr);
	end_thread_allocator_context();
	check(live_bytes() == 0);
	begin_thread_allocator_context();
	check(allocate_in_thread(BLOCK_SIZE) != nullptr);
	end_thread_allocator_context();
	check(live_bytes() == BLOCK_SIZE);
	// A failed block in a small arena must be removed even when the arena
	// itself fits the cache, rather than being handed to the next context.
	begin_thread_allocator_context();
	check(allocate_in_thread(BLOCK_SIZE) != nullptr);
	tracker.fail_size = BLOCK_SIZE;
	check(allocate_in_thread(BLOCK_SIZE) == nullptr);
	end_thread_allocator_context();
	check(live_bytes() == BLOCK_SIZE);
	begin_thread_allocator_context();
	check(allocate_in_thread(BLOCK_SIZE) != nullptr);
	check(allocate_in_thread(BLOCK_SIZE) != nullptr);
	end_thread_allocator_context();
	check(live_bytes() == 2 * BLOCK_SIZE);
}

static void test_concurrent_contexts()
{
	constexpr unsigned threads = 8;
	std::mutex mutex;
	std::condition_variable cond;
	unsigned ready = 0;
	unsigned ended = 0;
	std::array<void *, threads> first = {};
	std::vector<std::thread> workers;
	for (unsigned i = 0; i < threads; i++)
	{
		workers.emplace_back([&, i] {
			for (unsigned repeat = 0; repeat < 64; repeat++)
			{
				begin_thread_allocator_context();
				auto *ptr = static_cast<unsigned char *>(allocate_in_thread(BLOCK_SIZE));
				std::memset(ptr, i + 1, BLOCK_SIZE);
				if (!repeat)
				{
					for (size_t b = 1; b < CACHED_BLOCK_COUNT; b++)
						check(allocate_in_thread(BLOCK_SIZE) != nullptr);
					std::unique_lock<std::mutex> holder(mutex);
					first[i] = ptr;
					ready++;
					cond.notify_all();
					cond.wait(holder, [&] { return ready == threads; });
					for (unsigned j = 0; j < threads; j++) if (i != j) check(first[i] != first[j]);
				}
				check(allocate_in_thread(BLOCK_SIZE + 1) != nullptr);
				for (size_t b = 0; b < BLOCK_SIZE; b++) check(ptr[b] == i + 1);
				end_thread_allocator_context();
				if (!repeat)
				{
					std::unique_lock<std::mutex> holder(mutex);
					if (++ended == threads)
					{
						check(live_bytes() == CACHED_ALLOCATOR_COUNT * CACHED_BLOCK_COUNT * BLOCK_SIZE);
						cond.notify_all();
					}
					else
						cond.wait(holder, [&] { return ended == threads; });
				}
				void *heap = allocate_in_thread(123);
				free_in_thread(heap);
			}
		});
	}
	for (auto &worker : workers) worker.join();
	{
		std::lock_guard<std::mutex> holder(allocator_cache_mutex);
		check(allocator_cache_count == CACHED_ALLOCATOR_COUNT);
	}
	check(live_bytes() <= CACHED_ALLOCATOR_COUNT * CACHED_BLOCK_COUNT * BLOCK_SIZE);
}

int main()
{
	test_heap_and_reuse();
	test_reset_and_huge_allocations();
	test_retention_limit();
	test_failed_allocations();
	test_concurrent_contexts();
	std::puts("PASS: heap fallback, reuse, reset, bounds, allocation failures and concurrent contexts.");
	return 0;
}
