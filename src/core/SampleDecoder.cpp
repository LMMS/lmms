#include "SampleDecoder.h"

#include <QFile>
#include <QFileInfo>
#include <QString>
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

// Used to assign priority to decoders for files with certain extensions
std::unordered_map<const char*, SampleDecoder::Decoder> SampleDecoder::s_extensionMap
	= {{".ds", &SampleDecoder::decodeSampleDS}
#ifdef LMMS_HAVE_OGGVORBIS
		,
		{".ogg", &SampleDecoder::decodeSampleOggVorbis}
#endif
};

// Collection of decoders. Ordered in favor for audio files that we do not know the codec for.
std::vector<SampleDecoder::Decoder> SampleDecoder::s_decoders = {&SampleDecoder::decodeSampleSF
#ifdef LMMS_HAVE_OGGVORBIS
	,
	&SampleDecoder::decodeSampleOggVorbis
#endif
	,
	&SampleDecoder::decodeSampleDS};

auto SampleDecoder::decode(const QString& audioFile) -> Result
{
	const auto fileExtension = QFileInfo{audioFile}.suffix();
	auto it = std::find_if(s_extensionMap.begin(), s_extensionMap.end(), [&](auto entry) {
		auto [key, _] = entry;
		return fileExtension == key;
	});

	if (it != s_extensionMap.end())
	{
		try
		{
			auto decoder = it->second;
			return decoder(audioFile);
		}
		catch (...)
		{
		}
	}

	for (auto decoder : s_decoders)
	{
		if (it != s_extensionMap.end() && decoder.target<Decoder>() == it->second.target<Decoder>()) { continue; }
		try
		{
			return decoder(audioFile);
		}
		catch (...)
		{
			continue;
		}
	}

	throw std::runtime_error{"Decoding failure: Unknown audio codec or error in decoding audio file"};
}

auto SampleDecoder::decodeSampleSF(const QString& audioFile) -> Result
{
	SNDFILE* sndFile = nullptr;
	auto sfInfo = SF_INFO{};

	// Use QFile to handle unicode file names on Windows
	auto file = QFile{audioFile};
	if (!file.open(QIODevice::ReadOnly))
	{
		throw std::runtime_error{
			"Failed to open sample " + audioFile.toStdString() + ": " + file.errorString().toStdString()};
	}

	sndFile = sf_open_fd(file.handle(), SFM_READ, &sfInfo, false);
	if (sf_error(sndFile) != 0)
	{
		throw std::runtime_error{"Failure opening audio handle: " + std::string{sf_strerror(sndFile)}};
	}

	auto buf = std::vector<sample_t>(sfInfo.channels * sfInfo.frames);
	sf_read_float(sndFile, buf.data(), buf.size());

	sf_close(sndFile);
	file.close();

	auto result = std::vector<sampleFrame>(sfInfo.frames);
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

	return {std::move(result), static_cast<int>(sfInfo.samplerate)};
}

auto SampleDecoder::decodeSampleDS(const QString& audioFile) -> Result
{
	// Populated by DrumSynth::GetDSFileSamples
	int_sample_t* dataPtr = nullptr;

	auto ds = DrumSynth{};
	const auto engineRate = Engine::audioEngine()->processingSampleRate();
	const auto frames = ds.GetDSFileSamples(audioFile, dataPtr, DEFAULT_CHANNELS, engineRate);
	const auto data = std::unique_ptr<int_sample_t[]>{dataPtr}; // NOLINT, we have to use a C-style array here

	if (frames <= 0 || !data) { throw std::runtime_error{"Decoding failure: failed to decode DrumSynth file."}; }

	auto result = std::vector<sampleFrame>(frames);
	src_short_to_float_array(data.get(), &result[0][0], frames * DEFAULT_CHANNELS);

	return {std::move(result), static_cast<int>(engineRate)};
}

#ifdef LMMS_HAVE_OGGVORBIS
auto SampleDecoder::decodeSampleOggVorbis(const QString& audioFile) -> Result
{
	auto vorbisFile = OggVorbis_File{};
	const auto openError = ov_fopen(audioFile.toLocal8Bit(), &vorbisFile);

	if (openError != 0)
	{
		throw std::runtime_error{"Decoding failure for Ogg/Vorbis: error code " + std::to_string(openError)};
	}

	const auto vorbisInfo = ov_info(&vorbisFile, -1);
	const auto numChannels = vorbisInfo->channels;
	const auto sampleRate = vorbisInfo->rate;
	const auto numSamples = ov_pcm_total(&vorbisFile, -1);

	auto buffer = std::vector<float>(numSamples);
	auto output = static_cast<float**>(nullptr);

	auto totalSamplesRead = 0;
	while (true)
	{
		auto samplesRead = ov_read_float(&vorbisFile, &output, numSamples, 0);

		if (samplesRead < 0)
		{
			throw std::runtime_error{"Decoding failure for Ogg/Vorbis: error code " + std::to_string(openError)};
		}
		else if (samplesRead == 0) { break; }

		std::copy_n(*output, samplesRead, buffer.begin() + totalSamplesRead);
		totalSamplesRead += samplesRead;
	}

	ov_clear(&vorbisFile);
	auto result = std::vector<sampleFrame>(numSamples / numChannels);
	for (int i = 0; i < buffer.size(); ++i)
	{
		if (numChannels == 1) { result[i] = {buffer[i], buffer[i]}; }
		else if (numChannels > 1) { result[i] = {buffer[i * numChannels], buffer[i * numChannels + 1]}; }
	}

	return {std::move(result), static_cast<int>(sampleRate)};
}
#endif // LMMS_HAVE_OGGVORBIS
} // namespace lmms
