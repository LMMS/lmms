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

#include <QString>
#include <memory>
#include <vector>

#include "AudioEngine.h"
#include "Engine.h"
#include "LmmsTypes.h"
#include "lmms_export.h"

namespace lmms {
class LMMS_EXPORT SampleBuffer
{
public:
	using value_type = SampleFrame;
	using reference = SampleFrame&;
	using const_reference = const SampleFrame&;
	using iterator = std::vector<SampleFrame>::iterator;
	using const_iterator = std::vector<SampleFrame>::const_iterator;
	using difference_type = std::vector<SampleFrame>::difference_type;
	using size_type = std::vector<SampleFrame>::size_type;
	using reverse_iterator = std::vector<SampleFrame>::reverse_iterator;
	using const_reverse_iterator = std::vector<SampleFrame>::const_reverse_iterator;

	SampleBuffer() = default;
	explicit SampleBuffer(const QString& audioFile);
	SampleBuffer(const QString& base64, int sampleRate);
	SampleBuffer(std::vector<SampleFrame> data, int sampleRate);
	SampleBuffer(
		const SampleFrame* data, size_t numFrames, int sampleRate = Engine::audioEngine()->outputSampleRate());

	friend void swap(SampleBuffer& first, SampleBuffer& second) noexcept;
	auto toBase64() const -> QString;

	auto audioFile() const -> const QString& { return m_audioFile; }
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

	auto data() const -> const SampleFrame* { return m_data.data(); }
	auto size() const -> size_type { return m_data.size(); }
	auto empty() const -> bool { return m_data.empty(); }

	static auto emptyBuffer() -> std::shared_ptr<const SampleBuffer>;

private:
	std::vector<SampleFrame> m_data;
	QString m_audioFile;
	sample_rate_t m_sampleRate = Engine::audioEngine()->outputSampleRate();
};

} // namespace lmms

#endif // LMMS_SAMPLE_BUFFER_H
