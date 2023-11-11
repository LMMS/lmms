/*
 * SlicerT.cpp - simple slicer plugin
 *
 * Copyright (c) 2023 Daniel Kauss Serna <daniel.kauss.serna@gmail.com>
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

#include "SlicerT.h"

#include <QDomElement>
#include <cmath>
#include <fftw3.h>

#include "Engine.h"
#include "InstrumentTrack.h"
#include "PathUtil.h"
#include "Song.h"
#include "embed.h"
#include "lmms_constants.h"
#include "plugin_export.h"

namespace lmms {

extern "C" {
Plugin::Descriptor PLUGIN_EXPORT slicert_plugin_descriptor = {
	LMMS_STRINGIFY(PLUGIN_NAME),
	"SlicerT",
	QT_TRANSLATE_NOOP("PluginBrowser", "Basic Slicer"),
	"Daniel Kauss Serna <daniel.kauss.serna@gmail.com>",
	0x0100,
	Plugin::Type::Instrument,
	new PluginPixmapLoader("logo"),
	nullptr,
	nullptr,
};
} // end extern

// ################################# SlicerT ####################################

SlicerT::SlicerT(InstrumentTrack* instrumentTrack)
	: Instrument(instrumentTrack, &slicert_plugin_descriptor)
	, m_noteThreshold(0.6f, 0.0f, 2.0f, 0.01f, this, tr("Note threshold"))
	, m_fadeOutFrames(10.0f, 0.0f, 100.0f, 0.1f, this, tr("FadeOut"))
	, m_originalBPM(1, 1, 999, this, tr("Original bpm"))
	, m_sliceSnap(this, tr("Slice snap"))
	, m_enableSync(false, this, tr("BPM sync"))
	, m_originalSample()
	, m_parentTrack(instrumentTrack)
{
	m_sliceSnap.addItem("Off");
	m_sliceSnap.addItem("1/1");
	m_sliceSnap.addItem("1/2");
	m_sliceSnap.addItem("1/4");
	m_sliceSnap.addItem("1/8");
	m_sliceSnap.addItem("1/16");
	m_sliceSnap.addItem("1/32");
	m_sliceSnap.setValue(0);
}

void SlicerT::playNote(NotePlayHandle* handle, sampleFrame* workingBuffer)
{
	if (m_originalSample.frames() <= 1) { return; }

	int noteIndex = handle->key() - m_parentTrack->baseNote();
	const fpp_t frames = handle->framesLeftForCurrentPeriod();
	const f_cnt_t offset = handle->noteOffset();
	const int bpm = Engine::getSong()->getTempo();
	const float pitchRatio = 1 / std::exp2(m_parentTrack->pitchModel()->value() / 1200);

	float speedRatio = static_cast<float>(m_originalBPM.value()) / bpm;
	if (!m_enableSync.value()) { speedRatio = 1; }
	speedRatio *= pitchRatio;
	speedRatio *= Engine::audioEngine()->processingSampleRate() / static_cast<float>(m_originalSample.sampleRate());

	float sliceStart, sliceEnd;
	if (noteIndex == 0) // full sample at base note
	{
		sliceStart = 0;
		sliceEnd = 1;
	}
	else if (noteIndex > 0 && noteIndex < m_slicePoints.size())
	{
		noteIndex -= 1;
		sliceStart = m_slicePoints[noteIndex];
		sliceEnd = m_slicePoints[noteIndex + 1];
	}
	else
	{
		emit isPlaying(-1, 0, 0);
		return;
	}

	if (!handle->m_pluginData) { handle->m_pluginData = new PlaybackState(sliceStart); }
	auto playbackState = static_cast<PlaybackState*>(handle->m_pluginData);

	float noteDone = playbackState->noteDone();
	float noteLeft = sliceEnd - noteDone;

	if (noteLeft > 0)
	{
		int noteFrame = noteDone * m_originalSample.frames();

		SRC_STATE* resampleState = playbackState->resamplingState();
		SRC_DATA resampleData;
		resampleData.data_in = (m_originalSample.data() + noteFrame)->data();
		resampleData.data_out = (workingBuffer + offset)->data();
		resampleData.input_frames = noteLeft * m_originalSample.frames();
		resampleData.output_frames = frames;
		resampleData.src_ratio = speedRatio;

		src_process(resampleState, &resampleData);

		float nextNoteDone = noteDone + frames * (1.0f / speedRatio) / m_originalSample.frames();
		playbackState->setNoteDone(nextNoteDone);

		// exponential fade out, applyRelease() not used since it extends the note length
		int fadeOutFrames = m_fadeOutFrames.value() / 1000.0f * Engine::audioEngine()->processingSampleRate();
		int noteFramesLeft = noteLeft * m_originalSample.frames() * speedRatio;
		for (int i = 0; i < frames; i++)
		{
			float fadeValue = static_cast<float>(noteFramesLeft - i) / fadeOutFrames;
			fadeValue = std::clamp(fadeValue, 0.0f, 1.0f);
			fadeValue = cosinusInterpolate(0, 1, fadeValue);

			workingBuffer[i + offset][0] *= fadeValue;
			workingBuffer[i + offset][1] *= fadeValue;
		}

		instrumentTrack()->processAudioBuffer(workingBuffer, frames + offset, handle);

		emit isPlaying(noteDone, sliceStart, sliceEnd);
	}
	else { emit isPlaying(-1, 0, 0); }
}

void SlicerT::deleteNotePluginData(NotePlayHandle* handle)
{
	delete static_cast<PlaybackState*>(handle->m_pluginData);
}

// uses the spectral flux to determine the change in magnitude
// resources:
// http://www.iro.umontreal.ca/~pift6080/H09/documents/papers/bello_onset_tutorial.pdf
void SlicerT::findSlices()
{
	if (m_originalSample.frames() <= 1) { return; }
	m_slicePoints = {};

	const int windowSize = 512;
	const float minBeatLength = 0.05f; // in seconds, ~ 1/4 length at 220 bpm

	int sampleRate = m_originalSample.sampleRate();
	int minDist = sampleRate * minBeatLength;

	float maxMag = -1;
	std::vector<float> singleChannel(m_originalSample.frames(), 0);
	for (int i = 0; i < m_originalSample.frames(); i++)
	{
		singleChannel[i] = (m_originalSample.data()[i][0] + m_originalSample.data()[i][1]) / 2;
		maxMag = std::max(maxMag, singleChannel[i]);
	}

	// normalize and find 0 crossings
	std::vector<int> zeroCrossings;
	float lastValue = 1;
	for (int i = 0; i < singleChannel.size(); i++)
	{
		singleChannel[i] /= maxMag;
		if (sign(lastValue) != sign(singleChannel[i]))
		{
			zeroCrossings.push_back(i);
			lastValue = singleChannel[i];
		}
	}

	std::vector<float> prevMags(windowSize / 2, 0);
	std::vector<float> fftIn(windowSize, 0);
	std::array<fftwf_complex, windowSize> fftOut;

	fftwf_plan fftPlan = fftwf_plan_dft_r2c_1d(windowSize, fftIn.data(), fftOut.data(), FFTW_MEASURE);

	int lastPoint = -minDist - 1; // to always store 0 first
	float spectralFlux = 0;
	float prevFlux = 1E-10; // small value, no divison by zero
	float real, imag, magnitude, diff;

	for (int i = 0; i < singleChannel.size() - windowSize; i += windowSize)
	{
		// fft
		std::copy_n(singleChannel.data() + i, windowSize, fftIn.data());
		fftwf_execute(fftPlan);

		// calculate spectral flux in regard to last window
		for (int j = 0; j < windowSize / 2; j++) // only use niquistic frequencies
		{
			real = fftOut[j][0];
			imag = fftOut[j][1];
			magnitude = std::sqrt(real * real + imag * imag);

			// using L2-norm (euclidean distance)
			diff = std::sqrt(std::pow(magnitude - prevMags[j], 2));
			spectralFlux += diff;

			prevMags[j] = magnitude;
		}

		if (spectralFlux / prevFlux > 1.0f + m_noteThreshold.value() && i - lastPoint > minDist)
		{
			m_slicePoints.push_back(i);
			lastPoint = i;
			if (m_slicePoints.size() > 128) { break; } // no more keys on the keyboard
		}

		prevFlux = spectralFlux;
		spectralFlux = 1E-10; // again for no divison by zero
	}

	m_slicePoints.push_back(m_originalSample.frames());

	for (float& sliceValue : m_slicePoints)
	{
		int closestZeroCrossing = *std::lower_bound(zeroCrossings.begin(), zeroCrossings.end(), sliceValue);
		if (std::abs(sliceValue - closestZeroCrossing) < windowSize) { sliceValue = closestZeroCrossing; }
	}

	float beatsPerMin = m_originalBPM.value() / 60.0f;
	float samplesPerBeat = m_originalSample.sampleRate() / beatsPerMin * 4.0f;
	int noteSnap = m_sliceSnap.value();
	int sliceLock = samplesPerBeat / std::exp2(noteSnap + 1);
	if (noteSnap == 0) { sliceLock = 1; }
	for (float& sliceValue : m_slicePoints)
	{
		sliceValue += sliceLock / 2;
		sliceValue -= static_cast<int>(sliceValue) % sliceLock;
	}

	m_slicePoints.erase(std::unique(m_slicePoints.begin(), m_slicePoints.end()), m_slicePoints.end());

	for (float& sliceIndex : m_slicePoints)
	{
		sliceIndex /= m_originalSample.frames();
	}

	m_slicePoints[0] = 0;
	m_slicePoints[m_slicePoints.size() - 1] = 1;

	emit dataChanged();
}

// find the bpm of the sample by assuming its in 4/4 time signature ,
// and lies in the 100 - 200 bpm range
void SlicerT::findBPM()
{
	if (m_originalSample.frames() <= 1) { return; }

	float sampleRate = m_originalSample.sampleRate();
	float totalFrames = m_originalSample.frames();
	float sampleLength = totalFrames / sampleRate;

	float bpmEstimate = 240.0f / sampleLength;

	while (bpmEstimate < 100)
	{
		bpmEstimate *= 2;
	}

	while (bpmEstimate > 200)
	{
		bpmEstimate /= 2;
	}

	m_originalBPM.setValue(bpmEstimate);
	m_originalBPM.setInitValue(bpmEstimate);
}

std::vector<Note> SlicerT::getMidi()
{
	std::vector<Note> outputNotes;

	float speedRatio = static_cast<float>(m_originalBPM.value()) / Engine::getSong()->getTempo();
	float outFrames = m_originalSample.frames() * speedRatio;

	float framesPerTick = Engine::framesPerTick();
	float totalTicks = outFrames / framesPerTick;
	float lastEnd = 0;

	for (int i = 0; i < m_slicePoints.size() - 1; i++)
	{
		float sliceStart = lastEnd;
		float sliceEnd = totalTicks * m_slicePoints[i + 1];

		Note sliceNote = Note();
		sliceNote.setKey(i + m_parentTrack->baseNote() + 1);
		sliceNote.setPos(sliceStart);
		sliceNote.setLength(sliceEnd - sliceStart + 1); // + 1 so that the notes allign
		outputNotes.push_back(sliceNote);

		lastEnd = sliceEnd;
	}

	return outputNotes;
}

void SlicerT::updateFile(QString file)
{
	m_originalSample.setAudioFile(file);

	findBPM();
	findSlices();

	emit dataChanged();
}

void SlicerT::updateSlices()
{
	findSlices();
}

void SlicerT::saveSettings(QDomDocument& document, QDomElement& element)
{
	element.setAttribute("version", "1");
	element.setAttribute("src", m_originalSample.audioFile());
	if (m_originalSample.audioFile().isEmpty())
	{
		QString s;
		element.setAttribute("sampledata", m_originalSample.toBase64(s));
	}

	element.setAttribute("totalSlices", static_cast<int>(m_slicePoints.size()));
	for (int i = 0; i < m_slicePoints.size(); i++)
	{
		element.setAttribute(tr("slice_%1").arg(i), m_slicePoints[i]);
	}

	m_fadeOutFrames.saveSettings(document, element, "fadeOut");
	m_noteThreshold.saveSettings(document, element, "threshold");
	m_originalBPM.saveSettings(document, element, "origBPM");
	m_enableSync.saveSettings(document, element, "syncEnable");
}

void SlicerT::loadSettings(const QDomElement& element)
{
	if (!element.attribute("src").isEmpty())
	{
		m_originalSample.setAudioFile(element.attribute("src"));

		QString absolutePath = PathUtil::toAbsolute(m_originalSample.audioFile());
		if (!QFileInfo(absolutePath).exists())
		{
			QString message = tr("Sample not found: %1").arg(m_originalSample.audioFile());
			Engine::getSong()->collectError(message);
		}
	}
	else if (!element.attribute("sampledata").isEmpty())
	{
		m_originalSample.loadFromBase64(element.attribute("srcdata"));
	}

	if (!element.attribute("totalSlices").isEmpty())
	{
		int totalSlices = element.attribute("totalSlices").toInt();
		m_slicePoints = {};
		for (int i = 0; i < totalSlices; i++)
		{
			m_slicePoints.push_back(element.attribute(tr("slice_%1").arg(i)).toFloat());
		}
	}

	m_fadeOutFrames.loadSettings(element, "fadeOut");
	m_noteThreshold.loadSettings(element, "threshold");
	m_originalBPM.loadSettings(element, "origBPM");
	m_enableSync.loadSettings(element, "syncEnable");

	emit dataChanged();
}

QString SlicerT::nodeName() const
{
	return slicert_plugin_descriptor.name;
}

gui::PluginView* SlicerT::instantiateView(QWidget* parent)
{
	return new gui::SlicerTView(this, parent);
}

extern "C" {
PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* m, void*)
{
	return new SlicerT(static_cast<InstrumentTrack*>(m));
}
} // extern
} // namespace lmms
