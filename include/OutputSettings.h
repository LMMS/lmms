/*
 * OutputSettings.h - Stores the settings for file rendering
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef OUTPUT_SETTINGS_H
#define OUTPUT_SETTINGS_H


class OutputSettings
{
public:
	enum BitDepth
	{
		Depth_16Bit,
		Depth_24Bit,
		Depth_32Bit,
		NumDepths
	};

	class BitRateSettings
	{
	public:
		BitRateSettings(bitrate_t bitRate, bool isVariableBitRate) :
			m_bitRate(bitRate),
			m_isVariableBitRate(isVariableBitRate)
		{}

		bool isVariableBitRate() const { return m_isVariableBitRate; }
		void setVariableBitrate(bool variableBitRate = true) { m_isVariableBitRate = variableBitRate; }

		bitrate_t getBitRate() const { return m_bitRate; }
		void setBitRate(bitrate_t bitRate) { m_bitRate = bitRate; }

	private:
		bitrate_t m_bitRate;
		bool m_isVariableBitRate;
	};

public:
	OutputSettings( sample_rate_t sampleRate,
			BitRateSettings const & bitRateSettings,
			BitDepth bitDepth ) :
		m_sampleRate(sampleRate),
		m_bitRateSettings(bitRateSettings),
		m_bitDepth(bitDepth)
	{
	}

	sample_rate_t getSampleRate() const { return m_sampleRate; }
	void setSampleRate(sample_rate_t sampleRate) { m_sampleRate = sampleRate; }

	BitRateSettings const & getBitRateSettings() const { return m_bitRateSettings; }
	void setBitRateSettings(BitRateSettings const & bitRateSettings) { m_bitRateSettings = bitRateSettings; }

	BitDepth getBitDepth() const { return m_bitDepth; }
	void setBitDepth(BitDepth bitDepth) { m_bitDepth = bitDepth; }

private:
	sample_rate_t m_sampleRate;
	BitRateSettings m_bitRateSettings;
	BitDepth m_bitDepth;
};

#endif
