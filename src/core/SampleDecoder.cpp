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
#include <QFileInfo>
#include <QString>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sndfile.h>

#ifdef LMMS_HAVE_OGGVORBIS
#include <vorbis/vorbisfile.h>
#endif

#include "AudioEngine.h"
#include "DrumSynth.h"
#include "Engine.h"
#include "lmms_basics.h"

namespace lmms {

namespace {

using Decoder = std::optional<SampleDecoder::Result> (*)(const std::filesystem::path&);

auto decodeSampleSF(const std::filesystem::path& audioFile) -> std::optional<SampleDecoder::Result>;
auto decodeSampleDS(const std::filesystem::path& audioFile) -> std::optional<SampleDecoder::Result>;
#ifdef LMMS_HAVE_OGGVORBIS
auto decodeSampleOggVorbis(const std::filesystem::path& audioFile) -> std::optional<SampleDecoder::Result>;
#endif

static constexpr std::array<Decoder, 3> decoders = {&decodeSampleSF,
#ifdef LMMS_HAVE_OGGVORBIS
	&decodeSampleOggVorbis,
#endif
	&decodeSampleDS};

auto decodeSampleSF(const std::filesystem::path& audioFile) -> std::optional<SampleDecoder::Result>
{
	auto sfInfo = SF_INFO{};

#ifdef LMMS_BUILD_WIN32
	const auto sndFile = sf_wchar_open(audioFile.c_str(), SFM_READ, &sfInfo);
#else
	const auto sndFile = sf_open(audioFile.c_str(), SFM_READ, &sfInfo);
#endif

	if (sf_error(sndFile) != 0) { return std::nullopt; }

	auto buf = std::vector<sample_t>(sfInfo.channels * sfInfo.frames);
	sf_read_float(sndFile, buf.data(), buf.size());

	sf_close(sndFile);

	auto result = std::vector<SampleFrame>(sfInfo.frames);
	for (int i = 0; i < static_cast<int>(result.size()); ++i)
	{
		if (sfInfo.channels == 1)
		{
			// Upmix from mono to stereo
			result[i] = {buf[i], buf[i]};
		}
		else if (sfInfo.channels > 1)
		{
			// TODO: Add support for higher number of channels (i.e., 5.1 channel systems)
			// The current behavior assumes stereo in all cases excluding mono.
			// This may not be the expected behavior, given some audio files with a higher number of channels.
			result[i] = {buf[i * sfInfo.channels], buf[i * sfInfo.channels + 1]};
		}
	}

	return SampleDecoder::Result{std::move(result), static_cast<int>(sfInfo.samplerate)};
}

auto decodeSampleDS(const std::filesystem::path& audioFile) -> std::optional<SampleDecoder::Result>
{
	// Populated by DrumSynth::GetDSFileSamples
	int_sample_t* dataPtr = nullptr;

	auto ds = DrumSynth{};
	const auto engineRate = Engine::audioEngine()->outputSampleRate();
	const auto frames = ds.GetDSFileSamples(audioFile, dataPtr, DEFAULT_CHANNELS, engineRate);
	const auto data = std::unique_ptr<int_sample_t[]>{dataPtr}; // NOLINT, we have to use a C-style array here

	if (frames <= 0 || !data) { return std::nullopt; }

	auto result = std::vector<SampleFrame>(frames);
	src_short_to_float_array(data.get(), &result[0][0], frames * DEFAULT_CHANNELS);

	return SampleDecoder::Result{std::move(result), static_cast<int>(engineRate)};
}

#ifdef LMMS_HAVE_OGGVORBIS
auto decodeSampleOggVorbis(const std::filesystem::path& audioFile) -> std::optional<SampleDecoder::Result>
{
	static auto s_read = [](void* buffer, size_t size, size_t count, void* stream) -> size_t {
		auto file = static_cast<std::ifstream*>(stream);
		file->read(static_cast<char*>(buffer), size * count);
		return file->gcount();
	};

	static auto s_seek = [](void* stream, ogg_int64_t offset, int whence) -> int {
		auto file = static_cast<std::ifstream*>(stream);
		if (whence == SEEK_SET) { file->seekg(offset); }
		else if (whence == SEEK_CUR) { file->seekg(offset, std::ios_base::cur); }
		else if (whence == SEEK_END) { file->seekg(offset, std::ios_base::end); }
		else { return -1; }
		return 0;
	};

	static auto s_close = [](void* stream) -> int {
		auto file = static_cast<std::ifstream*>(stream);
		file->close();
		return 0;
	};

	static auto s_tell = [](void* stream) -> long {
		auto file = static_cast<std::ifstream*>(stream);
		return file->tellg();
	};

	static ov_callbacks s_callbacks = {s_read, s_seek, s_close, s_tell};


	auto fileStream = std::ifstream{audioFile};
	auto vorbisFile = OggVorbis_File{};

	if (!fileStream.is_open() || ov_open_callbacks(&fileStream, &vorbisFile, nullptr, 0, s_callbacks) < 0)
	{
		return std::nullopt;
	}

	const auto vorbisInfo = ov_info(&vorbisFile, -1);
	if (vorbisInfo == nullptr) { return std::nullopt; }

	const auto numChannels = vorbisInfo->channels;
	const auto sampleRate = vorbisInfo->rate;
	const auto numSamples = ov_pcm_total(&vorbisFile, -1);
	if (numSamples < 0) { return std::nullopt; }

	auto buffer = std::vector<float>(numSamples);
	auto output = static_cast<float**>(nullptr);

	auto totalSamplesRead = 0;
	while (true)
	{
		auto samplesRead = ov_read_float(&vorbisFile, &output, numSamples, 0);

		if (samplesRead < 0) { return std::nullopt; }
		else if (samplesRead == 0) { break; }

		std::copy_n(*output, samplesRead, buffer.begin() + totalSamplesRead);
		totalSamplesRead += samplesRead;
	}

	auto result = std::vector<SampleFrame>(totalSamplesRead / numChannels);
	for (auto i = std::size_t{0}; i < result.size(); ++i)
	{
		if (numChannels == 1) { result[i] = {buffer[i], buffer[i]}; }
		else if (numChannels > 1) { result[i] = {buffer[i * numChannels], buffer[i * numChannels + 1]}; }
	}

	ov_clear(&vorbisFile);
	return SampleDecoder::Result{std::move(result), static_cast<int>(sampleRate)};
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

auto SampleDecoder::decode(const std::filesystem::path& audioFile) -> std::optional<Result>
{
	auto result = std::optional<Result>{};
	for (const auto& decoder : decoders)
	{
		result = decoder(audioFile);
		if (result) { break; }
	}

	return result;
}

} // namespace lmms
