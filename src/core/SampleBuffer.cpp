/*
 * SampleBuffer.cpp - container-class SampleBuffer
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

#include "SampleBuffer.h"
#include "Oscillator.h"

#include <algorithm>

#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QPainter>


#include <sndfile.h>

#define OV_EXCLUDE_STATIC_CALLBACKS
#ifdef LMMS_HAVE_OGGVORBIS
#include <vorbis/vorbisfile.h>
#endif

#ifdef LMMS_HAVE_FLAC_STREAM_ENCODER_H
#include <FLAC/stream_encoder.h>
#endif

#ifdef LMMS_HAVE_FLAC_STREAM_DECODER_H
#include <FLAC/stream_decoder.h>
#endif


#include "AudioEngine.h"
#include "base64.h"
#include "ConfigManager.h"
#include "DrumSynth.h"
#include "endian_handling.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "Note.h"
#include "PathUtil.h"

#include "FileDialog.h"

namespace lmms
{

SampleBuffer::SampleBuffer() :
	m_userAntiAliasWaveTable(nullptr),
	m_audioFile(""),
	m_origData(nullptr),
	m_origFrames(0),
	m_data(nullptr),
	m_frames(0),
	m_startFrame(0),
	m_endFrame(0),
	m_loopStartFrame(0),
	m_loopEndFrame(0),
	m_amplification(1.0f),
	m_reversed(false),
	m_frequency(DefaultBaseFreq),
	m_sampleRate(audioEngineSampleRate())
{

	connect(Engine::audioEngine(), SIGNAL(sampleRateChanged()), this, SLOT(sampleRateChanged()));
	update();
}



SampleBuffer::SampleBuffer(const QString & audioFile, bool isBase64Data)
	: SampleBuffer()
{
	if (isBase64Data)
	{
		loadFromBase64(audioFile);
	}
	else
	{
		m_audioFile = audioFile;
		update();
	}
}




SampleBuffer::SampleBuffer(const sampleFrame * data, const f_cnt_t frames)
	: SampleBuffer()
{
	if (frames > 0)
	{
		m_origData = MM_ALLOC<sampleFrame>( frames);
		memcpy(m_origData, data, frames * BYTES_PER_FRAME);
		m_origFrames = frames;
		update();
	}
}




SampleBuffer::SampleBuffer(const f_cnt_t frames)
	: SampleBuffer()
{
	if (frames > 0)
	{
		m_origData = MM_ALLOC<sampleFrame>( frames);
		memset(m_origData, 0, frames * BYTES_PER_FRAME);
		m_origFrames = frames;
		update();
	}
}




SampleBuffer::SampleBuffer(const SampleBuffer& orig)
{
	orig.m_varLock.lockForRead();

	m_audioFile = orig.m_audioFile;
	m_origFrames = orig.m_origFrames;
	m_origData = (m_origFrames > 0) ? MM_ALLOC<sampleFrame>( m_origFrames) : nullptr;
	m_frames = orig.m_frames;
	m_data = (m_frames > 0) ? MM_ALLOC<sampleFrame>( m_frames) : nullptr;
	m_startFrame = orig.m_startFrame;
	m_endFrame = orig.m_endFrame;
	m_loopStartFrame = orig.m_loopStartFrame;
	m_loopEndFrame = orig.m_loopEndFrame;
	m_amplification = orig.m_amplification;
	m_reversed = orig.m_reversed;
	m_frequency = orig.m_frequency;
	m_sampleRate = orig.m_sampleRate;

	//Deep copy m_origData and m_data from original
	const auto origFrameBytes = m_origFrames * BYTES_PER_FRAME;
	const auto frameBytes = m_frames * BYTES_PER_FRAME;
	if (orig.m_origData != nullptr && origFrameBytes > 0)
		{ memcpy(m_origData, orig.m_origData, origFrameBytes); }
	if (orig.m_data != nullptr && frameBytes > 0)
		{ memcpy(m_data, orig.m_data, frameBytes); }

	orig.m_varLock.unlock();
}




void swap(SampleBuffer& first, SampleBuffer& second) noexcept
{
	using std::swap;

	// Lock both buffers for writing, with address as lock ordering
	if (&first == &second) { return; }
	else if (&first > &second)
	{
		first.m_varLock.lockForWrite();
		second.m_varLock.lockForWrite();
	}
	else
	{
		second.m_varLock.lockForWrite();
		first.m_varLock.lockForWrite();
	}

	first.m_audioFile.swap(second.m_audioFile);
	swap(first.m_origData, second.m_origData);
	swap(first.m_data, second.m_data);
	swap(first.m_origFrames, second.m_origFrames);
	swap(first.m_frames, second.m_frames);
	swap(first.m_startFrame, second.m_startFrame);
	swap(first.m_endFrame, second.m_endFrame);
	swap(first.m_loopStartFrame, second.m_loopStartFrame);
	swap(first.m_loopEndFrame, second.m_loopEndFrame);
	swap(first.m_amplification, second.m_amplification);
	swap(first.m_frequency, second.m_frequency);
	swap(first.m_reversed, second.m_reversed);
	swap(first.m_sampleRate, second.m_sampleRate);

	// Unlock again
	first.m_varLock.unlock();
	second.m_varLock.unlock();
}




SampleBuffer& SampleBuffer::operator=(SampleBuffer that)
{
	swap(*this, that);
	return *this;
}




SampleBuffer::~SampleBuffer()
{
	MM_FREE(m_origData);
	MM_FREE(m_data);
}



void SampleBuffer::sampleRateChanged()
{
	update(true);
}

sample_rate_t SampleBuffer::audioEngineSampleRate()
{
	return Engine::audioEngine()->processingSampleRate();
}


void SampleBuffer::update(bool keepSettings)
{
	const bool lock = (m_data != nullptr);
	if (lock)
	{
		Engine::audioEngine()->requestChangeInModel();
		m_varLock.lockForWrite();
		MM_FREE(m_data);
	}

	// File size and sample length limits
	const int fileSizeMax = 300; // MB
	const int sampleLengthMax = 90; // Minutes

	enum class FileLoadError
	{
		None,
		ReadPermissionDenied,
		TooLarge,
		Invalid
	};
	FileLoadError fileLoadError = FileLoadError::None;

	if (m_audioFile.isEmpty() && m_origData != nullptr && m_origFrames > 0)
	{
		// TODO: reverse- and amplification-property is not covered
		// by following code...
		m_data = MM_ALLOC<sampleFrame>( m_origFrames);
		memcpy(m_data, m_origData, m_origFrames * BYTES_PER_FRAME);
		if (keepSettings == false)
		{
			m_frames = m_origFrames;
			m_loopStartFrame = m_startFrame = 0;
			m_loopEndFrame = m_endFrame = m_frames;
		}
	}
	else if (!m_audioFile.isEmpty())
	{
		QString file = PathUtil::toAbsolute(m_audioFile);
		int_sample_t * buf = nullptr;
		sample_t * fbuf = nullptr;
		ch_cnt_t channels = DEFAULT_CHANNELS;
		sample_rate_t samplerate = audioEngineSampleRate();
		m_frames = 0;

		const QFileInfo fileInfo(file);
		if (!fileInfo.isReadable())
		{
			fileLoadError = FileLoadError::ReadPermissionDenied;
		}
		else if (fileInfo.size() > fileSizeMax * 1024 * 1024)
		{
			fileLoadError = FileLoadError::TooLarge;
		}
		else
		{
			// Use QFile to handle unicode file names on Windows
			QFile f(file);
			SNDFILE * sndFile = nullptr;
			SF_INFO sfInfo;
			sfInfo.format = 0;

			if (f.open(QIODevice::ReadOnly) && (sndFile = sf_open_fd(f.handle(), SFM_READ, &sfInfo, false)))
			{
				f_cnt_t frames = sfInfo.frames;
				int rate = sfInfo.samplerate;
				if (frames / rate > sampleLengthMax * 60)
				{
					fileLoadError = FileLoadError::TooLarge;
				}
				sf_close(sndFile);
			}
			else
			{
				fileLoadError = FileLoadError::Invalid;
			}
			f.close();
		}

		if (fileLoadError == FileLoadError::None)
		{
#ifdef LMMS_HAVE_OGGVORBIS
			// workaround for a bug in libsndfile or our libsndfile decoder
			// causing some OGG files to be distorted -> try with OGG Vorbis
			// decoder first if filename extension matches "ogg"
			if (m_frames == 0 && fileInfo.suffix() == "ogg")
			{
				m_frames = decodeSampleOGGVorbis(file, buf, channels, samplerate);
			}
#endif
			if (m_frames == 0)
			{
				m_frames = decodeSampleSF(file, fbuf, channels, samplerate);
			}
#ifdef LMMS_HAVE_OGGVORBIS
			if (m_frames == 0)
			{
				m_frames = decodeSampleOGGVorbis(file, buf, channels, samplerate);
			}
#endif
			if (m_frames == 0)
			{
				m_frames = decodeSampleDS(file, buf, channels, samplerate);
			}
		}

		if (m_frames == 0 || fileLoadError != FileLoadError::None)  // if still no frames, bail
		{
			// sample couldn't be decoded, create buffer containing
			// one sample-frame
			m_data = MM_ALLOC<sampleFrame>( 1);
			memset(m_data, 0, sizeof(*m_data));
			m_frames = 1;
			m_loopStartFrame = m_startFrame = 0;
			m_loopEndFrame = m_endFrame = 1;
		}
		else // otherwise normalize sample rate
		{
			normalizeSampleRate(samplerate, keepSettings);
		}
	}
	else
	{
		// neither an audio-file nor a buffer to copy from, so create
		// buffer containing one sample-frame
		m_data = MM_ALLOC<sampleFrame>( 1);
		memset(m_data, 0, sizeof(*m_data));
		m_frames = 1;
		m_loopStartFrame = m_startFrame = 0;
		m_loopEndFrame = m_endFrame = 1;
	}

	if (lock)
	{
		m_varLock.unlock();
		Engine::audioEngine()->doneChangeInModel();
	}

	emit sampleUpdated();

	// allocate space for anti-aliased wave table
	if (m_userAntiAliasWaveTable == nullptr)
	{
		m_userAntiAliasWaveTable = std::make_unique<OscillatorConstants::waveform_t>();
	}
	Oscillator::generateAntiAliasUserWaveTable(this);

	if (fileLoadError != FileLoadError::None)
	{
		QString title = tr("Fail to open file");
		QString message;

		switch (fileLoadError)
		{
			case FileLoadError::None:
				// present just to avoid a compiler warning
				break;

			case FileLoadError::ReadPermissionDenied:
				message = tr("Read permission denied");
				break;

			case FileLoadError::TooLarge:
				message = tr("Audio files are limited to %1 MB "
					"in size and %2 minutes of playing time"
					).arg(fileSizeMax).arg(sampleLengthMax);
				break;

			case FileLoadError::Invalid:
				message = tr("Invalid audio file");
				break;
		}

		if (gui::getGUI() != nullptr)
		{
			QMessageBox::information(nullptr, title, message, QMessageBox::Ok);
		}
		else
		{
			fprintf(stderr, "%s\n", message.toUtf8().constData());
		}
	}
}


void SampleBuffer::convertIntToFloat(
	int_sample_t * & ibuf,
	f_cnt_t frames,
	int channels
)
{
	// following code transforms int-samples into float-samples and does amplifying & reversing
	const float fac = 1 / OUTPUT_SAMPLE_MULTIPLIER;
	m_data = MM_ALLOC<sampleFrame>( frames);
	const int ch = (channels > 1) ? 1 : 0;

	// if reversing is on, we also reverse when scaling
	bool isReversed = m_reversed;
	int idx = isReversed ? (frames - 1) * channels : 0;
	for (f_cnt_t frame = 0; frame < frames; ++frame)
	{
		m_data[frame][0] = ibuf[idx+0] * fac;
		m_data[frame][1] = ibuf[idx+ch] * fac;
		idx += isReversed ? -channels : channels;
	}

	delete[] ibuf;
}

void SampleBuffer::directFloatWrite(
	sample_t * & fbuf,
	f_cnt_t frames,
	int channels
)
{

	m_data = MM_ALLOC<sampleFrame>( frames);
	const int ch = (channels > 1) ? 1 : 0;

	// if reversing is on, we also reverse when scaling
	bool isReversed = m_reversed;
	int idx = isReversed ? (frames - 1) * channels : 0;
	for (f_cnt_t frame = 0; frame < frames; ++frame)
	{
		m_data[frame][0] = fbuf[idx+0];
		m_data[frame][1] = fbuf[idx+ch];
		idx += isReversed ? -channels : channels;
	}

	delete[] fbuf;
}


void SampleBuffer::normalizeSampleRate(const sample_rate_t srcSR, bool keepSettings)
{
	const sample_rate_t oldRate = m_sampleRate;
	// do samplerate-conversion to our default-samplerate
	if (srcSR != audioEngineSampleRate())
	{
		SampleBuffer * resampled = resample(srcSR, audioEngineSampleRate());

		m_sampleRate = audioEngineSampleRate();
		MM_FREE(m_data);
		m_frames = resampled->frames();
		m_data = MM_ALLOC<sampleFrame>( m_frames);
		memcpy(m_data, resampled->data(), m_frames * sizeof(sampleFrame));
		delete resampled;
	}

	if (keepSettings == false)
	{
		// update frame-variables
		m_loopStartFrame = m_startFrame = 0;
		m_loopEndFrame = m_endFrame = m_frames;
	}
	else if (oldRate != audioEngineSampleRate())
	{
		auto oldRateToNewRateRatio = static_cast<float>(audioEngineSampleRate()) / oldRate;

		m_startFrame = std::clamp(f_cnt_t(m_startFrame * oldRateToNewRateRatio), 0, m_frames);
		m_endFrame = std::clamp(f_cnt_t(m_endFrame * oldRateToNewRateRatio), m_startFrame, m_frames);
		m_loopStartFrame = std::clamp(f_cnt_t(m_loopStartFrame * oldRateToNewRateRatio), 0, m_frames);
		m_loopEndFrame = std::clamp(f_cnt_t(m_loopEndFrame * oldRateToNewRateRatio), m_loopStartFrame, m_frames);
		m_sampleRate = audioEngineSampleRate();
	}
}




f_cnt_t SampleBuffer::decodeSampleSF(
	QString fileName,
	sample_t * & buf,
	ch_cnt_t & channels,
	sample_rate_t & samplerate
)
{
	SNDFILE * sndFile;
	SF_INFO sfInfo;
	sfInfo.format = 0;
	f_cnt_t frames = 0;
	sf_count_t sfFramesRead;


	// Use QFile to handle unicode file names on Windows
	QFile f(fileName);
	if (f.open(QIODevice::ReadOnly) && (sndFile = sf_open_fd(f.handle(), SFM_READ, &sfInfo, false)))
	{
		frames = sfInfo.frames;

		buf = new sample_t[sfInfo.channels * frames];
		sfFramesRead = sf_read_float(sndFile, buf, sfInfo.channels * frames);

		if (sfFramesRead < sfInfo.channels * frames)
		{
#ifdef DEBUG_LMMS
			qDebug("SampleBuffer::decodeSampleSF(): could not read"
				" sample %s: %s", fileName, sf_strerror(nullptr));
#endif
		}
		channels = sfInfo.channels;
		samplerate = sfInfo.samplerate;

		sf_close(sndFile);
	}
	else
	{
#ifdef DEBUG_LMMS
		qDebug("SampleBuffer::decodeSampleSF(): could not load "
				"sample %s: %s", fileName, sf_strerror(nullptr));
#endif
	}
	f.close();

	//write down either directly or convert i->f depending on file type

	if (frames > 0 && buf != nullptr)
	{
		directFloatWrite(buf, frames, channels);
	}

	return frames;
}




#ifdef LMMS_HAVE_OGGVORBIS

// callback-functions for reading ogg-file

size_t qfileReadCallback(void * ptr, size_t size, size_t n, void * udata )
{
	return static_cast<QFile *>(udata)->read((char*) ptr, size * n);
}




int qfileSeekCallback(void * udata, ogg_int64_t offset, int whence)
{
	auto f = static_cast<QFile*>(udata);

	if (whence == SEEK_CUR)
	{
		f->seek(f->pos() + offset);
	}
	else if (whence == SEEK_END)
	{
		f->seek(f->size() + offset);
	}
	else
	{
		f->seek(offset);
	}
	return 0;
}




int qfileCloseCallback(void * udata)
{
	delete static_cast<QFile *>(udata);
	return 0;
}




long qfileTellCallback(void * udata)
{
	return static_cast<QFile *>(udata)->pos();
}




f_cnt_t SampleBuffer::decodeSampleOGGVorbis(
	QString fileName,
	int_sample_t * & buf,
	ch_cnt_t & channels,
	sample_rate_t & samplerate
)
{
	static ov_callbacks callbacks =
	{
		qfileReadCallback,
		qfileSeekCallback,
		qfileCloseCallback,
		qfileTellCallback
	} ;

	OggVorbis_File vf;

	f_cnt_t frames = 0;

	auto f = new QFile(fileName);
	if (f->open(QFile::ReadOnly) == false)
	{
		delete f;
		return 0;
	}

	int err = ov_open_callbacks(f, &vf, nullptr, 0, callbacks);

	if (err < 0)
	{
		switch (err)
		{
			case OV_EREAD:
				printf("SampleBuffer::decodeSampleOGGVorbis():"
						" media read error\n");
				break;
			case OV_ENOTVORBIS:
				printf("SampleBuffer::decodeSampleOGGVorbis():"
					" not an Ogg Vorbis file\n");
				break;
			case OV_EVERSION:
				printf("SampleBuffer::decodeSampleOGGVorbis():"
						" vorbis version mismatch\n");
				break;
			case OV_EBADHEADER:
				printf("SampleBuffer::decodeSampleOGGVorbis():"
					" invalid Vorbis bitstream header\n");
				break;
			case OV_EFAULT:
				printf("SampleBuffer::decodeSampleOgg(): "
					"internal logic fault\n");
				break;
		}
		delete f;
		return 0;
	}

	ov_pcm_seek(&vf, 0);

	channels = ov_info(&vf, -1)->channels;
	samplerate = ov_info(&vf, -1)->rate;

	ogg_int64_t total = ov_pcm_total(&vf, -1);

	buf = new int_sample_t[total * channels];
	int bitstream = 0;
	long bytesRead = 0;

	do
	{
		bytesRead = ov_read(&vf,
				(char *) &buf[frames * channels],
				(total - frames) * channels * BYTES_PER_INT_SAMPLE,
				isLittleEndian() ? 0 : 1,
				BYTES_PER_INT_SAMPLE,
				1,
				&bitstream
		);

		if (bytesRead < 0)
		{
			break;
		}
		frames += bytesRead / (channels * BYTES_PER_INT_SAMPLE);
	}
	while (bytesRead != 0 && bitstream == 0);

	ov_clear(&vf);

	// if buffer isn't empty, convert it to float and write it down
	if (frames > 0 && buf != nullptr)
	{
		convertIntToFloat(buf, frames, channels);
	}

	return frames;
}
#endif // LMMS_HAVE_OGGVORBIS




f_cnt_t SampleBuffer::decodeSampleDS(
	QString fileName,
	int_sample_t * & buf,
	ch_cnt_t & channels,
	sample_rate_t & samplerate
)
{
	DrumSynth ds;
	f_cnt_t frames = ds.GetDSFileSamples(fileName, buf, channels, samplerate);

	if (frames > 0 && buf != nullptr)
	{
		convertIntToFloat(buf, frames, channels);
	}

	return frames;

}




bool SampleBuffer::play(
	sampleFrame * ab,
	handleState * state,
	const fpp_t frames,
	const float freq,
	const LoopMode loopMode
)
{
	f_cnt_t startFrame = m_startFrame;
	f_cnt_t endFrame = m_endFrame;
	f_cnt_t loopStartFrame = m_loopStartFrame;
	f_cnt_t loopEndFrame = m_loopEndFrame;

	if (endFrame == 0 || frames == 0)
	{
		return false;
	}

	// variable for determining if we should currently be playing backwards in a ping-pong loop
	bool isBackwards = state->isBackwards();

	// The SampleBuffer can play a given sample with increased or decreased pitch. However, only
	// samples that contain a tone that matches the default base note frequency of 440 Hz will
	// produce the exact requested pitch in [Hz].
	const double freqFactor = (double) freq / (double) m_frequency *
		m_sampleRate / Engine::audioEngine()->processingSampleRate();

	// calculate how many frames we have in requested pitch
	const auto totalFramesForCurrentPitch = static_cast<f_cnt_t>((endFrame - startFrame) / freqFactor);

	if (totalFramesForCurrentPitch == 0)
	{
		return false;
	}


	// this holds the index of the first frame to play
	f_cnt_t playFrame = std::max(state->m_frameIndex, startFrame);

	if (loopMode == LoopMode::Off)
	{
		if (playFrame >= endFrame || (endFrame - playFrame) / freqFactor == 0)
		{
			// the sample is done being played
			return false;
		}
	}
	else if (loopMode == LoopMode::On)
	{
		playFrame = getLoopedIndex(playFrame, loopStartFrame, loopEndFrame);
	}
	else
	{
		playFrame = getPingPongIndex(playFrame, loopStartFrame, loopEndFrame);
	}

	f_cnt_t fragmentSize = (f_cnt_t)(frames * freqFactor) + MARGIN[state->interpolationMode()];

	sampleFrame * tmp = nullptr;

	// check whether we have to change pitch...
	if (freqFactor != 1.0 || state->m_varyingPitch)
	{
		SRC_DATA srcData;
		// Generate output
		srcData.data_in =
			getSampleFragment(playFrame, fragmentSize, loopMode, &tmp, &isBackwards,
			loopStartFrame, loopEndFrame, endFrame )->data();
		srcData.data_out = ab->data();
		srcData.input_frames = fragmentSize;
		srcData.output_frames = frames;
		srcData.src_ratio = 1.0 / freqFactor;
		srcData.end_of_input = 0;
		int error = src_process(state->m_resamplingData, &srcData);
		if (error)
		{
			printf("SampleBuffer: error while resampling: %s\n",
							src_strerror(error));
		}
		if (srcData.output_frames_gen > frames)
		{
			printf("SampleBuffer: not enough frames: %ld / %d\n",
					srcData.output_frames_gen, frames);
		}
		// Advance
		switch (loopMode)
		{
			case LoopMode::Off:
				playFrame += srcData.input_frames_used;
				break;
			case LoopMode::On:
				playFrame += srcData.input_frames_used;
				playFrame = getLoopedIndex(playFrame, loopStartFrame, loopEndFrame);
				break;
			case LoopMode::PingPong:
			{
				f_cnt_t left = srcData.input_frames_used;
				if (state->isBackwards())
				{
					playFrame -= srcData.input_frames_used;
					if (playFrame < loopStartFrame)
					{
						left -= (loopStartFrame - playFrame);
						playFrame = loopStartFrame;
					}
					else left = 0;
				}
				playFrame += left;
				playFrame = getPingPongIndex(playFrame, loopStartFrame, loopEndFrame);
				break;
			}
		}
	}
	else
	{
		// we don't have to pitch, so we just copy the sample-data
		// as is into pitched-copy-buffer

		// Generate output
		memcpy(ab,
			getSampleFragment(playFrame, frames, loopMode, &tmp, &isBackwards,
				loopStartFrame, loopEndFrame, endFrame),
			frames * BYTES_PER_FRAME);
		// Advance
		switch (loopMode)
		{
			case LoopMode::Off:
				playFrame += frames;
				break;
			case LoopMode::On:
				playFrame += frames;
				playFrame = getLoopedIndex(playFrame, loopStartFrame, loopEndFrame);
				break;
			case LoopMode::PingPong:
			{
				f_cnt_t left = frames;
				if (state->isBackwards())
				{
					playFrame -= frames;
					if (playFrame < loopStartFrame)
					{
						left -= (loopStartFrame - playFrame);
						playFrame = loopStartFrame;
					}
					else left = 0;
				}
				playFrame += left;
				playFrame = getPingPongIndex(playFrame, loopStartFrame, loopEndFrame);
				break;
			}
		}
	}

	if (tmp != nullptr)
	{
		MM_FREE(tmp);
	}

	state->setBackwards(isBackwards);
	state->setFrameIndex(playFrame);

	for (fpp_t i = 0; i < frames; ++i)
	{
		ab[i][0] *= m_amplification;
		ab[i][1] *= m_amplification;
	}

	return true;
}




sampleFrame * SampleBuffer::getSampleFragment(
	f_cnt_t index,
	f_cnt_t frames,
	LoopMode loopMode,
	sampleFrame * * tmp,
	bool * backwards,
	f_cnt_t loopStart,
	f_cnt_t loopEnd,
	f_cnt_t end
) const
{
	if (loopMode == LoopMode::Off)
	{
		if (index + frames <= end)
		{
			return m_data + index;
		}
	}
	else if (loopMode == LoopMode::On)
	{
		if (index + frames <= loopEnd)
		{
			return m_data + index;
		}
	}
	else
	{
		if (!*backwards && index + frames < loopEnd)
		{
			return m_data + index;
		}
	}

	*tmp = MM_ALLOC<sampleFrame>( frames);

	if (loopMode == LoopMode::Off)
	{
		f_cnt_t available = end - index;
		memcpy(*tmp, m_data + index, available * BYTES_PER_FRAME);
		memset(*tmp + available, 0, (frames - available) * BYTES_PER_FRAME);
	}
	else if (loopMode == LoopMode::On)
	{
		f_cnt_t copied = std::min(frames, loopEnd - index);
		memcpy(*tmp, m_data + index, copied * BYTES_PER_FRAME);
		f_cnt_t loopFrames = loopEnd - loopStart;
		while (copied < frames)
		{
			f_cnt_t todo = std::min(frames - copied, loopFrames);
			memcpy(*tmp + copied, m_data + loopStart, todo * BYTES_PER_FRAME);
			copied += todo;
		}
	}
	else
	{
		f_cnt_t pos = index;
		bool currentBackwards = pos < loopStart
			? false
			: *backwards;
		f_cnt_t copied = 0;


		if (currentBackwards)
		{
			copied = std::min(frames, pos - loopStart);
			for (int i = 0; i < copied; i++)
			{
				(*tmp)[i][0] = m_data[pos - i][0];
				(*tmp)[i][1] = m_data[pos - i][1];
			}
			pos -= copied;
			if (pos == loopStart) { currentBackwards = false; }
		}
		else
		{
			copied = std::min(frames, loopEnd - pos);
			memcpy(*tmp, m_data + pos, copied * BYTES_PER_FRAME);
			pos += copied;
			if (pos == loopEnd) { currentBackwards = true; }
		}

		while (copied < frames)
		{
			if (currentBackwards)
			{
				f_cnt_t todo = std::min(frames - copied, pos - loopStart);
				for (int i = 0; i < todo; i++)
				{
					(*tmp)[copied + i][0] = m_data[pos - i][0];
					(*tmp)[copied + i][1] = m_data[pos - i][1];
				}
				pos -= todo;
				copied += todo;
				if (pos <= loopStart) { currentBackwards = false; }
			}
			else
			{
				f_cnt_t todo = std::min(frames - copied, loopEnd - pos);
				memcpy(*tmp + copied, m_data + pos, todo * BYTES_PER_FRAME);
				pos += todo;
				copied += todo;
				if (pos >= loopEnd) { currentBackwards = true; }
			}
		}
		*backwards = currentBackwards;
	}

	return *tmp;
}




f_cnt_t SampleBuffer::getLoopedIndex(f_cnt_t index, f_cnt_t startf, f_cnt_t endf) const
{
	if (index < endf)
	{
		return index;
	}
	return startf + (index - startf) % (endf - startf);
}


f_cnt_t SampleBuffer::getPingPongIndex(f_cnt_t index, f_cnt_t startf, f_cnt_t endf) const
{
	if (index < endf)
	{
		return index;
	}
	const f_cnt_t loopLen = endf - startf;
	const f_cnt_t loopPos = (index - endf) % (loopLen * 2);

	return (loopPos < loopLen)
		? endf - loopPos
		: startf + (loopPos - loopLen);
}


/* @brief Draws a sample buffer on the QRect given in the range [fromFrame, toFrame)
 * @param QPainter p: Painter object for the painting operations
 * @param QRect dr: QRect where the buffer will be drawn in
 * @param QRect clip: QRect used for clipping
 * @param f_cnt_t fromFrame: First frame of the range
 * @param f_cnt_t toFrame: Last frame of the range non-inclusive
 */
void SampleBuffer::visualize(
	QPainter & p,
	const QRect & dr,
	const QRect & clip,
	f_cnt_t fromFrame,
	f_cnt_t toFrame
)
{
	if (m_frames == 0) { return; }

	const bool focusOnRange = toFrame <= m_frames && 0 <= fromFrame && fromFrame < toFrame;
	//TODO: If the clip QRect is not being used we should remove it
	//p.setClipRect(clip);
	const int w = dr.width();
	const int h = dr.height();

	const int yb = h / 2 + dr.y();
	const float ySpace = h * 0.5f;
	const int nbFrames = focusOnRange ? toFrame - fromFrame : m_frames;

	const double fpp = std::max(1., static_cast<double>(nbFrames) / w);
	// There are 2 possibilities: Either nbFrames is bigger than
	// the width, so we will have width points, or nbFrames is
	// smaller than the width (fpp = 1) and we will have nbFrames
	// points
	const int totalPoints = nbFrames > w
		? w
		: nbFrames;
	std::vector<QPointF> fEdgeMax(totalPoints);
	std::vector<QPointF> fEdgeMin(totalPoints);
	std::vector<QPointF> fRmsMax(totalPoints);
	std::vector<QPointF> fRmsMin(totalPoints);
	int curPixel = 0;
	const int xb = dr.x();
	const int first = focusOnRange ? fromFrame : 0;
	const int last = focusOnRange ? toFrame - 1 : m_frames - 1;
	// When the number of frames isn't perfectly divisible by the
	// width, the remaining frames don't fit the last pixel and are
	// past the visible area. lastVisibleFrame is the index number of
	// the last visible frame.
	const int visibleFrames = (fpp * w);
	const int lastVisibleFrame = focusOnRange
		? fromFrame + visibleFrames - 1
		: visibleFrames - 1;

	for (double frame = first; frame <= last && frame <= lastVisibleFrame; frame += fpp)
	{
		float maxData = -1;
		float minData = 1;

		auto rmsData = std::array<float, 2>{};

		// Find maximum and minimum samples within range
		for (int i = 0; i < fpp && frame + i <= last; ++i)
		{
			for (int j = 0; j < 2; ++j)
			{
				auto curData = m_data[static_cast<int>(frame) + i][j];

				if (curData > maxData) { maxData = curData; }
				if (curData < minData) { minData = curData; }

				rmsData[j] += curData * curData;
			}
		}

		const float trueRmsData = (rmsData[0] + rmsData[1]) / 2 / fpp;
		const float sqrtRmsData = sqrt(trueRmsData);
		const float maxRmsData = std::clamp(sqrtRmsData, minData, maxData);
		const float minRmsData = std::clamp(-sqrtRmsData, minData, maxData);

		// If nbFrames >= w, we can use curPixel to calculate X
		// but if nbFrames < w, we need to calculate it proportionally
		// to the total number of points
		auto x = nbFrames >= w
			? xb + curPixel
			: xb + ((static_cast<double>(curPixel) / nbFrames) * w);
		// Partial Y calculation
		auto py = ySpace * m_amplification;
		fEdgeMax[curPixel] = QPointF(x, (yb - (maxData * py)));
		fEdgeMin[curPixel] = QPointF(x, (yb - (minData * py)));
		fRmsMax[curPixel] = QPointF(x, (yb - (maxRmsData * py)));
		fRmsMin[curPixel] = QPointF(x, (yb - (minRmsData * py)));
		++curPixel;
	}

	for (int i = 0; i < totalPoints; ++i)
	{
		p.drawLine(fEdgeMax[i], fEdgeMin[i]);
	}

	p.setPen(p.pen().color().lighter(123));

	for (int i = 0; i < totalPoints; ++i)
	{
		p.drawLine(fRmsMax[i], fRmsMin[i]);
	}
}




QString SampleBuffer::openAudioFile() const
{
	gui::FileDialog ofd(nullptr, tr("Open audio file"));

	QString dir;
	if (!m_audioFile.isEmpty())
	{
		QString f = m_audioFile;
		if (QFileInfo(f).isRelative())
		{
			f = ConfigManager::inst()->userSamplesDir() + f;
			if (QFileInfo(f).exists() == false)
			{
				f = ConfigManager::inst()->factorySamplesDir() +
								m_audioFile;
			}
		}
		dir = QFileInfo(f).absolutePath();
	}
	else
	{
		dir = ConfigManager::inst()->userSamplesDir();
	}
	// change dir to position of previously opened file
	ofd.setDirectory(dir);
	ofd.setFileMode(gui::FileDialog::ExistingFiles);

	// set filters
	QStringList types;
	types << tr("All Audio-Files (*.wav *.ogg *.ds *.flac *.spx *.voc "
					"*.aif *.aiff *.au *.raw)")
		<< tr("Wave-Files (*.wav)")
		<< tr("OGG-Files (*.ogg)")
		<< tr("DrumSynth-Files (*.ds)")
		<< tr("FLAC-Files (*.flac)")
		<< tr("SPEEX-Files (*.spx)")
		//<< tr("MP3-Files (*.mp3)")
		//<< tr("MIDI-Files (*.mid)")
		<< tr("VOC-Files (*.voc)")
		<< tr("AIFF-Files (*.aif *.aiff)")
		<< tr("AU-Files (*.au)")
		<< tr("RAW-Files (*.raw)")
		//<< tr("MOD-Files (*.mod)")
		;
	ofd.setNameFilters(types);
	if (!m_audioFile.isEmpty())
	{
		// select previously opened file
		ofd.selectFile(QFileInfo(m_audioFile).fileName());
	}

	if (ofd.exec () == QDialog::Accepted)
	{
		if (ofd.selectedFiles().isEmpty())
		{
			return QString();
		}
		return PathUtil::toShortestRelative(ofd.selectedFiles()[0]);
	}

	return QString();
}




QString SampleBuffer::openAndSetAudioFile()
{
	QString fileName = this->openAudioFile();

	if(!fileName.isEmpty())
	{
		this->setAudioFile(fileName);
	}

	return fileName;
}


QString SampleBuffer::openAndSetWaveformFile()
{
	if (m_audioFile.isEmpty())
	{
		m_audioFile = ConfigManager::inst()->factorySamplesDir() + "waveforms/10saw.flac";
	}

	QString fileName = this->openAudioFile();

	if (!fileName.isEmpty())
	{
		this->setAudioFile(fileName);
	}
	else
	{
		m_audioFile = "";
	}

	return fileName;
}



#undef LMMS_HAVE_FLAC_STREAM_ENCODER_H	/* not yet... */
#undef LMMS_HAVE_FLAC_STREAM_DECODER_H

#ifdef LMMS_HAVE_FLAC_STREAM_ENCODER_H
FLAC__StreamEncoderWriteStatus flacStreamEncoderWriteCallback(
	const FLAC__StreamEncoder * /*encoder*/,
	const FLAC__byte buffer[],
	unsigned int /*samples*/,
	unsigned int bytes,
	unsigned int /*currentFrame*/,
	void * clientData
)
{
/*	if (bytes == 0)
	{
		return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
	}*/
	return (static_cast<QBuffer *>(clientData)->write(
				(const char *) buffer, bytes) == (int) bytes)
		? FLAC__STREAM_ENCODER_WRITE_STATUS_OK
		: FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
}


void flacStreamEncoderMetadataCallback(
	const FLAC__StreamEncoder *,
	const FLAC__StreamMetadata * metadata,
	void * clientData
)
{
	QBuffer * b = static_cast<QBuffer *>(clientData);
	b->seek(0);
	b->write((const char *) metadata, sizeof(*metadata));
}

#endif // LMMS_HAVE_FLAC_STREAM_ENCODER_H



QString & SampleBuffer::toBase64(QString & dst) const
{
#ifdef LMMS_HAVE_FLAC_STREAM_ENCODER_H
	const f_cnt_t FRAMES_PER_BUF = 1152;

	FLAC__StreamEncoder * flacEnc = FLAC__stream_encoder_new();
	FLAC__stream_encoder_set_channels(flacEnc, DEFAULT_CHANNELS);
	FLAC__stream_encoder_set_blocksize(flacEnc, FRAMES_PER_BUF);
/*	FLAC__stream_encoder_set_do_exhaustive_model_search(flacEnc, true);
	FLAC__stream_encoder_set_do_mid_side_stereo(flacEnc, true);*/
	FLAC__stream_encoder_set_sample_rate(flacEnc,
		Engine::audioEngine()->sampleRate());

	QBuffer baWriter;
	baWriter.open(QBuffer::WriteOnly);

	FLAC__stream_encoder_set_write_callback(flacEnc,
		flacStreamEncoderWriteCallback);
	FLAC__stream_encoder_set_metadata_callback(flacEnc,
		flacStreamEncoderMetadataCallback);
	FLAC__stream_encoder_set_client_data(flacEnc, &baWriter);

	if (FLAC__stream_encoder_init(flacEnc) != FLAC__STREAM_ENCODER_OK)
	{
		printf("Error within FLAC__stream_encoder_init()!\n");
	}

	f_cnt_t frameCnt = 0;

	while (frameCnt < m_frames)
	{
		f_cnt_t remaining = std::min<f_cnt_t>(FRAMES_PER_BUF, m_frames - frameCnt);
		FLAC__int32 buf[FRAMES_PER_BUF * DEFAULT_CHANNELS];
		for (f_cnt_t f = 0; f < remaining; ++f)
		{
			for (ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS; ++ch)
			{
				buf[f*DEFAULT_CHANNELS+ch] = (FLAC__int32)(
					AudioEngine::clip(m_data[f+frameCnt][ch]) *
						OUTPUT_SAMPLE_MULTIPLIER);
			}
		}
		FLAC__stream_encoder_process_interleaved(flacEnc, buf, remaining);
		frameCnt += remaining;
	}
	FLAC__stream_encoder_finish(flacEnc);
	FLAC__stream_encoder_delete(flacEnc);
	printf("%d %d\n", frameCnt, (int)baWriter.size());
	baWriter.close();

	base64::encode(baWriter.buffer().data(), baWriter.buffer().size(), dst);

#else	// LMMS_HAVE_FLAC_STREAM_ENCODER_H

	base64::encode((const char *) m_data,
		m_frames * sizeof(sampleFrame), dst);

#endif	// LMMS_HAVE_FLAC_STREAM_ENCODER_H

	return dst;
}




SampleBuffer * SampleBuffer::resample(const sample_rate_t srcSR, const sample_rate_t dstSR )
{
	sampleFrame * data = m_data;
	const f_cnt_t frames = m_frames;
	const auto dstFrames = static_cast<f_cnt_t>((frames / (float)srcSR) * (float)dstSR);
	auto dstSB = new SampleBuffer(dstFrames);
	sampleFrame * dstBuf = dstSB->m_origData;

	// yeah, libsamplerate, let's rock with sinc-interpolation!
	int error;
	SRC_STATE * state;
	if ((state = src_new(SRC_SINC_MEDIUM_QUALITY, DEFAULT_CHANNELS, &error)) != nullptr)
	{
		SRC_DATA srcData;
		srcData.end_of_input = 1;
		srcData.data_in = data->data();
		srcData.data_out = dstBuf->data();
		srcData.input_frames = frames;
		srcData.output_frames = dstFrames;
		srcData.src_ratio = (double) dstSR / srcSR;
		if ((error = src_process(state, &srcData)))
		{
			printf("SampleBuffer: error while resampling: %s\n", src_strerror(error));
		}
		src_delete(state);
	}
	else
	{
		printf("Error: src_new() failed in SampleBuffer.cpp!\n");
	}
	dstSB->update();
	return dstSB;
}




void SampleBuffer::setAudioFile(const QString & audioFile)
{
	m_audioFile = PathUtil::toShortestRelative(audioFile);
	update();
}



#ifdef LMMS_HAVE_FLAC_STREAM_DECODER_H

struct flacStreamDecoderClientData
{
	QBuffer * readBuffer;
	QBuffer * writeBuffer;
} ;



FLAC__StreamDecoderReadStatus flacStreamDecoderReadCallback(
	const FLAC__StreamDecoder * /*decoder*/,
	FLAC__byte * buffer,
	unsigned int * bytes,
	void * clientData
)
{
	int res = static_cast<flacStreamDecoderClientData *>(
		clientData)->readBuffer->read((char *) buffer, *bytes);

	if (res > 0)
	{
		*bytes = res;
		return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
	}

	*bytes = 0;
	return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
}




FLAC__StreamDecoderWriteStatus flacStreamDecoderWriteCallback(
	const FLAC__StreamDecoder * /*decoder*/,
	const FLAC__Frame * frame,
	const FLAC__int32 * const buffer[],
	void * clientData
)
{
	if (frame->header.channels != 2)
	{
		printf("channels != 2 in flacStreamDecoderWriteCallback()\n");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}

	if (frame->header.bits_per_sample != 16)
	{
		printf("bits_per_sample != 16 in flacStreamDecoderWriteCallback()\n");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}

	const f_cnt_t numberOfFrames = frame->header.blocksize;
	for (f_cnt_t f = 0; f < numberOfFrames; ++f)
	{
		sampleFrame sframe = { buffer[0][f] / OUTPUT_SAMPLE_MULTIPLIER,
					buffer[1][f] / OUTPUT_SAMPLE_MULTIPLIER
		} ;
		static_cast<flacStreamDecoderClientData *>(
					clientData )->writeBuffer->write(
				(const char *) sframe, sizeof(sframe));
	}
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}


void flacStreamDecoderMetadataCallback(
	const FLAC__StreamDecoder *,
	const FLAC__StreamMetadata *,
	void * /*clientData*/
)
{
	printf("stream decoder metadata callback\n");
/*	QBuffer * b = static_cast<QBuffer *>(clientData);
	b->seek(0);
	b->write((const char *) metadata, sizeof(*metadata));*/
}


void flacStreamDecoderErrorCallback(
	const FLAC__StreamDecoder *,
	FLAC__StreamDecoderErrorStatus status,
	void * /*clientData*/
)
{
	printf("error callback! %d\n", status);
	// what to do now??
}

#endif // LMMS_HAVE_FLAC_STREAM_DECODER_H


void SampleBuffer::loadFromBase64(const QString & data)
{
	char * dst = nullptr;
	int dsize = 0;
	base64::decode(data, &dst, &dsize);

#ifdef LMMS_HAVE_FLAC_STREAM_DECODER_H

	QByteArray origData = QByteArray::fromRawData(dst, dsize);
	QBuffer baReader(&origData);
	baReader.open(QBuffer::ReadOnly);

	QBuffer baWriter;
	baWriter.open(QBuffer::WriteOnly);

	flacStreamDecoderClientData cdata = { &baReader, &baWriter } ;

	FLAC__StreamDecoder * flacDec = FLAC__stream_decoder_new();

	FLAC__stream_decoder_set_read_callback(flacDec,
		flacStreamDecoderReadCallback);
	FLAC__stream_decoder_set_write_callback(flacDec,
		flacStreamDecoderWriteCallback);
	FLAC__stream_decoder_set_error_callback(flacDec,
		flacStreamDecoderErrorCallback);
	FLAC__stream_decoder_set_metadata_callback(flacDec,
		flacStreamDecoderMetadataCallback);
	FLAC__stream_decoder_set_client_data(flacDec, &cdata);

	FLAC__stream_decoder_init(flacDec);

	FLAC__stream_decoder_process_until_end_of_stream(flacDec);

	FLAC__stream_decoder_finish(flacDec);
	FLAC__stream_decoder_delete(flacDec);

	baReader.close();

	origData = baWriter.buffer();
	printf("%d\n", (int) origData.size());

	m_origFrames = origData.size() / sizeof(sampleFrame);
	MM_FREE(m_origData);
	m_origData = MM_ALLOC<sampleFrame>( m_origFrames);
	memcpy(m_origData, origData.data(), origData.size());

#else /* LMMS_HAVE_FLAC_STREAM_DECODER_H */

	m_origFrames = dsize / sizeof(sampleFrame);
	MM_FREE(m_origData);
	m_origData = MM_ALLOC<sampleFrame>( m_origFrames);
	memcpy(m_origData, dst, dsize);

#endif // LMMS_HAVE_FLAC_STREAM_DECODER_H

	delete[] dst;

	m_audioFile = QString();
	update();
}




void SampleBuffer::setStartFrame(const f_cnt_t s)
{
	m_startFrame = s;
}




void SampleBuffer::setEndFrame(const f_cnt_t e)
{
	m_endFrame = e;
}




void SampleBuffer::setAmplification(float a)
{
	m_amplification = a;
	emit sampleUpdated();
}




void SampleBuffer::setReversed(bool on)
{
	Engine::audioEngine()->requestChangeInModel();
	m_varLock.lockForWrite();
	if (m_reversed != on) { std::reverse(m_data, m_data + m_frames); }
	m_reversed = on;
	m_varLock.unlock();
	Engine::audioEngine()->doneChangeInModel();
	emit sampleUpdated();
}





SampleBuffer::handleState::handleState(bool varyingPitch, int interpolationMode) :
	m_frameIndex(0),
	m_varyingPitch(varyingPitch),
	m_isBackwards(false)
{
	int error;
	m_interpolationMode = interpolationMode;

	if ((m_resamplingData = src_new(interpolationMode, DEFAULT_CHANNELS, &error)) == nullptr)
	{
		qDebug("Error: src_new() failed in SampleBuffer.cpp!\n");
	}
}




SampleBuffer::handleState::~handleState()
{
	src_delete(m_resamplingData);
}

} // namespace lmms
