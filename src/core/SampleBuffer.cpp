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


#include <QBuffer>
#include <QFileInfo>
#include <QMessageBox>
#include <QPainter>
#include <QDomElement>
#include <QDebug>
#include "base64.h"
#include "ConfigManager.h"
#include "DrumSynth.h"

#include "FileDialog.h"
#include "internal/SampleBufferFileHelper.h"
#include "lmms_math.h"
#include "interpolation.h"


SampleBuffer::SampleBuffer(const QString &_audio_file, bool ignoreError)
		: SampleBuffer{internal::SampleBufferFileHelper::Load(_audio_file, ignoreError)}
{
	m_playInfo->setMaybeAudioFile(_audio_file);
}


SampleBuffer::SampleBuffer(SampleBuffer::DataVector &&movedData, sample_rate_t sampleRate)
	: SampleBuffer{internal::SampleBufferData{std::move(movedData), sampleRate}}
{
}

SampleBuffer::SampleBuffer(SampleBuffer &&sampleBuffer) noexcept
	:
	  m_data{std::move(sampleBuffer.m_data)},
	  m_playInfo(std::move(sampleBuffer.m_playInfo))
{
	emit sampleUpdated(UpdateType::Clear);
}

SampleBuffer &SampleBuffer::operator=(SampleBuffer &&other) noexcept{
	DataChangeHelper helper(this, UpdateType::Clear);

	m_data = std::move(other.m_data);
	m_playInfo = std::move(other.m_playInfo);

	return *this;
}

void SampleBuffer::saveSettings(QDomDocument &doc, QDomElement &_this) {
	{
		QString string;
		_this.setAttribute("data", toBase64(string));
	}

	auto data = guardedData();
	_this.setAttribute("sampleRate", data->getSampleRate());
	_this.setAttribute("amplification", data->getAmplification());
	_this.setAttribute("frequency", data->getFrequency());
}

void SampleBuffer::loadSettings(const QDomElement &_this) {
	sample_rate_t loadingSampleRate = Engine::mixer()->baseSampleRate();

	if (_this.hasAttribute("sampleRate")) {
		loadingSampleRate = _this.attribute("sampleRate").toUInt();
	} else {
		qWarning("SampleBuffer::loadSettings: Using default sampleRate. That could lead to invalid values");
	}

	if (_this.hasAttribute("data")) {
		*m_data = internal::SampleBufferData::loadFromBase64(_this.attribute("data"), loadingSampleRate);
	}

	if (_this.hasAttribute("amplification")) {
		m_data->setAmplification(_this.attribute("amplification").toFloat());
	}

	if (_this.hasAttribute("frequency")) {
		m_data->setFrequency(_this.attribute("frequency").toFloat());
	}
}

bool SampleBuffer::play(sampleFrame *_ab, handleState *_state,
						const fpp_t _frames,
						const float _freq,
						const LoopMode _loopmode) {
	f_cnt_t startFrame = m_playInfo->getStartFrame();
	f_cnt_t endFrame = m_playInfo->getEndFrame();
	f_cnt_t loopStartFrame = m_playInfo->getLoopStartFrame();
	f_cnt_t loopEndFrame = m_playInfo->getLoopEndFrame();

	if (endFrame == 0 || _frames == 0) {
		return false;
	}

	// variable for determining if we should currently be playing backwards in a ping-pong loop
	bool is_backwards = _state->isBackwards();

	const double freq_factor = (double) _freq / (double) m_data->getFrequency() *
							   double(m_data->getSampleRate()) / double(Engine::mixer()->processingSampleRate());

	// calculate how many frames we have in requested pitch
	const auto total_frames_for_current_pitch = static_cast<f_cnt_t>((endFrame - startFrame) /
																	 freq_factor );

	if (total_frames_for_current_pitch == 0) {
		return false;
	}


	// this holds the index of the first frame to play
	f_cnt_t play_frame = qMax(_state->m_frameIndex, startFrame);

	if (_loopmode == LoopOff) {
		if (play_frame >= endFrame || (endFrame - play_frame) / freq_factor == 0) {
			// the sample is done being played
			return false;
		}
	} else if (_loopmode == LoopOn) {
		play_frame = getLoopedIndex(play_frame, loopStartFrame, loopEndFrame);
	} else {
		play_frame = getPingPongIndex(play_frame, loopStartFrame, loopEndFrame);
	}

	f_cnt_t fragment_size = (f_cnt_t) (_frames * freq_factor) + MARGIN[_state->interpolationMode()];

	sampleFrame *tmp = nullptr;

	f_cnt_t usedFrames;
	// check whether we have to change pitch...
	if (freq_factor != 1.0 || _state->m_varyingPitch) {
		SRC_DATA src_data;
		// Generate output
		src_data.data_in =
				libSampleRateSrc(m_data->getSampleFragment(play_frame, fragment_size, _loopmode, &tmp, &is_backwards,
														   loopStartFrame, loopEndFrame, endFrame))->data();
		src_data.data_out = _ab->data();
		src_data.input_frames = fragment_size;
		src_data.output_frames = _frames;
		src_data.src_ratio = 1.0 / freq_factor;
		src_data.end_of_input = 0;
		int error = src_process(_state->m_resamplingData,
								&src_data);
		if (error) {
			printf("SampleBuffer: error while resampling: %s\n",
				   src_strerror(error));
		}
		if (src_data.output_frames_gen > _frames) {
			printf("SampleBuffer: not enough frames: %ld / %d\n",
				   src_data.output_frames_gen, _frames);
		}

		usedFrames = src_data.input_frames_used;
	} else {
		// we don't have to pitch, so we just copy the sample-data
		// as is into pitched-copy-buffer

		// Generate output
		memcpy(_ab,
			   m_data->getSampleFragment(play_frame, _frames, _loopmode, &tmp, &is_backwards,
										 loopStartFrame, loopEndFrame, endFrame),
			   _frames * BYTES_PER_FRAME);
		usedFrames = _frames;
	}

	// Advance
	switch (_loopmode) {
		case LoopOff:
			play_frame += usedFrames;
			break;
		case LoopOn:
			play_frame += usedFrames;
			play_frame = getLoopedIndex(play_frame, loopStartFrame, loopEndFrame);
			break;
		case LoopPingPong: {
			f_cnt_t left = usedFrames;
			if (_state->isBackwards()) {
				play_frame -= usedFrames;
				if (play_frame < loopStartFrame) {
					left -= (loopStartFrame - play_frame);
					play_frame = loopStartFrame;
				} else left = 0;
			}
			play_frame += left;
			play_frame = getPingPongIndex(play_frame, loopStartFrame, loopEndFrame);
			break;
		}
	}

	if (tmp != nullptr) {
		MM_FREE(tmp);
	}

	_state->setBackwards(is_backwards);
	_state->setFrameIndex(play_frame);

	for (fpp_t i = 0; i < _frames; ++i) {
		_ab[i][0] *= m_data->getAmplification();
		_ab[i][1] *= m_data->getAmplification();
	}

	return true;
}


f_cnt_t SampleBuffer::getLoopedIndex(f_cnt_t _index, f_cnt_t _startf, f_cnt_t _endf) {
	if (_index < _endf) {
		return _index;
	}
	return _startf + (_index - _startf)
					 % (_endf - _startf);
}


f_cnt_t SampleBuffer::getPingPongIndex(f_cnt_t _index, f_cnt_t _startf, f_cnt_t _endf) {
	if (_index < _endf) {
		return _index;
	}
	const f_cnt_t looplen = _endf - _startf;
	const f_cnt_t looppos = (_index - _endf) % (looplen * 2);

	return (looppos < looplen)
		   ? _endf - looppos
		   : _startf + (looppos - looplen);
}


void SampleBuffer::visualize(QPainter &_p, const QRect &_dr,
							 const QRect &_clip, f_cnt_t _from_frame, f_cnt_t _to_frame) {
	auto polyPair = visualizeToPoly(_dr, _clip, _from_frame, _to_frame);

	_p.setRenderHint(QPainter::Antialiasing);
	_p.drawPolyline(polyPair.first);
	_p.drawPolyline(polyPair.second);
}

std::pair<QPolygonF, QPolygonF> SampleBuffer::visualizeToPoly(const QRect &_dr, const QRect &_clip,
															  f_cnt_t _from_frame, f_cnt_t _to_frame) const {
	const int w = _dr.width();
	const int h = _dr.height();
	const bool focus_on_range = _from_frame < _to_frame;
	int y_space = (h / 2);

	/* Don't visualize while rendering / doing after-rendering changes. */
	return Engine::mixer()->runWhileNotRendering([=]() -> std::pair<QPolygonF, QPolygonF> {
		if (m_data->frames() == 0) return {};

		auto to_frame = _to_frame;
		if (to_frame > m_data->frames())
			to_frame = m_data->frames();


		const int nb_frames = focus_on_range ? to_frame - _from_frame : m_data->frames();
		if (nb_frames == 0) return {};

		const int fpp = tLimit<int>(nb_frames / w, 1, 20);

		bool shouldAddAdditionalPoint = (nb_frames % fpp) != 0;
		int pointsCount = (nb_frames / fpp) + (shouldAddAdditionalPoint ? 1 : 0);
		auto l = QPolygonF(pointsCount);
		auto r = QPolygonF(pointsCount);

		int n = 0;
		const int xb = _dr.x();
		const int first = focus_on_range ? _from_frame : 0;
		const int last = focus_on_range ? to_frame : m_data->frames();

		int zeroPoint = _dr.y() + y_space;
		if (h % 2 != 0)
			zeroPoint += 1;
		for (int frame = first; frame < last; frame += fpp) {
			double x = (xb + (frame - first) * double(w) / nb_frames);

			l[n] = QPointF(x,
						   (zeroPoint + (m_data->data()[frame][0] * y_space)));
			r[n] = QPointF(x,
						   (zeroPoint + (m_data->data()[frame][1] * y_space)));

			++n;
		}

		return {std::move(l), std::move(r)};
	});
}


QString SampleBuffer::openAudioFile(const QString &currentAudioFile) {
	FileDialog ofd(NULL, tr("Open audio file"));

	QString dir;
	if (!currentAudioFile.isEmpty()) {
		QString f = currentAudioFile;
		if (QFileInfo(f).isRelative()) {
			f = ConfigManager::inst()->userSamplesDir() + f;
			if (QFileInfo(f).exists() == false) {
				f = ConfigManager::inst()->factorySamplesDir() +
					currentAudioFile;
			}
		}
		dir = QFileInfo(f).absolutePath();
	} else {
		dir = ConfigManager::inst()->userSamplesDir();
	}
	// change dir to position of previously opened file
	ofd.setDirectory(dir);
	ofd.setFileMode(FileDialog::ExistingFiles);

	// set filters
	QStringList types;
	types << tr("All Audio-Files (*.wav *.ogg *.ds *.flac *.spx *.voc "
				"*.aif *.aiff *.au *.raw)")
		  << tr("Wave-Files (*.wav)")
		  << tr("OGG-Files (*.ogg)")
		  << tr("DrumSynth-Files (*.ds)")
		  << tr("FLAC-Files (*.flac)")
		  << tr("SPEEX-Files (*.spx)")
		  //<< tr( "MP3-Files (*.mp3)" )
		  //<< tr( "MIDI-Files (*.mid)" )
		  << tr("VOC-Files (*.voc)")
		  << tr("AIFF-Files (*.aif *.aiff)")
		  << tr("AU-Files (*.au)")
		  << tr("RAW-Files (*.raw)")
		//<< tr( "MOD-Files (*.mod)" )
			;
	ofd.setNameFilters(types);
	if (!currentAudioFile.isEmpty()) {
		// select previously opened file
		ofd.selectFile(QFileInfo(currentAudioFile).fileName());
	}

	if (ofd.exec() == QDialog::Accepted) {
		if (ofd.selectedFiles().isEmpty()) {
			return QString();
		}
		return tryToMakeRelative(ofd.selectedFiles()[0]);
	}

	return QString();
}


QString SampleBuffer::openAndSetAudioFile(const QString &currentAudioFile) {
	QString fileName = this->openAudioFile(currentAudioFile);

	if (!fileName.isEmpty()) {
		this->setAudioFile(fileName);
	}

	return fileName;
}


QString SampleBuffer::openAndSetWaveformFile(QString currentAudioFile) {
	if (currentAudioFile.isEmpty()) {
		currentAudioFile = ConfigManager::inst()->factorySamplesDir() + "waveforms/10saw.flac";
	}

	QString fileName = this->openAudioFile(currentAudioFile);

	if (!fileName.isEmpty()) {
		this->setAudioFile(fileName);
	}

	return fileName;
}

sample_t SampleBuffer::userWaveSample(const float _sample) const {
	auto data = guardedData();
	f_cnt_t dataFrames = data->frames();
	if (dataFrames == 0)
		return 0;

	const float frame = _sample * dataFrames;
	f_cnt_t f1 = static_cast<f_cnt_t>( frame ) % dataFrames;
	if (f1 < 0) {
		f1 += dataFrames;
	}

	return linearInterpolate(data->data()[f1][0], data->data()[(f1 + 1) % dataFrames][0], fraction(frame));
}


QString &SampleBuffer::toBase64(QString &_dst) const {
	auto data = guardedData();

	base64::encode((const char *) data->data(),
				   data->frames() * sizeof(sampleFrame), _dst);

	return _dst;
}

QFuture<void> SampleBuffer::setAmplification(float _a) {
	return runAsyncToSetData([_a](GuardedData &data) {
		data->setAmplification(_a);
	});
}

QFuture<void> SampleBuffer::setFrequency(float frequency) {
	return runAsyncToSetData([frequency](GuardedData &data) {
		data->setFrequency(frequency);
	});
}

QString SampleBuffer::tryToMakeRelative(const QString &file) {
	if (QFileInfo(file).isRelative() == false) {
		// Normalize the path
		QString f(QDir::cleanPath(file));

		// First, look in factory samples
		// Isolate "samples/" from "data:/samples/"
		QString samplesSuffix = ConfigManager::inst()->factorySamplesDir().mid(
				ConfigManager::inst()->dataDir().length());

		// Iterate over all valid "data:/" searchPaths
		for (const QString &path : QDir::searchPaths("data")) {
			QString samplesPath = QDir::cleanPath(path + samplesSuffix) + "/";
			if (f.startsWith(samplesPath)) {
				return QString(f).mid(samplesPath.length());
			}
		}

		// Next, look in user samples
		QString usd = ConfigManager::inst()->userSamplesDir();
		usd.replace(QDir::separator(), '/');
		if (f.startsWith(usd)) {
			return QString(f).mid(usd.length());
		}
	}
	return file;
}


QString SampleBuffer::tryToMakeAbsolute(const QString &file) {
	QFileInfo f(file);

	if (f.isRelative()) {
		f = QFileInfo(ConfigManager::inst()->userSamplesDir() + file);

		if (!f.exists()) {
			f = QFileInfo(ConfigManager::inst()->factorySamplesDir() + file);
		}
	}

	if (f.exists()) {
		return f.absoluteFilePath();
	}
	return file;
}

SampleBuffer::handleState::handleState(bool _varying_pitch, int interpolation_mode) :
		m_frameIndex(0),
		m_varyingPitch(_varying_pitch),
		m_isBackwards(false) {
	int error;
	m_interpolationMode = interpolation_mode;

	if ((m_resamplingData = src_new(interpolation_mode, DEFAULT_CHANNELS, &error)) == NULL) {
		qDebug("Error: src_new() failed in sample_buffer.cpp!\n");
	}
}


SampleBuffer::handleState::~handleState() {
	src_delete(m_resamplingData);
}

SampleBuffer::DataChangeHelper::DataChangeHelper(SampleBuffer *buffer, SampleBuffer::UpdateType updateType)
		:m_buffer{buffer}, m_updateType{updateType}
{
}

SampleBuffer::DataChangeHelper::~DataChangeHelper() {
	*m_buffer->m_playInfo = internal::SampleBufferPlayInfo(m_buffer->m_data->frames());

	m_buffer->m_infoChangeNotifier->onValueUpdated(m_buffer->createInfo());
	emit m_buffer->sampleUpdated(m_updateType);
}

void SampleBuffer::addData(const SampleBuffer::DataVector &vector, sample_rate_t sampleRate) {
	// First of all, don't let anyone read.
	DataChangeHelper helper(this, UpdateType::Append);

	m_data->addData(vector, sampleRate);
}

void SampleBuffer::resetData(DataVector &&newData, sample_rate_t dataSampleRate) {
	DataChangeHelper helper(this, UpdateType::Clear);
	m_data->resetData(std::move(newData), dataSampleRate);
}

void SampleBuffer::reverse() {
	DataChangeHelper helper(this, UpdateType::Clear);
	m_data->reverse();
}

void SampleBuffer::setAudioFile(const QString &audioFile, bool ignoreError)
{
	*this = SampleBuffer(audioFile, ignoreError);
}

SampleBuffer::SampleBuffer()
	: SampleBuffer{internal::SampleBufferData{}}
{
}

SampleBuffer::SampleBuffer(internal::SampleBufferData &&data)
		: m_data{std::make_shared<internal::SampleBufferData>(std::move(data))},
		  m_playInfo(std::make_shared<internal::SampleBufferPlayInfo>(m_data->frames()))
{
	emit sampleUpdated(UpdateType::Clear);
}

SampleBuffer::SampleBuffer(const QString &base64Data, sample_rate_t sample_rate)
		: SampleBuffer{internal::SampleBufferData::loadFromBase64(base64Data, sample_rate)} {
}
