/*
 * AudioSampleRecorder.h - device-class that implements recording
 *                         audio-buffers into RAM
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_AUDIO_SAMPLE_RECORDER_H
#define LMMS_AUDIO_SAMPLE_RECORDER_H

#include <QList>
#include <QPair>
#include <memory>

#include "AudioDevice.h"

namespace lmms
{

class SampleBuffer;


class AudioSampleRecorder : public AudioDevice
{
public:
	AudioSampleRecorder( const ch_cnt_t _channels, bool & _success_ful, AudioEngine* audioEngine );
	~AudioSampleRecorder() override;

	f_cnt_t framesRecorded() const;
	std::shared_ptr<const SampleBuffer> createSampleBuffer();

private:
	void writeBuffer(const SampleFrame* _ab, const fpp_t _frames) override;

	using BufferList = QList<QPair<SampleFrame*, fpp_t>>;
	BufferList m_buffers;

} ;

} // namespace lmms

#endif // LMMS_AUDIO_SAMPLE_RECORDER_H
