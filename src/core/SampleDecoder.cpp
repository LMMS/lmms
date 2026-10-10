/*
 * SampleDecoder.cpp - Decodes audio files in various formats
 *
 * Copyright (c) 2023 saker <sakertooth@gmail.com>
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

#include "SampleDecoder.h"

#include <QFile>
#include <QString>
#include <memory>
#include <sndfile.h>
#include "SampleBuffer.h"

#ifdef LMMS_HAVE_OGGVORBIS
#include <vorbis/vorbisfile.h>
#endif

#include "AudioEngine.h"
#include "DrumSynth.h"
#include "Engine.h"
#include "LmmsTypes.h"

namespace lmms {

namespace {

using Decoder = std::optional<SampleBuffer> (*)(const QString&);

auto decodeSampleSF(const QString& audioFile) -> std::optional<SampleBuffer>;
auto decodeSampleDS(const QString& audioFile) -> std::optional<SampleBuffer>;
#ifdef LMMS_HAVE_OGGVORBIS
auto decodeSampleOggVorbis(const QString& audioFile) -> std::optional<SampleBuffer>;
#endif

static constexpr std::array<Decoder, 3> decoders = {&decodeSampleSF,
#ifdef LMMS_HAVE_OGGVORBIS
	&decodeSampleOggVorbis,
#endif
	&decodeSampleDS};

auto decodeSampleSF(const QString& audioFile) -> std::optional<SampleBuffer>
{
	auto sfinfo = SF_INFO{};

#ifndef LMMS_BUILD_WIN32
	const auto utf8Path = audioFile.toUtf8();
	const auto sndfile = sf_open(utf8Path.data(), SFM_READ, &sfinfo);
#else
	const auto utf16Path = audioFile.toStdWString();
	const auto sndfile = sf_wchar_open(utf16Path.c_str(), SFM_READ, &sfinfo);
#endif

	if (!sndfile || sf_error(sndfile) != 0) { return std::nullopt; }

	auto buffer = SampleBuffer{static_cast<ch_cnt_t>(sfinfo.channels), static_cast<f_cnt_t>(sfinfo.frames),
		static_cast<sample_rate_t>(sfinfo.samplerate), audioFile};

	sf_readf_float(sndfile, buffer.data(), buffer.frames());
	sf_close(sndfile);
	return buffer;
}

auto decodeSampleDS(const QString& audioFile) -> std::optional<SampleBuffer>
{
	// Populated by DrumSynth::GetDSFileSamples
	int_sample_t* dataPtr = nullptr;

	auto ds = DrumSynth{};
	const auto engineRate = Engine::audioEngine()->outputSampleRate();
	const auto frames = ds.GetDSFileSamples(audioFile, dataPtr, DEFAULT_CHANNELS, engineRate);
	const auto data = std::unique_ptr<int_sample_t[]>{dataPtr}; // NOLINT, we have to use a C-style array here

	if (frames <= 0 || !data) { return std::nullopt; }

	auto result = SampleBuffer{2, static_cast<f_cnt_t>(frames), engineRate, audioFile};
	src_short_to_float_array(data.get(), &result[0][0], frames * DEFAULT_CHANNELS);

	return result;
}

#ifdef LMMS_HAVE_OGGVORBIS
auto decodeSampleOggVorbis(const QString& audioFile) -> std::optional<SampleBuffer>
{
	auto vorbisFile = OggVorbis_File{};

#ifndef LMMS_BUILD_WIN32
	const auto utf8Path = audioFile.toUtf8();
	const auto file = fopen(utf8Path.data(), "rb");
#else
	const auto utf16Path = audioFile.toStdWString();
	const auto file = fopen(utf16Path.c_str(), L"rb");
#endif

	if (!file) { return std::nullopt; }
	if (ov_open_callbacks(file, &vorbisFile, nullptr, 0, OV_CALLBACKS_DEFAULT) < 0) { return std::nullopt; }

	const auto vorbisInfo = ov_info(&vorbisFile, -1);
	if (vorbisInfo == nullptr) { return std::nullopt; }

	const auto numChannels = static_cast<ch_cnt_t>(vorbisInfo->channels);
	const auto sampleRate = static_cast<sample_rate_t>(vorbisInfo->rate);
	const auto numFrames = static_cast<f_cnt_t>(ov_pcm_total(&vorbisFile, 0));

	auto buffer = SampleBuffer{numChannels, numFrames, sampleRate, audioFile};
	auto output = static_cast<float**>(nullptr);
	auto currentSection = 0;
	auto totalFramesRead = 0;
	auto framesRead = 0;

	while ((framesRead = ov_read_float(&vorbisFile, &output, numFrames - totalFramesRead, &currentSection)) > 0)
	{
		const auto info = ov_info(&vorbisFile, currentSection);

		// Vorbis files can contain multiple bitstreams of different channels and sample rates
		// We ignore any bitstreams that differ in channel count and sample rate from the first one seen
		if (info->channels != numChannels || info->rate != sampleRate) { break; }

		for (auto channel = 0; channel < numChannels; ++channel)
		{
			for (auto frame = 0; frame < framesRead; ++frame)
			{
				buffer[totalFramesRead + frame][channel] = output[channel][frame];
			}
		}

		totalFramesRead += framesRead;
	}

	ov_clear(&vorbisFile);
	return buffer;
}
#endif // LMMS_HAVE_OGGVORBIS
} // namespace

auto SampleDecoder::supportedAudioTypes() -> const std::vector<AudioType>&
{
	static const auto s_audioTypes = [] {
		auto types = std::vector<AudioType>();

		// Add DrumSynth by default since that support comes from us
		types.push_back(AudioType{"DrumSynth", "ds"});

		auto sfFormatInfo = SF_FORMAT_INFO{};
		auto simpleTypeCount = 0;
		sf_command(nullptr, SFC_GET_SIMPLE_FORMAT_COUNT, &simpleTypeCount, sizeof(int));

		// TODO: Ideally, this code should be iterating over the major formats, but some important extensions such as
		// *.ogg are not included. This is planned for future versions of sndfile.
		for (int simple = 0; simple < simpleTypeCount; ++simple)
		{
			sfFormatInfo.format = simple;
			sf_command(nullptr, SFC_GET_SIMPLE_FORMAT, &sfFormatInfo, sizeof(sfFormatInfo));

			auto it = std::find_if(types.begin(), types.end(),
				[&](const AudioType& type) { return sfFormatInfo.extension == type.extension; });
			if (it != types.end()) { continue; }

			auto name = std::string{sfFormatInfo.extension};
			std::transform(name.begin(), name.end(), name.begin(), [](unsigned char ch) { return std::toupper(ch); });

			types.push_back(AudioType{std::move(name), sfFormatInfo.extension});
		}

		std::sort(types.begin(), types.end(), [&](const AudioType& a, const AudioType& b) { return a.name < b.name; });
		return types;
	}();
	return s_audioTypes;
}

auto SampleDecoder::decode(const QString& audioFile) -> std::optional<SampleBuffer>
{
	auto result = std::optional<SampleBuffer>{};
	for (const auto& decoder : decoders)
	{
		result = decoder(audioFile);
		if (result) { break; }
	}

	return result;
}

} // namespace lmms
