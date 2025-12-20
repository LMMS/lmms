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
	SampleBuffer() = default;
	explicit SampleBuffer(const QString& audioFile);
	SampleBuffer(const QString& base64, int sampleRate);
	SampleBuffer(std::vector<SampleFrame> data, int sampleRate);
	SampleBuffer(
		const SampleFrame* data, size_t numFrames, int sampleRate = Engine::audioEngine()->outputSampleRate());

	auto toBase64() const -> QString;

	auto audioFile() const -> const QString& { return m_audioFile; }
	auto sampleRate() const -> sample_rate_t { return m_sampleRate; }

	auto data() const -> const SampleFrame* { return m_data->data(); }
	auto size() const -> std::size_t { return m_data->size(); }
	auto empty() const -> bool { return m_data->empty(); }

private:
	static auto emptyData() -> std::shared_ptr<std::vector<SampleFrame>>;
	std::shared_ptr<std::vector<SampleFrame>> m_data = emptyData();
	QString m_audioFile;
	sample_rate_t m_sampleRate = Engine::audioEngine()->outputSampleRate();
};

} // namespace lmms

#endif // LMMS_SAMPLE_BUFFER_H
