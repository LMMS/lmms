#include "AudioFileFlac.h"
#include "endian_handling.h"
#include "Mixer.h"

AudioFileFlac::AudioFileFlac(OutputSettings const& outputSettings, ch_cnt_t const channels, bool& successful, QString const& file, Mixer* mixer):
	AudioFileDevice(outputSettings,channels,file,mixer),
	m_sf(nullptr)
{
	successful = outputFileOpened() && startEncoding();
}

AudioFileFlac::~AudioFileFlac()
{
	finishEncoding();
}

bool AudioFileFlac::startEncoding()
{
	m_sfinfo.samplerate=sampleRate();
	m_sfinfo.channels=channels();
	m_sfinfo.frames = mixer()->framesPerPeriod();
	m_sfinfo.sections=1;
	m_sfinfo.seekable=0;

	m_sfinfo.format = SF_FORMAT_FLAC;

	switch (getOutputSettings().getBitDepth())
	{
		case OutputSettings::Depth_24Bit:
		case OutputSettings::Depth_32Bit:
			// FLAC does not support 32bit sampling, so take it as 24.
			m_sfinfo.format |= SF_FORMAT_PCM_24;
			break;
		default:
			m_sfinfo.format |= SF_FORMAT_PCM_16;
	}

#ifdef LMMS_HAVE_SF_COMPLEVEL
	double compression = getOutputSettings().getCompressionLevel();
	sf_command(m_sf,SFC_SET_COMPRESSION_LEVEL,&compression,sizeof(double));
#endif

	m_sf = sf_open(
#ifdef LMMS_BUILD_WIN32
		outputFile().toLocal8bit().constData(),
#else
		outputFile().toUtf8().constData(),
#endif
		SFM_WRITE,
		&m_sfinfo
	);

	sf_command(m_sf,SFC_SET_CLIPPING,nullptr,SF_TRUE);

	sf_set_string(m_sf,SF_STR_SOFTWARE,"LMMS");

	return true;
}

void AudioFileFlac::writeBuffer(surroundSampleFrame const* _ab, fpp_t const frames, float master_gain)
{
	OutputSettings::BitDepth depth = getOutputSettings().getBitDepth();

	if (depth == OutputSettings::Depth_24Bit || depth == OutputSettings::Depth_32Bit) // Float encoding
	{
		float* buf = new float[frames*channels()];
		for(fpp_t frame = 0; frame < frames; ++frame)
		{
			for(ch_cnt_t channel=0; channel<channels(); ++channel)
			{
				buf[frame*channels()+channel] = _ab[frame][channel] * master_gain;
			}
		}
		sf_writef_float(m_sf,buf,frames);
		delete[] buf;
	}
	else // integer PCM encoding
	{
		int_sample_t* buf = new int_sample_t[frames*channels()];
		convertToS16(_ab,frames,master_gain,buf,!isLittleEndian());
		sf_writef_short(m_sf,static_cast<short*>(buf),frames);
		delete[] buf;
	}

}


void AudioFileFlac::finishEncoding()
{
	if (m_sf)
	{
		sf_write_sync(m_sf);
		sf_close(m_sf);
	}
}