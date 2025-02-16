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

#include <vector>

#include "AudioEngine.h"
#include "Engine.h"
#include "ResourceCache.h"
#include "SampleFrame.h"
#include "lmms_export.h"

namespace lmms {

class LMMS_EXPORT SampleBuffer : public ResourceCache::Resource
{
public:
	SampleBuffer() = default;
	explicit SampleBuffer(const std::filesystem::path& path);
	SampleBuffer(const std::string& base64, int sampleRate = Engine::audioEngine()->outputSampleRate());
	SampleBuffer(const SampleFrame* buffer, size_t size, int sampleRate);
	SampleBuffer(std::vector<SampleFrame> buffer, int sampleRate);

	std::string toBase64() const;

	int sampleRate() const { return m_sampleRate; }
	void setSampleRate(int sampleRate) { m_sampleRate = sampleRate; }

	auto path() const -> const std::filesystem::path& { return m_path; }

	auto data() const -> const std::vector<SampleFrame>& { return m_data; }
	auto data() -> std::vector<SampleFrame>& { return m_data; }

	auto size() const { return m_data.size(); }
	auto empty() const { return m_data.empty(); }

	auto begin() const { return m_data.begin(); }
	auto begin() { return m_data.begin(); }

	auto end() const { return m_data.end(); }
	auto end() { return m_data.end(); }

private:
	std::vector<SampleFrame> m_data;
	std::filesystem::path m_path;
	int m_sampleRate = 0;
};

} // namespace lmms

#endif // LMMS_SAMPLE_BUFFER_H
