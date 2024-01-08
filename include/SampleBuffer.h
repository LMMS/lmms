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

	enum class Source
	{
		Unknown,
		AudioFile,
		Base64
	};

	class Access
	{
	private:
		Access() = default;
		Access(const Access&) = default;
		friend class SampleCache;
	};

	SampleBuffer() = delete;
	SampleBuffer(Access) {}
	SampleBuffer(Access, const QString& audioFile);
	SampleBuffer(Access, const QString& base64, int sampleRate);
	SampleBuffer(Access, std::vector<sampleFrame> data, int sampleRate);
	SampleBuffer(Access, const sampleFrame* data, int numFrames, int sampleRate);

	static auto create() -> std::shared_ptr<const SampleBuffer>;
	static auto create(const QString& audioFile) -> std::shared_ptr<const SampleBuffer>;
	static auto create(const QString& base64, int sampleRate) -> std::shared_ptr<const SampleBuffer>;
	static auto create(std::vector<sampleFrame> data, int sampleRate) -> std::shared_ptr<const SampleBuffer>;
	static auto create(const sampleFrame* data, int numFrames,
		int sampleRate = Engine::audioEngine()->processingSampleRate()) -> std::shared_ptr<const SampleBuffer>;

	~SampleBuffer() = default;

	friend void swap(SampleBuffer& first, SampleBuffer& second) noexcept;

	auto get() const -> std::shared_ptr<const SampleBuffer>;

	auto toBase64() const -> QString;

	auto source() const -> const QString& { return m_source; }
	auto sourceType() const -> Source { return m_sourceType; }
	auto audioFileAbsolute() const -> const QString&;
	auto audioFileRelative() const -> QString; //!< use when saving to project files
	auto base64() const -> const QString&;
	auto sampleRate() const -> sample_rate_t { return m_sampleRate; }

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
	QString m_source; //!< absolute audio file path or base64 data
	Source m_sourceType = Source::Unknown;
	sample_rate_t m_sampleRate = Engine::audioEngine()->processingSampleRate();
};

} // namespace lmms

#endif // LMMS_SAMPLE_BUFFER_H
