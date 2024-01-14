/*
 * SampleBuffer.h - container-class SampleBuffer
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#ifndef LMMS_SAMPLE_BUFFER_H
#define LMMS_SAMPLE_BUFFER_H

#include <QByteArray>
#include <QString>
#include <memory>
#include <optional>
#include <samplerate.h>
#include <vector>

#include "AudioEngine.h"
#include "Engine.h"
#include "lmms_basics.h"
#include "lmms_export.h"

namespace lmms {
class LMMS_EXPORT SampleBuffer
	: public std::enable_shared_from_this<SampleBuffer>
{
public:
	using value_type = sampleFrame;
	using reference = sampleFrame&;
	using const_reference = const sampleFrame&;
	using iterator = std::vector<sampleFrame>::iterator;
	using const_iterator = std::vector<sampleFrame>::const_iterator;
	using difference_type = std::vector<sampleFrame>::difference_type;
	using size_type = std::vector<sampleFrame>::size_type;
	using reverse_iterator = std::vector<sampleFrame>::reverse_iterator;
	using const_reverse_iterator = std::vector<sampleFrame>::const_reverse_iterator;

	class Source
	{
	public:
		enum class Type
		{
			Unknown,
			AudioFile,
			Base64
		};

		Source() = default;
		Source(const QString& filePath);
		Source(const QString& base64, sample_rate_t sampleRate);

		/**
		 * A unique string identifying the SampleBuffer's source.
		 *   - For audio files, this is the absolute file path.
		 *   - For base64, this is a string encoding the hash of the base64 data + the sample rate.
		 *   - For anything else, this is empty.
		*/
		auto identifier() const -> const QString& { return m_identifier; }

		auto type() const -> Type { return m_type; }

		auto hash() const -> std::size_t { return m_hash; }

		//! The audio file full path or an empty string
		auto audioFileAbsolute() const -> const QString&;

		//! The audio file relative path or an empty string
		auto audioFileRelative() const -> QString;

		struct Hasher
		{
			auto operator()(const Source& src) const noexcept -> std::size_t
			{
				return src.hash();
			}
		};

		friend auto operator==(const Source& lhs, const Source& rhs) noexcept -> bool
		{
			return lhs.m_type == rhs.m_type
				&& lhs.m_hash == rhs.m_hash
				&& lhs.m_identifier == rhs.m_identifier;
		}

	private:
		Type m_type = Type::Unknown;
		QString m_identifier;
		std::size_t m_hash = 0;
	};

	//! passkey idiom
	class Access
	{
	public:
		friend class SampleBuffer;
		friend class SampleLoader;
		Access(Access&&) = default;
	private:
		Access() {}
		Access(const Access&) = default;
	};

	SampleBuffer() = delete;
	SampleBuffer(Access) {}
	SampleBuffer(Access, const QString& audioFile);
	SampleBuffer(Access, const QString& base64, sample_rate_t sampleRate);
	SampleBuffer(Access, std::vector<sampleFrame> data, sample_rate_t sampleRate);
	SampleBuffer(Access, const sampleFrame* data, int numFrames, sample_rate_t sampleRate);

	static auto create() -> std::shared_ptr<const SampleBuffer>;
	static auto create(const QString& audioFile) -> std::shared_ptr<const SampleBuffer>;
	static auto create(const QString& base64, sample_rate_t sampleRate) -> std::shared_ptr<const SampleBuffer>;
	static auto create(std::vector<sampleFrame> data, sample_rate_t sampleRate)
		-> std::shared_ptr<const SampleBuffer>;
	static auto create(const sampleFrame* data, int numFrames,
		sample_rate_t sampleRate = Engine::audioEngine()->processingSampleRate())
		-> std::shared_ptr<const SampleBuffer>;

	~SampleBuffer() = default;

	friend void swap(SampleBuffer& first, SampleBuffer& second) noexcept;

	auto shared() const -> std::shared_ptr<const SampleBuffer>;

	auto toBase64() const -> QString;

	auto sampleRate() const -> sample_rate_t { return m_sampleRate; }
	auto source() const -> const Source& { return m_source; }

	auto begin() -> iterator { return m_data.begin(); }
	auto end() -> iterator { return m_data.end(); }

	auto begin() const -> const_iterator { return m_data.begin(); }
	auto end() const -> const_iterator { return m_data.end(); }

	auto cbegin() const -> const_iterator { return m_data.cbegin(); }
	auto cend() const -> const_iterator { return m_data.cend(); }

	auto rbegin() -> reverse_iterator { return m_data.rbegin(); }
	auto rend() -> reverse_iterator { return m_data.rend(); }

	auto rbegin() const -> const_reverse_iterator { return m_data.rbegin(); }
	auto rend() const -> const_reverse_iterator { return m_data.rend(); }

	auto crbegin() const -> const_reverse_iterator { return m_data.crbegin(); }
	auto crend() const -> const_reverse_iterator { return m_data.crend(); }

	auto data() const -> const sampleFrame* { return m_data.data(); }
	auto size() const -> size_type { return m_data.size(); }
	auto empty() const -> bool { return m_data.empty(); }

private:
	std::vector<sampleFrame> m_data;
	sample_rate_t m_sampleRate = Engine::audioEngine()->processingSampleRate();
	Source m_source;
};

} // namespace lmms

#endif // LMMS_SAMPLE_BUFFER_H
