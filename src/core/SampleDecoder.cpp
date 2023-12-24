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

namespace {

using Decoder = std::optional<SampleDecoder::Result>(*)(const QString&);

auto decodeSampleSF(const QString& audioFile) -> std::optional<SampleDecoder::Result>;
auto decodeSampleDS(const QString& audioFile) -> std::optional<SampleDecoder::Result>;
#ifdef LMMS_HAVE_OGGVORBIS
auto decodeSampleOggVorbis(const QString& audioFile) -> std::optional<SampleDecoder::Result>;
#endif

static constexpr std::array<Decoder, 3> decoders = {&decodeSampleSF,
#ifdef LMMS_HAVE_OGGVORBIS
	&decodeSampleOggVorbis,
#endif
	&decodeSampleDS};

auto decodeSampleSF(const QString& audioFile) -> std::optional<SampleDecoder::Result>
{
	SNDFILE* sndFile = nullptr;
	auto sfInfo = SF_INFO{};

	// Use QFile to handle unicode file names on Windows
	auto file = QFile{audioFile};
	if (!file.open(QIODevice::ReadOnly)) { return std::nullopt; }

	sndFile = sf_open_fd(file.handle(), SFM_READ, &sfInfo, false);
	if (sf_error(sndFile) != 0) { return std::nullopt; }

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

	return SampleDecoder::Result{std::move(result), static_cast<int>(sfInfo.samplerate)};
}

auto decodeSampleDS(const QString& audioFile) -> std::optional<SampleDecoder::Result>
{
	// Populated by DrumSynth::GetDSFileSamples
	int_sample_t* dataPtr = nullptr;

	auto ds = DrumSynth{};
	const auto engineRate = Engine::audioEngine()->processingSampleRate();
	const auto frames = ds.GetDSFileSamples(audioFile, dataPtr, DEFAULT_CHANNELS, engineRate);
	const auto data = std::unique_ptr<int_sample_t[]>{dataPtr}; // NOLINT, we have to use a C-style array here

	if (frames <= 0 || !data) { return std::nullopt; }

	auto result = std::vector<sampleFrame>(frames);
	src_short_to_float_array(data.get(), &result[0][0], frames * DEFAULT_CHANNELS);

	return SampleDecoder::Result{std::move(result), static_cast<int>(engineRate)};
}

#ifdef LMMS_HAVE_OGGVORBIS
auto decodeSampleOggVorbis(const QString& audioFile) -> std::optional<SampleDecoder::Result>
{
	auto vorbisFile = OggVorbis_File{};
	const auto openError = ov_fopen(audioFile.toLocal8Bit(), &vorbisFile);

	if (openError != 0) { return std::nullopt; }

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

		if (samplesRead < 0) { return std::nullopt; }
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

	return SampleDecoder::Result{std::move(result), static_cast<int>(sampleRate)};
}
#endif // LMMS_HAVE_OGGVORBIS
} // namespace

auto SampleDecoder::supportedAudioTypes() -> const std::vector<AudioType>&
{
	static const auto s_audioTypes = []
	{
		auto types = std::vector<AudioType>();

		// Add DrumSynth by default since that support comes from us
		types.push_back(AudioType{"DrumSynth", "ds"});

		auto sfFormatInfo = SF_FORMAT_INFO{};
		auto simpleTypeCount = 0;
		sf_command(nullptr, SFC_GET_SIMPLE_FORMAT_COUNT, &simpleTypeCount, sizeof(int));

		// TODO: Ideally, this code should be iterating over the major formats, but some important extensions such as *.ogg
		// are not included. This is planned for future versions of sndfile.
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

			return types;
		}

		std::sort(types.begin(), types.end(),
			[&](const AudioType& a, const AudioType& b) { return a.name < b.name; });
		return types;
	}();
	return s_audioTypes;
}

auto SampleDecoder::decode(const QString& audioFile) -> std::optional<Result>
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
