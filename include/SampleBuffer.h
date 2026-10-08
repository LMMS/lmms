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

#include "LmmsTypes.h"
#include "SampleFrame.h"
#include "lmms_export.h"

namespace lmms {
class LMMS_EXPORT SampleBuffer
{
public:
	static constexpr auto DefaultSampleRate = sample_rate_t{44100};

	SampleBuffer() = default;
	SampleBuffer(f_cnt_t frames, sample_rate_t sampleRate, const QString& audioFile = "");

	auto operator[](f_cnt_t index) -> SampleFrame& { return m_data[index]; }
	auto operator[](f_cnt_t index) const -> const SampleFrame& { return m_data[index]; }

	auto begin() { return m_data.begin(); }
	auto begin() const { return m_data.begin(); }

	auto end() { return m_data.end(); }
	auto end() const { return m_data.end(); }

	auto toBase64() const -> QString;

	auto audioFile() const -> const QString& { return m_audioFile; }
	auto sampleRate() const -> sample_rate_t { return m_sampleRate; }

	auto data() -> SampleFrame* { return m_data.data(); }
	auto data() const -> const SampleFrame* { return m_data.data(); }

	auto size() const -> f_cnt_t { return m_data.size(); }
	auto empty() const -> bool { return m_data.empty(); }

	static auto emptyBuffer() -> std::shared_ptr<const SampleBuffer>;

	static std::shared_ptr<const SampleBuffer> fromFile(const QString& path);
	static std::shared_ptr<const SampleBuffer> fromBase64(const QString& str, sample_rate_t sampleRate = DefaultSampleRate);

private:
	std::vector<SampleFrame> m_data;
	QString m_audioFile;
	sample_rate_t m_sampleRate{};
};

} // namespace lmms

#endif // LMMS_SAMPLE_BUFFER_H
