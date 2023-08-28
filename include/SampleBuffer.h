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
#include <optional>
#include <samplerate.h>
#include <vector>

#include "AudioEngine.h"
#include "Engine.h"
#include "lmms_basics.h"
#include "lmms_export.h"

namespace lmms {
class LMMS_EXPORT SampleBuffer
{
public:
	using value_type = sampleFrame;
	using reference = sampleFrame&;
	using const_iterator = std::vector<sampleFrame>::const_iterator;
	using const_reverse_iterator = std::vector<sampleFrame>::const_reverse_iterator;
	using difference_type = std::vector<sampleFrame>::difference_type;
	using size_type = std::vector<sampleFrame>::size_type;

	SampleBuffer() = default;
	SampleBuffer(const QString& audioFile);
	SampleBuffer(const QByteArray& base64Data, int sampleRate);
	SampleBuffer(
		const sampleFrame* data, int numFrames, int sampleRate = Engine::audioEngine()->processingSampleRate());

	friend void swap(SampleBuffer& first, SampleBuffer& second) noexcept;
	auto toBase64() const -> QString;

	auto audioFile() const -> QString;
	auto sampleRate() const -> sample_rate_t;

	auto begin() const -> const_iterator;
	auto end() const -> const_iterator;
	auto rbegin() const -> const_reverse_iterator;
	auto rend() const -> const_reverse_iterator;

	auto data() const -> const sampleFrame*;
	auto size() const -> size_type;
	bool empty() const;

private:
	void decodeSampleSF(const QString& fileName);
	void decodeSampleDS(const QString& fileName);

private:
	std::vector<sampleFrame> m_data;
	std::optional<QString> m_audioFile;
	int m_sampleRate = 0;
};

} // namespace lmms

#endif // LMMS_SAMPLE_BUFFER_H
