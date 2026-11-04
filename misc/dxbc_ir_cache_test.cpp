/* Copyright (c) 2026 Erhan Bilgili
 * SPDX-License-Identifier: MIT
 */

#include "dxbc_ir_cache.hpp"
#include "ir/ir_serialize.h"

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

using LLVMBC::DxbcIrCache;
using dxbc_spv::ir::Builder;

static void check_at(bool condition, const char *expression, unsigned line)
{
	if (!condition)
	{
		std::fprintf(stderr, "DXBC IR cache check failed at line %u: %s\n", line, expression);
		std::abort();
	}
}
#define check(condition) check_at(bool(condition), #condition, __LINE__)

static constexpr size_t cache_budget = 1024 * 1024;
using Key = std::array<uint8_t, 36>;

static void set_u32(uint8_t *bytes, size_t offset, uint32_t value)
{
	for (unsigned i = 0; i < 4; i++)
		bytes[offset + i] = uint8_t(value >> (8 * i));
}

static Key make_key(uint8_t id)
{
	Key bytes = {};
	std::memcpy(bytes.data(), "DXBC", 4);
	// Synthetic compiler inputs need only a well-formed header and distinct identity.
	bytes[4] = id;
	set_u32(bytes.data(), 20, 1);
	set_u32(bytes.data(), 24, bytes.size());
	bytes[32] = 1;
	bytes[34] = 2;
	bytes[35] = id;
	return bytes;
}

static const Key key_a = make_key(3);
static const Key key_b = make_key(4);
static const Key key_c = make_key(5);

static std::unique_ptr<Builder> make_ir(const void *input, size_t size, unsigned revision)
{
	using namespace dxbc_spv::ir;
	auto builder = std::unique_ptr<Builder>(new Builder);
	// Keep every input byte, including zeroes and the length, observable in the IR.
	builder->makeConstant(uint32_t(0x12340000u + size));
	for (size_t i = 0; i < size; i++)
		builder->makeConstant(uint32_t(0x23450000u + i * 256u + static_cast<const uint8_t *>(input)[i]));
	builder->makeConstant(uint32_t(0x34560000u + revision));
	auto end = builder->add(Op::FunctionEnd());
	auto function = builder->addBefore(end, Op::Function(ScalarType::eVoid));
	auto entry = builder->add(Op::EntryPoint(function, ShaderStage::eCompute));
	builder->add(Op::SetCsWorkgroupSize(entry, 1, 1, 1));
	builder->addAfter(function, Op::Label());
	builder->addBefore(end, Op::Return());
	return builder;
}

static std::vector<uint8_t> serialize(const Builder &builder)
{
	dxbc_spv::ir::Serializer serializer(builder);
	std::vector<uint8_t> bytes(serializer.computeSerializedSize());
	check(serializer.serialize(bytes.data(), bytes.size()));
	return bytes;
}

struct CompilerControl
{
	std::atomic<unsigned> calls = { 0 };
	std::atomic<unsigned> revision = { 0 };
	std::atomic<bool> fail_next = { false };
	std::mutex mutex;
	std::condition_variable cond;
	unsigned entered = 0;
	bool hold = false;
	bool released = false;

	void wait_for_compilers(unsigned count)
	{
		std::unique_lock<std::mutex> holder(mutex);
		check(cond.wait_for(holder, std::chrono::seconds(10), [&] { return entered >= count; }));
	}

	void release()
	{
		std::lock_guard<std::mutex> holder(mutex);
		released = true;
		cond.notify_all();
	}
};

// Compiler is a function pointer, so each test installs its control before creating workers.
static CompilerControl *compiler_control;
static std::unique_ptr<Builder> compile(const void *input, size_t size)
{
	auto &control = *compiler_control;
	control.calls++;
	{
		std::unique_lock<std::mutex> holder(control.mutex);
		if (control.hold)
		{
			control.entered++;
			control.cond.notify_all();
			check(control.cond.wait_for(holder, std::chrono::seconds(10), [&] { return control.released; }));
		}
	}
	if (control.fail_next.exchange(false))
		return nullptr;
	return make_ir(input, size, control.revision.load());
}

static std::unique_ptr<Builder> get(DxbcIrCache &cache, const Key &key)
{
	return cache.get(key.data(), key.size(), compile);
}

class StartBarrier
{
public:
	explicit StartBarrier(unsigned count) : count(count) {}

	void arrive_and_wait()
	{
		std::unique_lock<std::mutex> holder(mutex);
		if (++arrived == count)
			cond.notify_all();
		else
			check(cond.wait_for(holder, std::chrono::seconds(10), [&] { return arrived == count; }));
	}

private:
	std::mutex mutex;
	std::condition_variable cond;
	unsigned count;
	unsigned arrived = 0;
};

static void test_memory_and_independence()
{
	CompilerControl control;
	compiler_control = &control;
	DxbcIrCache cache(cache_budget);
	auto first = get(cache, key_a);
	check(first);
	auto expected = serialize(*first);
	Key equal_copy = key_a;
	auto second = get(cache, equal_copy);
	check(second && second.get() != first.get());
	check(serialize(*second) == expected);
	first->makeConstant(0xdeadbeefu);
	check(serialize(*first) != expected);
	check(serialize(*second) == expected);
	second->makeConstant(0xcafebabeu);
	auto third = get(cache, key_a);
	check(third && serialize(*third) == expected);
	check(control.calls == 1);
	check(cache.getStats().memory_hits == 2);

	// Distinct checksums, and the same checksum with a different length, are separate keys.
	auto other = get(cache, key_b);
	auto short_input = key_a;
	set_u32(short_input.data(), 24, short_input.size() - 1);
	auto shorter = cache.get(short_input.data(), short_input.size() - 1, compile);
	check(other && shorter);
	check(serialize(*other) != expected);
	check(serialize(*shorter) != expected);
	check(control.calls == 3);
	check(cache.getStats().misses == 3);
}

static void test_same_key_concurrency(bool fail)
{
	CompilerControl control;
	control.hold = true;
	control.fail_next = fail;
	compiler_control = &control;
	DxbcIrCache cache(cache_budget);
	constexpr unsigned waiters = 6;
	std::array<std::unique_ptr<Builder>, waiters + 1> results;
	std::thread leader([&] { results[0] = get(cache, key_a); });
	control.wait_for_compilers(1);
	StartBarrier barrier(waiters + 1);
	std::vector<std::thread> workers;
	for (unsigned i = 0; i < waiters; i++)
		workers.emplace_back([&, i] { barrier.arrive_and_wait(); results[i + 1] = get(cache, key_a); });
	barrier.arrive_and_wait();
	// Observe that all callers reached the in-flight entry before allowing it to complete.
	auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
	while (cache.getStats().waits < waiters)
	{
		check(control.calls == 1);
		check(std::chrono::steady_clock::now() < deadline);
		std::this_thread::yield();
	}
	control.release();
	leader.join();
	for (auto &worker : workers) worker.join();
	if (fail)
	{
		check(!results[0]);
		// Every waiting caller must wake and recover; retry coordination is optional.
		check(control.calls >= 2);
	}
	else
	{
		check(control.calls == 1);
		check(cache.getStats().misses == 1);
		check(results[0]);
	}
	auto expected = serialize(*make_ir(key_a.data(), key_a.size(), 0));
	unsigned first_result = fail ? 1 : 0;
	for (unsigned i = first_result; i < results.size(); i++)
	{
		check(results[i] && serialize(*results[i]) == expected);
		for (unsigned j = first_result; j < i; j++) check(results[i].get() != results[j].get());
	}
	results[first_result]->makeConstant(0xdeadbeefu);
	for (unsigned i = first_result + 1; i < results.size(); i++) check(serialize(*results[i]) == expected);
	if (fail)
	{
		auto recovered = get(cache, key_a);
		check(recovered && serialize(*recovered) == expected);
		auto calls = control.calls.load();
		check(get(cache, key_a));
		check(control.calls == calls);
	}
}

static void test_different_key_concurrency()
{
	CompilerControl control;
	control.hold = true;
	compiler_control = &control;
	DxbcIrCache cache(cache_budget);
	StartBarrier barrier(2);
	std::unique_ptr<Builder> a, b;
	std::thread first([&] { barrier.arrive_and_wait(); a = get(cache, key_a); });
	std::thread second([&] { barrier.arrive_and_wait(); b = get(cache, key_b); });
	// Both callbacks must run concurrently: holding one key cannot hold the global cache mutex.
	control.wait_for_compilers(2);
	control.release();
	first.join();
	second.join();
	check(a && b && serialize(*a) != serialize(*b));
	check(control.calls == 2);
}

static void test_failure_and_bypass()
{
	CompilerControl control;
	compiler_control = &control;
	DxbcIrCache cache(cache_budget);
	control.fail_next = true;
	check(!get(cache, key_a));
	check(cache.getStats().bytes == 0);
	check(get(cache, key_a));
	check(get(cache, key_a));
	check(control.calls == 2);
	check(cache.getStats().memory_hits == 1);

	control.calls = 0;
	DxbcIrCache bypass(0);
	check(get(bypass, key_a));
	check(get(bypass, key_a));
	check(control.calls == 2);
	check(bypass.getStats().bytes == 0);
	check(bypass.getStats().memory_hits == 0);
}

static void test_lru_budget()
{
	CompilerControl control;
	compiler_control = &control;
	size_t entry_bytes;
	{
		DxbcIrCache probe(cache_budget);
		auto result = get(probe, key_a);
		check(result);
		entry_bytes = probe.getStats().bytes;
		check(entry_bytes >= serialize(*result).size());
	}
	control.calls = 0;
	DxbcIrCache cache(entry_bytes * 2);
	check(get(cache, key_a));
	check(get(cache, key_b));
	check(get(cache, key_a)); // Make B the least recently used entry.
	check(get(cache, key_c));
	check(control.calls == 3);
	check(cache.getStats().evictions >= 1);
	check(cache.getStats().bytes <= entry_bytes * 2);
	check(get(cache, key_a));
	check(control.calls == 3);
	check(get(cache, key_b));
	check(control.calls == 4);
	check(cache.getStats().bytes <= entry_bytes * 2);

	control.calls = 0;
	DxbcIrCache too_small(entry_bytes - 1);
	check(get(too_small, key_a));
	check(get(too_small, key_a));
	check(control.calls == 2);
	check(too_small.getStats().bytes == 0);
}

static void test_header_eligibility()
{
	CompilerControl control;
	compiler_control = &control;
	DxbcIrCache cache(cache_budget);
	check(get(cache, key_a));
	const auto bytes = cache.getStats().bytes;

	// Header reads must also work when the caller's bytecode pointer is unaligned.
	std::array<uint8_t, key_a.size() + 1> unaligned;
	std::memcpy(unaligned.data() + 1, key_a.data(), key_a.size());
	check(cache.get(unaligned.data() + 1, key_a.size(), compile));
	check(control.calls == 1);

	auto check_bypass = [&](const void *input, size_t size) {
		const auto calls = control.calls.load();
		auto first = cache.get(input, size, compile);
		auto second = cache.get(input, size, compile);
		check(first && second && serialize(*first) == serialize(*second));
		check(control.calls == calls + 2);
		check(cache.getStats().bytes == bytes);
	};

	auto invalid = key_a;
	std::memset(invalid.data() + 4, 0, 16); // Missing checksum.
	check_bypass(invalid.data(), invalid.size());
	invalid = key_a;
	invalid[0] = 'X';
	check_bypass(invalid.data(), invalid.size());
	invalid = key_a;
	set_u32(invalid.data(), 20, 2); // Unsupported container version.
	check_bypass(invalid.data(), invalid.size());
	invalid = key_a;
	set_u32(invalid.data(), 24, invalid.size() - 1);
	check_bypass(invalid.data(), invalid.size());
	invalid = key_a;
	set_u32(invalid.data(), 28, 2); // Chunk-offset table extends past the input.
	check_bypass(invalid.data(), invalid.size());
	for (size_t size = 0; size < 32; size++)
		check_bypass(key_a.data(), size);
	check_bypass(nullptr, 0);
	check(get(cache, key_a));
	check(cache.getStats().misses == 1);
}

static void test_source_not_retained()
{
	CompilerControl control;
	compiler_control = &control;
	constexpr size_t budget = 4096;
	DxbcIrCache cache(budget);
	std::vector<uint8_t> input(budget * 2, 0x5a);
	std::memcpy(input.data(), key_a.data(), key_a.size());
	set_u32(input.data(), 24, input.size());
	auto compact_compile = [](const void *, size_t) {
		compiler_control->calls++;
		return make_ir(key_a.data(), key_a.size(), 0);
	};
	// Large source shaders can fit when only their compact key and IR are retained.
	auto first = cache.get(input.data(), input.size(), compact_compile);
	auto second = cache.get(input.data(), input.size(), compact_compile);
	check(first && second && serialize(*first) == serialize(*second));
	check(control.calls == 1);
	check(cache.getStats().memory_hits == 1);
	check(cache.getStats().evictions == 0);
	check(cache.getStats().bytes > 0 && cache.getStats().bytes <= budget);
}

static void test_stale_checksum_validation()
{
	CompilerControl control;
	compiler_control = &control;
	DxbcIrCache cache(cache_budget, true);
	auto first = get(cache, key_a);
	check(first);
	auto modified = key_a;
	modified.back() ^= 1; // Simulate rewritten bytecode without an updated checksum.
	auto fresh = get(cache, modified);
	check(fresh && serialize(*fresh) == serialize(*make_ir(modified.data(), modified.size(), 0)));
	check(serialize(*fresh) != serialize(*first));
	check(control.calls == 2);
	check(cache.getStats().validation_failures == 1);
	check(cache.getStats().bytes == 0);
}

static void test_cache_instance_lifetime()
{
	CompilerControl control;
	compiler_control = &control;
	std::vector<uint8_t> previous;
	{
		DxbcIrCache cache(cache_budget);
		auto result = get(cache, key_a);
		check(result);
		previous = serialize(*result);
		check(control.calls == 1);
	}

	// A new cache must compile the same input again after the old one is destroyed.
	control.revision = 1;
	DxbcIrCache cache(cache_budget);
	auto first = get(cache, key_a);
	auto second = get(cache, key_a);
	const auto expected = serialize(*make_ir(key_a.data(), key_a.size(), 1));
	check(first && second && first.get() != second.get());
	check(serialize(*first) == expected && serialize(*second) == expected);
	check(expected != previous);
	check(control.calls == 2);
	check(cache.getStats().misses == 1);
	check(cache.getStats().memory_hits == 1);
}

static void test_validation()
{
	CompilerControl control;
	compiler_control = &control;
	DxbcIrCache cache(cache_budget, true);
	auto first = get(cache, key_a);
	auto second = get(cache, key_a);
	check(first && second && serialize(*first) == serialize(*second));
	check(control.calls == 2);
	check(cache.getStats().validation_failures == 0);

	control.revision = 1;
	auto changed = get(cache, key_a);
	check(changed);
	const auto expected = serialize(*make_ir(key_a.data(), key_a.size(), 1));
	check(serialize(*changed) == expected);
	check(serialize(*changed) != serialize(*first));
	check(control.calls == 3);
	check(cache.getStats().validation_failures == 1);

	// Subsequent requests must recover without returning the old cached value.
	auto recovered = get(cache, key_a);
	auto validated = get(cache, key_a);
	check(recovered && validated);
	check(serialize(*recovered) == expected && serialize(*validated) == expected);
	check(control.calls == 5);
	check(cache.getStats().validation_failures == 1);
	check(cache.getStats().bytes > 0);
}

static void test_validation_failure()
{
	CompilerControl control;
	compiler_control = &control;
	DxbcIrCache cache(cache_budget, true);
	check(get(cache, key_a));
	check(cache.getStats().bytes > 0);
	control.fail_next = true;
	check(!get(cache, key_a));
	check(control.calls == 2);
	check(cache.getStats().validation_failures == 1);
	check(cache.getStats().bytes == 0);

	// Failed validation must invalidate the old memory entry even without a replacement.
	control.revision = 1;
	const auto expected = serialize(*make_ir(key_a.data(), key_a.size(), 1));
	auto recovered = get(cache, key_a);
	check(recovered && serialize(*recovered) == expected);
	check(control.calls == 3);
	check(cache.getStats().validation_failures == 1);
	check(cache.getStats().bytes > 0);
	auto validated = get(cache, key_a);
	check(validated && serialize(*validated) == expected);
	check(control.calls == 4);
	check(cache.getStats().validation_failures == 1);
}

int main()
{
	test_memory_and_independence();
	test_same_key_concurrency(false);
	test_same_key_concurrency(true);
	test_different_key_concurrency();
	test_failure_and_bypass();
	test_lru_budget();
	test_header_eligibility();
	test_source_not_retained();
	test_stale_checksum_validation();
	test_cache_instance_lifetime();
	test_validation();
	test_validation_failure();
	std::puts("PASS: DXBC IR cache identity, contents, concurrency, failures, LRU, lifetime and validation.");
	return 0;
}
