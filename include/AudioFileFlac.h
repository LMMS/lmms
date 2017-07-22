/*
 * AudioFileFlac.h - Audio device which encodes a wave stream into a FLAC file.
 */

#ifndef AUDIO_FILE_FLAC_H
#define AUDIO_FILE_FLAC_H

#include "lmmsconfig.h"

#include "AudioFileDevice.h"
#include <sndfile.h>

class AudioFileFlac: public AudioFileDevice
{
public:
	AudioFileFlac( OutputSettings const& outputSettings,
			ch_cnt_t const channels,
			bool& successful,
			QString const& file,
			Mixer* mixer
	);

	virtual ~AudioFileFlac();

	static AudioFileDevice* getInst( QString const& outputFilename,
			OutputSettings const& outputSettings,
			ch_cnt_t const channels,
			Mixer* mixer,
			bool& successful)
	{
		return new AudioFileFlac(outputSettings,channels,successful,outputFilename,mixer);
	}

private:

	SF_INFO  m_sfinfo;
	SNDFILE* m_sf;

	virtual void writeBuffer(surroundSampleFrame const* _ab,
						fpp_t const frames,
						float master_gain) override;

	bool startEncoding();
	void finishEncoding();

};

#endif //AUDIO_FILE_FLAC_H
