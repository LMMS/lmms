/*
 * OutputSettings.h - Stores the settings for file rendering
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2017 Michael Gregorius <michael.gregorius.git/at/arcor[dot]de>
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

#ifndef LMMS_OUTPUT_SETTINGS_H
#define LMMS_OUTPUT_SETTINGS_H

#include "LmmsTypes.h"

namespace lmms
{


class OutputSettings
{
public:
	enum class BitDepth
	{
		Depth16Bit,
		Depth24Bit,
		Depth32Bit
	};

	enum class StereoMode
	{
		Stereo,
		JointStereo,
		Mono
	};

public:
	OutputSettings(sample_rate_t sampleRate, bitrate_t bitRate, BitDepth bitDepth, StereoMode stereoMode)
		: m_sampleRate(sampleRate)
		, m_bitRate(bitRate)
		, m_bitDepth(bitDepth)
		, m_stereoMode(stereoMode)
		, m_compressionLevel(0.625) // 5/8
	{
	}

	OutputSettings(sample_rate_t sampleRate, bitrate_t bitRate, BitDepth bitDepth)
		: OutputSettings(sampleRate, bitRate, bitDepth, StereoMode::Stereo)
	{
	}

	sample_rate_t getSampleRate() const { return m_sampleRate; }
	void setSampleRate(sample_rate_t sampleRate) { m_sampleRate = sampleRate; }

	bitrate_t bitrate() const { return m_bitRate; }
	void setBitrate(bitrate_t bitrate) { m_bitRate = bitrate; }

	BitDepth getBitDepth() const { return m_bitDepth; }
	void setBitDepth(BitDepth bitDepth) { m_bitDepth = bitDepth; }

	StereoMode getStereoMode() const { return m_stereoMode; }
	void setStereoMode(StereoMode stereoMode) { m_stereoMode = stereoMode; }


	double getCompressionLevel() const{ return m_compressionLevel; }
	void setCompressionLevel(double level){
		// legal range is 0.0 to 1.0.
		m_compressionLevel = level;
	}

private:
	sample_rate_t m_sampleRate;
	bitrate_t m_bitRate;
	BitDepth m_bitDepth;
	StereoMode m_stereoMode;
	double m_compressionLevel;
};


} // namespace lmms

#endif // LMMS_OUTPUT_SETTINGS_H
