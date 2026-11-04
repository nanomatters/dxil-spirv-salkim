/* Copyright (c) 2026 Erhan Bilgili
 * SPDX-License-Identifier: MIT
 */
#include "dxbc_ir_cache.hpp"
#include "dxbc/dxbc_container.h"
#include "ir/ir_serialize.h"
#include "logging.hpp"
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <list>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace LLVMBC
{
using namespace dxbc_spv;
using Bytes = std::vector<uint8_t>;
using Blob = std::shared_ptr<const Bytes>;

static Bytes serialize(const ir::Builder &builder)
{
	ir::Serializer serializer(builder);
	Bytes bytes(serializer.computeSerializedSize());
	if (!serializer.serialize(bytes.data(), bytes.size()))
		bytes.clear();
	return bytes;
}

static std::unique_ptr<ir::Builder> deserialize(const Blob &bytes)
{
	if (!bytes || bytes->empty())
		return nullptr;
	auto builder = std::make_unique<ir::Builder>();
	ir::Deserializer decoder(bytes->data(), bytes->size());
	return decoder.deserializeFresh(*builder) ? std::move(builder) : nullptr;
}

struct DxbcIrKey
{
	util::md5::Digest hash;
	size_t size = 0;

	bool operator == (const DxbcIrKey &other) const
	{
		return size == other.size && hash == other.hash;
	}

	struct Hasher
	{
		size_t operator () (const DxbcIrKey &key) const
		{
			size_t result = key.size;
			for (auto byte : key.hash.data)
				result = (result ^ byte) * 16777619u;
			return result;
		}
	};
};

static bool readKey(const void *data, size_t size, DxbcIrKey &key)
{
	if (!data)
		return false;
	util::ByteReader reader(data, size);
	dxbc::FileHeader header;
	if (!reader.read(header.magic) || !reader.read(header.hash.data.size(), header.hash.data.data()) ||
	    !reader.read(header.version) || !reader.read(header.fileSize) || !reader.read(header.chunkCount))
		return false;
	if (header.magic != util::FourCC("DXBC") || header.version != 1 || header.fileSize != size ||
	    header.chunkCount > reader.getRemaining() / sizeof(uint32_t) || header.hash == util::md5::Digest{})
		return false;
	// Like DXVK, use the embedded checksum as shader identity. Frontend options
	// are fixed here, and the checksummed contents already include the stage.
	key = { header.hash, size };
	return true;
}

static std::string digest(const DxbcIrKey &key)
{
	const auto &hash = key.hash;
	std::string text;
	for (auto byte : hash.data)
	{
		text += "0123456789abcdef"[byte >> 4];
		text += "0123456789abcdef"[byte & 15];
	}
	return text;
}

struct DxbcIrCache::Impl
{
	struct Entry
	{
		DxbcIrKey key;
		Blob bytes;
		std::condition_variable ready_cond;
		bool ready = false;
		std::list<std::shared_ptr<Entry>>::iterator lru;
		size_t charge() const
		{
			assert(bytes);
			// Include retained IR capacity, with an allowance for container overhead.
			return sizeof(Entry) + bytes->capacity() + 96;
		}
	};

	const size_t max_bytes;
	const bool validate;
	std::mutex mutex;
	std::unordered_map<DxbcIrKey, std::shared_ptr<Entry>, DxbcIrKey::Hasher> entries;
	std::list<std::shared_ptr<Entry>> lru;
	Stats stats;

	Impl(size_t max_bytes_, bool validate_)
		: max_bytes(max_bytes_), validate(validate_) {}

	void erase(std::shared_ptr<Entry> entry)
	{
		stats.bytes -= entry->charge();
		entries.erase(entry->key);
		lru.erase(entry->lru);
	}
};

DxbcIrCache::DxbcIrCache(size_t max_bytes, bool validate)
	: impl(std::make_unique<Impl>(max_bytes, validate)) {}
DxbcIrCache::~DxbcIrCache() = default;

DxbcIrCache::Stats DxbcIrCache::getStats()
{
	std::lock_guard<std::mutex> lock(impl->mutex);
	return impl->stats;
}

std::unique_ptr<ir::Builder> DxbcIrCache::get(const void *data, size_t size, Compiler compile)
{
	auto &cache = *impl;
	auto run_compiler = [&] {
		{
			std::lock_guard<std::mutex> lock(cache.mutex);
			cache.stats.compiles++;
		}
		return compile(data, size);
	};
	DxbcIrKey key;
	if (!cache.max_bytes || !readKey(data, size, key))
		return run_compiler();
	std::shared_ptr<Impl::Entry> entry;
	Blob bytes;
	bool producer = false;
	{
		std::unique_lock<std::mutex> lock(cache.mutex);
		auto itr = cache.entries.find(key);
		if (itr == cache.entries.end())
		{
			entry = std::make_shared<Impl::Entry>();
			entry->key = key;
			cache.entries.emplace(key, entry);
			producer = true;
		}
		else
		{
			entry = itr->second;
			if (!entry->ready)
			{
				cache.stats.waits++;
				entry->ready_cond.wait(lock, [&] { return entry->ready; });
			}
			bytes = entry->bytes;
			// An oversized or failed entry may already have left the map.
			auto current = cache.entries.find(key);
			if (bytes && current != cache.entries.end() && current->second == entry)
				cache.lru.splice(cache.lru.begin(), cache.lru, entry->lru);
			if (bytes)
				cache.stats.memory_hits++;
		}
	}

	auto builder = deserialize(bytes);
	bool hit = bool(builder);
	bool mismatch = false;
	if (builder && cache.validate)
	{
		auto fresh = run_compiler();
		// Check the reconstructed IR as well as the stored bytes. SSA numbering
		// can change during deserialization, but serialization is canonical.
		const auto reconstructed = serialize(*builder);
		mismatch = !fresh || reconstructed != serialize(*fresh) || reconstructed != *bytes;
		if (mismatch)
		{
			builder = std::move(fresh);
			hit = false;
			LOGE("DXBC IR cache validation failed for %s; using fresh translation.\n", digest(key).c_str());
		}
	}
	if (!hit && !mismatch)
		builder = run_compiler();
	if (producer && !hit)
	{
		bytes = builder ? std::make_shared<Bytes>(serialize(*builder)) : nullptr;
		if (bytes && bytes->empty())
			bytes.reset();
	}

	{
		std::lock_guard<std::mutex> lock(cache.mutex);
		cache.stats.validation_failures += mismatch;
		if (!hit)
			cache.stats.misses++;
		if (producer)
		{
			entry->bytes = bytes;
			entry->ready = true;
			if (bytes && entry->charge() <= cache.max_bytes)
			{
				while (cache.stats.bytes > cache.max_bytes - entry->charge())
				{
					cache.erase(cache.lru.back());
					cache.stats.evictions++;
				}
				cache.lru.push_front(entry);
				entry->lru = cache.lru.begin();
				cache.stats.bytes += entry->charge();
			}
			else
				cache.entries.erase(key);
			entry->ready_cond.notify_all();
		}
		else if (!hit)
		{
			auto current = cache.entries.find(key);
			if (current != cache.entries.end() && current->second == entry)
				cache.erase(entry);
		}
	}
	return builder;
}

static bool enabled(const char *name)
{
	const char *value = std::getenv(name);
	return value && !strcmp(value, "1");
}

static size_t cache_size()
{
	if (const char *value = std::getenv("DXIL_SPIRV_DXBC_CACHE"))
		if (!strcmp(value, "0"))
			return 0;
	if (const char *value = std::getenv("DXIL_SPIRV_DXBC_CACHE_MB"))
	{
		char *end;
		auto mb = strtoul(value, &end, 10);
		if (*value && !*end && mb <= 4096 && mb <= SIZE_MAX / (1024 * 1024))
			return size_t(mb) * 1024 * 1024;
	}
	return 128 * 1024 * 1024;
}

std::unique_ptr<ir::Builder> getCachedDxbcIr(const void *data, size_t size, DxbcIrCache::Compiler compile)
{
	static DxbcIrCache cache(cache_size(), enabled("DXIL_SPIRV_DXBC_CACHE_VALIDATE"));
	static const bool trace = enabled("DXIL_SPIRV_DXBC_CACHE_STATS");
	auto start = trace ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};
	auto builder = cache.get(data, size, compile);
	if (trace)
	{
		DxbcIrKey key;
		readKey(data, size, key);
		const auto stats = cache.getStats();
		const auto us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count();
		LOGI("DXBC IR cache %s: %lld us, memory=%llu miss=%llu compile=%llu wait=%llu invalid=%llu evict=%llu bytes=%zu\n",
		     digest(key).c_str(), static_cast<long long>(us),
		     static_cast<unsigned long long>(stats.memory_hits),
		     static_cast<unsigned long long>(stats.misses), static_cast<unsigned long long>(stats.compiles),
		     static_cast<unsigned long long>(stats.waits),
		     static_cast<unsigned long long>(stats.validation_failures), static_cast<unsigned long long>(stats.evictions), stats.bytes);
	}
	return builder;
}
}
