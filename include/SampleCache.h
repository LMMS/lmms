#ifndef LMMS_SAMPLE_CACHE_H
#define LMMS_SAMPLE_CACHE_H

#include <filesystem>

#include "PathUtil.h"
#include "SampleBuffer.h"
#include "SampleThumbnailCache.h"

namespace lmms {
class SampleCache
{
public:
	static auto fetchBuffer(const std::filesystem::path& path) -> std::shared_ptr<const SampleBuffer>
	{
		auto [it, inserted] = s_buffers.try_emplace(path, SampleBufferEntry{});
		auto& entry = it->second;
		const auto lastWriteTime = std::filesystem::last_write_time(path);

		if (!inserted && entry.lastWriteTime == lastWriteTime)
		{
			const auto resource = entry.resource.lock();
			if (resource) { return resource; }
		}

		const auto resource = SampleBuffer::fromFile(PathUtil::pathToQString(path));
		if (!resource) { return nullptr; }

		entry.resource = resource;
		entry.lastWriteTime = lastWriteTime;
		return resource;
	}

	static auto fetchThumbnail(const std::filesystem::path& path) -> std::shared_ptr<const SampleThumbnailCache>
	{
		auto [it, inserted] = s_thumbnailCaches.try_emplace(path, SampleThumbnailCacheEntry{});
		auto& entry = it->second;
		const auto lastWriteTime = std::filesystem::last_write_time(path);

		if (!inserted && entry.lastWriteTime == lastWriteTime)
		{
			const auto resource = entry.resource.lock();
			if (resource) { return resource; }
		}

		auto buffer = fetchBuffer(path);
		if (!buffer) { return nullptr; }

		const auto resource = std::make_shared<const SampleThumbnailCache>(std::move(*buffer));
		entry.resource = std::make_shared<const SampleThumbnailCache>(std::move(*resource));
		entry.lastWriteTime = lastWriteTime;
		return resource;
	}

private:
	struct SampleBufferEntry
	{
		std::weak_ptr<const SampleBuffer> resource;
		std::filesystem::file_time_type lastWriteTime;
	};

	struct SampleThumbnailCacheEntry
	{
		std::weak_ptr<const SampleThumbnailCache> resource;
		std::filesystem::file_time_type lastWriteTime;
	};

	inline static std::unordered_map<std::filesystem::path, SampleBufferEntry> s_buffers;
	inline static std::unordered_map<std::filesystem::path, SampleThumbnailCacheEntry> s_thumbnailCaches;
};
} // namespace lmms

#endif // LMMS_SAMPLE_CACHE_H