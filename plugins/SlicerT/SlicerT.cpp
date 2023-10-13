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
#include <fftw3.h>
#include <math.h>

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
	new PluginPixmapLoader("icon"),
	nullptr,
	nullptr,
};
} // end extern

PhaseVocoder::PhaseVocoder()
	: m_FFTSpectrum(s_windowSize)
	, m_FFTInput(s_windowSize, 0)
	, m_IFFTReconstruction(s_windowSize, 0)
	, m_allMagnitudes(s_windowSize, 0)
	, m_allFrequencies(s_windowSize, 0)
	, m_processedFreq(s_windowSize, 0)
	, m_processedMagn(s_windowSize, 0)
{
	m_fftPlan = fftwf_plan_dft_r2c_1d(s_windowSize, m_FFTInput.data(), m_FFTSpectrum.data(), FFTW_MEASURE);
	m_ifftPlan = fftwf_plan_dft_c2r_1d(s_windowSize, m_FFTSpectrum.data(), m_IFFTReconstruction.data(), FFTW_MEASURE);
}

PhaseVocoder::~PhaseVocoder()
{
	fftwf_destroy_plan(m_fftPlan);
	fftwf_destroy_plan(m_ifftPlan);
}

void PhaseVocoder::loadData(std::vector<float> originalData, int sampleRate, float newRatio)
{
	m_dataLock.lock();

	m_originalBuffer = originalData;
	m_originalSampleRate = sampleRate;
	m_scaleRatio = -1; // force update, kinda hacky

	m_dataLock.unlock(); // stupid, but QRecursiveMutex is too expensive to have in updateParas and getFrames
	updateParams(newRatio);
	m_dataLock.lock();

	// set buffer sizes
	m_processedWindows.resize(m_numWindows, false);
	m_lastPhase.resize(m_numWindows * s_windowSize, 0);
	m_sumPhase.resize((m_numWindows + 1) * s_windowSize, 0);
	m_freqCache.resize(m_numWindows * s_windowSize, 0);
	m_magCache.resize(m_numWindows * s_windowSize, 0);

	// clear phase buffers
	std::fill(m_lastPhase.begin(), m_lastPhase.end(), 0);
	std::fill(m_sumPhase.begin(), m_sumPhase.end(), 0);

	// maybe limit this to a set amount of windows to reduce initial lag spikes
	for (int i = 0; i < m_numWindows; i++)
	{
		if (!m_processedWindows[i])
		{
			generateWindow(i, false); // first pass, no cache
			m_processedWindows[i] = true;
		}
	}

	m_dataLock.unlock();
}

void PhaseVocoder::getFrames(std::vector<float>& outData, int start, int frames)
{
	if (m_originalBuffer.size() < 2048) { return; }
	m_dataLock.lock();

	if (m_scaleRatio == 1) { // directly copy original data
		std::copy_n(m_originalBuffer.data() + start, frames, outData.data());
		m_dataLock.unlock();
		return;
	}

	int windowMargin = s_overSampling / 2; // numbers of windows before full quality
	int startWindow = (float)start / m_outStepSize - windowMargin;
	int endWindow = (float)(start + frames) / m_outStepSize + windowMargin;

	startWindow = std::clamp(startWindow, 0, m_numWindows - 1);
	endWindow = std::clamp(endWindow, 0, m_numWindows - 1);

	// discard previous phaseSum if not processed
	if (!m_processedWindows[startWindow])
	{
		std::fill_n(m_sumPhase.data() + startWindow * s_windowSize, s_windowSize, 0);
	}

	// this encompases the minimum windows needed to get full quality,
	// which must be computed
	for (int i = startWindow; i < endWindow; i++)
	{
		if (!m_processedWindows[i])
		{
			generateWindow(i, true); // theses should use the cache
			m_processedWindows[i] = true;
		}
	}

	for (int i = 0; i < frames; i++)
	{
		outData[i] = m_processedBuffer[start + i];
	}

	m_dataLock.unlock();
}

// adjust pv params buffers to a new scale ratio
void PhaseVocoder::updateParams(float newRatio)
{
	if (m_originalBuffer.size() < 2048) { return; }
	if (newRatio == m_scaleRatio) { return; } // nothing changed
	m_dataLock.lock();

	m_scaleRatio = newRatio;
	m_stepSize = (float)s_windowSize / s_overSampling;
	m_numWindows = (float)m_originalBuffer.size() / m_stepSize - s_overSampling - 1;
	m_outStepSize = m_scaleRatio * (float)m_stepSize; // float, else inaccurate
	m_freqPerBin = m_originalSampleRate / s_windowSize;
	m_expectedPhaseIn = 2. * F_PI * (float)m_stepSize / (float)s_windowSize;
	m_expectedPhaseOut = 2. * F_PI * (float)m_outStepSize / (float)s_windowSize;

	m_processedBuffer.resize(m_scaleRatio * m_originalBuffer.size(), 0);

	// very slow :(
	std::fill(m_processedWindows.begin(), m_processedWindows.end(), false);
	std::fill(m_processedBuffer.begin(), m_processedBuffer.end(), 0);

	m_dataLock.unlock();
}

// time shifts one window from originalBuffer and writes to m_processedBuffer
// resources:
// http://blogs.zynaptiq.com/bernsee/pitch-shifting-using-the-ft/
// https://sethares.engr.wisc.edu/vocoders/phasevocoder.html
// https://dsp.stackexchange.com/questions/40101/audio-time-stretching-without-pitch-shifting/40367#40367
// https://www.guitarpitchshifter.com/
// https://en.wikipedia.org/wiki/Window_function
void PhaseVocoder::generateWindow(int windowNum, bool useCache)
{
	// declare vars
	float real, imag, phase, magnitude, freq, deltaPhase = 0;
	int windowStart = (float)windowNum * m_stepSize;
	int windowIndex = (float)windowNum * s_windowSize;

	if (!useCache)
	{ // normal stuff
		std::copy_n(m_originalBuffer.data() + windowStart, s_windowSize, m_FFTInput.data());

		// FFT
		fftwf_execute(m_fftPlan);

		// analysis step
		for (int j = 0; j < s_windowSize / 2; j++) // only process nyquistic frequency
		{
			real = m_FFTSpectrum[j][0];
			imag = m_FFTSpectrum[j][1];

			magnitude = 2. * sqrt(real * real + imag * imag);
			phase = atan2(imag, real);

			// calculate difference in phase with prev window
			freq = phase;
			freq = phase
				- m_lastPhase[std::max(0, windowIndex + j - s_windowSize)]; // subtract prev pahse to get phase diference
			m_lastPhase[windowIndex + j] = phase;

			freq -= (float)j * m_expectedPhaseIn; // subtract expected phase
			// at this point, freq is the difference in phase
			// between the last phase, having removed the expected phase at this point in the sample

			// this puts freq in 0-2pi. Since the phase difference is proportional to the deviation in bin frequency,
			// with this we can better estimate the true frequency
			freq = fmod(freq + F_PI, -2.0f * F_PI) + F_PI;

			// convert phase difference into bin freq mulitplier
			freq = (float)s_overSampling * freq / (2. * F_PI);

			// add to the expected freq the change in freq calculated from the phase diff
			freq = (float)j * m_freqPerBin + freq * m_freqPerBin;

			m_allMagnitudes[j] = magnitude;
			m_allFrequencies[j] = freq;
		}
		// write cache
		std::copy_n(m_allFrequencies.data(), s_windowSize, m_freqCache.data() + windowIndex);
		std::copy_n(m_allMagnitudes.data(), s_windowSize, m_magCache.data() + windowIndex);
	}
	else
	{
		// read cache
		std::copy_n(m_freqCache.data() + windowIndex, s_windowSize, m_allFrequencies.data());
		std::copy_n(m_magCache.data() + windowIndex, s_windowSize, m_allMagnitudes.data());
	}

	// synthesis, all the operations are the reverse of the analysis
	for (int j = 0; j < s_windowSize / 2; j++)
	{
		magnitude = m_allMagnitudes[j];
		freq = m_allFrequencies[j];

		// difference to bin freq mulitplier
		deltaPhase = freq - (float)j * m_freqPerBin;

		// convert to phase difference
		deltaPhase /= m_freqPerBin;

		// difference in phase
		deltaPhase = 2. * F_PI * deltaPhase / s_overSampling;

		// add the expected phase
		deltaPhase += (float)j * m_expectedPhaseOut;

		// sum this phase to the total, to keep track of the out phase along the sample
		m_sumPhase[windowIndex + j] += deltaPhase;
		deltaPhase = m_sumPhase[windowIndex + j]; // final bin phase

		m_sumPhase[windowIndex + j + s_windowSize] = deltaPhase; // copy to the next


		m_FFTSpectrum[j][0] = magnitude * cos(deltaPhase);
		m_FFTSpectrum[j][1] = magnitude * sin(deltaPhase);
	}

	// inverse fft
	fftwf_execute(m_ifftPlan);

	// windowing
	for (int j = 0; j < s_windowSize; j++)
	{
		float outIndex = windowNum * m_outStepSize + j;

		// blackman-harris window
		float a0 = 0.35875f;
		float a1 = 0.48829f;
		float a2 = 0.14128f;
		float a3 = 0.01168f;

		float piN2 = 2.0f * F_PI * j;
		float window = a0 - (a1 * cos(piN2 / s_windowSize)) + (a2 * cos(2.0f * piN2 / s_windowSize)) - (a3 * cos(3.0f * piN2));

		// inverse fft magnitudes are windowsSize times bigger
		m_processedBuffer[outIndex] += window * (m_IFFTReconstruction[j] / s_windowSize / s_overSampling);
	}
}

// ################################# SlicerT ####################################

SlicerT::SlicerT(InstrumentTrack* instrumentTrack)
	: Instrument(instrumentTrack, &slicert_plugin_descriptor)
	, m_noteThreshold(0.6f, 0.0f, 2.0f, 0.01f, this, tr("Note threshold"))
	, m_fadeOutFrames(400.0f, 0.0f, 8192.0f, 1.0f, this, tr("FadeOut"))
	, m_originalBPM(1, 1, 999, this, tr("Original bpm"))
	, m_sliceSnap(this, tr("Slice snap"))
	, m_enableSync(true, this, tr("BPM sync"))
	, m_originalSample()
	, m_phaseVocoder()
	, m_parentTrack(instrumentTrack)
{
	m_sliceSnap.addItem("Off");
	m_sliceSnap.addItem("1/1");
	m_sliceSnap.addItem("1/2");
	m_sliceSnap.addItem("1/4");
	m_sliceSnap.addItem("1/8");
	m_sliceSnap.addItem("1/16");
	m_sliceSnap.addItem("1/32");
	m_sliceSnap.setValue(0); // no snap by default
}

void SlicerT::playNote(NotePlayHandle* handle, sampleFrame* workingBuffer)
{
	if (m_originalSample.frames() < 2048) { return; }

	// playback parameters
	const int noteIndex = handle->key() - m_parentTrack->baseNote();
	const int playedFrames = handle->totalFramesPlayed();
	const fpp_t frames = handle->framesLeftForCurrentPeriod();
	const f_cnt_t offset = handle->noteOffset();
	const int bpm = Engine::getSong()->getTempo();
	const float pitchRatio = pow(2, m_parentTrack->pitchModel()->value() / 1200);
	const float inversePitchRatio = 1.0f / pitchRatio;

	// update scaling parameters
	float speedRatio = (float)m_originalBPM.value() / bpm;
	if (!m_enableSync.value()) { speedRatio = 1; } // disable timeshift
	m_phaseVocoder.setScaleRatio(speedRatio);
	speedRatio *= inversePitchRatio; // adjust for pitch bend

	int totalFrames = inversePitchRatio * m_phaseVocoder.frames(); // adjust frames played with regards to pitch
	int sliceStart, sliceEnd;
	if (noteIndex > m_slicePoints.size() - 2 || noteIndex < 0) // full sample if ouside range
	{
		sliceStart = 0;
		sliceEnd = totalFrames;
	}
	else
	{
		sliceStart = m_slicePoints[noteIndex] * speedRatio;
		sliceEnd = m_slicePoints[noteIndex + 1] * speedRatio;
	}

	// slice vars
	int sliceFrames = sliceEnd - sliceStart;
	int currentNoteFrame = sliceStart + playedFrames;
	int noteFramesLeft = sliceFrames - playedFrames;

	if (noteFramesLeft > 0)
	{
		int framesToCopy = pitchRatio * frames + 1; // just in case
		int framesIndex = std::min((int)(pitchRatio * currentNoteFrame), m_phaseVocoder.frames() - framesToCopy);

		// load sample segmengt, with regards to pitch settings
		std::vector<sampleFrame> prePitchBuffer(framesToCopy, {0.0f, 0.0f});
		m_phaseVocoder.getFrames(prePitchBuffer.data(), framesIndex, framesToCopy);

		// if pitch is changed, resample, else just copy
		if (pitchRatio != 1.0f)
		{
			SRC_DATA resamplerData;

			resamplerData.data_in = (float*)prePitchBuffer.data();	   // wtf
			resamplerData.data_out = (float*)(workingBuffer + offset); // wtf is this
			resamplerData.input_frames = prePitchBuffer.size();
			resamplerData.output_frames = frames;
			resamplerData.src_ratio = inversePitchRatio;

			src_simple(&resamplerData, SRC_LINEAR, 2);
		}
		else { memcpy(workingBuffer + offset, prePitchBuffer.data(), frames * sizeof(sampleFrame)); }

		// exponential fade out, applyRelease kinda sucks
		if (noteFramesLeft < m_fadeOutFrames.value())
		{
			for (int i = 0; i < frames; i++)
			{
				float fadeValue = (float)(noteFramesLeft - i) / m_fadeOutFrames.value();
				// if the workingbuffer extends the sample
				fadeValue = std::clamp(fadeValue, 0.0f, 1.0f);
				fadeValue = pow(fadeValue, 2);

				workingBuffer[i + offset][0] *= fadeValue;
				workingBuffer[i + offset][1] *= fadeValue;
			}
		}

		instrumentTrack()->processAudioBuffer(workingBuffer, frames + offset, handle);

		// calculate absolute for the SlicerTWaveform
		float absoluteCurrentNote = (float)currentNoteFrame / totalFrames;
		float absoluteStartNote = (float)sliceStart / totalFrames;
		float abslouteEndNote = (float)sliceEnd / totalFrames;
		emit isPlaying(absoluteCurrentNote, absoluteStartNote, abslouteEndNote);
	}
	else { emit isPlaying(-1, 0, 0); }
}

// uses the spectral flux to determine the change in magnitude
// resources:
// http://www.iro.umontreal.ca/~pift6080/H09/documents/papers/bello_onset_tutorial.pdf
void SlicerT::findSlices()
{
	if (m_originalSample.frames() < 2048) { return; }
	m_slicePoints = {};

	// computacion params
	const int windowSize = 512;
	const float minBeatLength = 0.05f; // in seconds, ~ 1/4 length at 220 bpm

	int sampleRate = m_originalSample.sampleRate();
	int minDist = sampleRate * minBeatLength;

	// copy vector into one vector, averaging channels
	float maxMag = -1;
	std::vector<float> singleChannel(m_originalSample.frames(), 0);
	for (int i = 0; i < m_originalSample.frames(); i++)
	{
		singleChannel[i] = (m_originalSample.data()[i][0] + m_originalSample.data()[i][1]) / 2;
		maxMag = std::max(maxMag, singleChannel[i]);
	}

	// normalize
	for (int i = 0; i < singleChannel.size(); i++)
	{
		singleChannel[i] /= maxMag;
	}

	// buffers
	std::vector<float> prevMags(windowSize / 2, 0);
	std::vector<float> fftIn(windowSize, 0);
	std::vector<fftwf_complex> fftOut(windowSize);

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
			magnitude = sqrt(real * real + imag * imag);

			// using L2-norm (euclidean distance)
			diff = sqrt(pow(magnitude - prevMags[j], 2));
			spectralFlux += diff;

			prevMags[j] = magnitude;
		}

		// detect increases in flux
		if (spectralFlux / prevFlux > 1.0f + m_noteThreshold.value() && i - lastPoint > minDist)
		{
			m_slicePoints.push_back(i);
			lastPoint = i;
		}

		prevFlux = spectralFlux;
		spectralFlux = 1E-10; // again for no divison by zero
	}

	m_slicePoints.push_back(m_originalSample.frames());

	// snap slices to notes
	int noteSnap = m_sliceSnap.value();
	int timeSignature = Engine::getSong()->getTimeSigModel().getNumerator();
	int samplesPerBar = 60.0f * timeSignature / m_originalBPM.value() * m_originalSample.sampleRate();
	int sliceLock = samplesPerBar / pow(2, noteSnap + 1); // lock to note: 1 / noteSnap²
	if (noteSnap == 0) { sliceLock = 1; }				  // disable noteSnap

	for (int i = 0; i < m_slicePoints.size(); i++)
	{
		m_slicePoints[i] += sliceLock / 2;
		m_slicePoints[i] -= m_slicePoints[i] % sliceLock;
	}

	// remove duplicates
	m_slicePoints.erase(std::unique(m_slicePoints.begin(), m_slicePoints.end()), m_slicePoints.end());

	// fit to sample size
	m_slicePoints[0] = 0;
	m_slicePoints[m_slicePoints.size()-1] = m_originalSample.frames();

	// update UI
	emit dataChanged();
}

// find the bpm of the sample by assuming its in 4/4 time signature ,
// and lies in the 100 - 200 bpm range
void SlicerT::findBPM()
{
	if (m_originalSample.frames() < 2048) { return; }

	// caclulate length of sample
	float sampleRate = m_originalSample.sampleRate();
	float totalFrames = m_originalSample.frames();
	float sampleLength = totalFrames / sampleRate;

	// this assumes the sample has a time signature of x/4
	float bpmEstimate = 240.0f / sampleLength;

	// get into 100 - 200 range
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

void SlicerT::writeToMidi(std::vector<Note>* outClip)
{
	if (m_originalSample.frames() < 2048) { return; }

	// update incase bpm changed
	float speedRatio = (float)m_originalBPM.value() / Engine::getSong()->getTempo();
	m_phaseVocoder.setScaleRatio(speedRatio);

	// calculate how many "beats" are in the sample
	float ticksPerBar = DefaultTicksPerBar;
	float sampleRate = m_originalSample.sampleRate();
	float bpm = Engine::getSong()->getTempo();
	float samplesPerBeat = 60.0f / bpm * sampleRate;
	float beats = (float)m_phaseVocoder.frames() / samplesPerBeat;

	// calculate how many ticks in sample
	float barsInSample = beats / Engine::getSong()->getTimeSigModel().getDenominator();
	float totalTicks = ticksPerBar * barsInSample;

	float lastEnd = 0;

	// write to midi
	for (int i = 0; i < m_slicePoints.size() - 1; i++)
	{
		float sliceStart = lastEnd;
		float sliceEnd = (float)m_slicePoints[i + 1] / m_originalSample.frames() * totalTicks;

		Note sliceNote = Note();
		sliceNote.setKey(i + m_parentTrack->baseNote());
		sliceNote.setPos(sliceStart);
		sliceNote.setLength(sliceEnd - sliceStart + 1); // + 1 needed for whatever reason
		outClip->push_back(sliceNote);

		lastEnd = sliceEnd;
	}
}

void SlicerT::updateFile(QString file)
{
	m_originalSample.setAudioFile(file);
	if (m_originalSample.frames() < 2048) { return; }

	findBPM();
	findSlices();

	float speedRatio = (float)m_originalBPM.value() / Engine::getSong()->getTempo();
	m_phaseVocoder.loadSample(
		m_originalSample.data(), m_originalSample.frames(), m_originalSample.sampleRate(), speedRatio);

	emit dataChanged();
}

void SlicerT::updateSlices()
{
	findSlices();
}

void SlicerT::saveSettings(QDomDocument& document, QDomElement& element)
{
	// save sample
	element.setAttribute("src", m_originalSample.audioFile());
	if (m_originalSample.audioFile().isEmpty())
	{
		QString s;
		element.setAttribute("sampledata", m_originalSample.toBase64(s));
	}

	// save slice points
	element.setAttribute("totalSlices", (int)m_slicePoints.size());
	for (int i = 0; i < m_slicePoints.size(); i++)
	{
		element.setAttribute(tr("slice_%1").arg(i), m_slicePoints[i]);
	}

	// save knobs
	m_fadeOutFrames.saveSettings(document, element, "fadeOut");
	m_noteThreshold.saveSettings(document, element, "threshold");
	m_originalBPM.saveSettings(document, element, "origBPM");
}

void SlicerT::loadSettings(const QDomElement& element)
{
	// load sample
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

	// load slices
	if (!element.attribute("totalSlices").isEmpty())
	{
		int totalSlices = element.attribute("totalSlices").toInt();
		m_slicePoints = {};
		for (int i = 0; i < totalSlices; i++)
		{
			m_slicePoints.push_back(element.attribute(tr("slice_%1").arg(i)).toInt());
		}
	}

	// load knobs
	m_fadeOutFrames.loadSettings(element, "fadeOut");
	m_noteThreshold.loadSettings(element, "threshold");
	m_originalBPM.loadSettings(element, "origBPM");

	// create dynamic buffer
	float speedRatio = (float)m_originalBPM.value() / Engine::getSong()->getTempo();
	m_phaseVocoder.loadSample(
		m_originalSample.data(), m_originalSample.frames(), m_originalSample.sampleRate(), speedRatio);

	emit dataChanged();
}

QString SlicerT::nodeName() const
{
	return (slicert_plugin_descriptor.name);
}

gui::PluginView* SlicerT::instantiateView(QWidget* parent)
{
	return (new gui::SlicerTView(this, parent));
}

extern "C" {
// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* m, void*)
{
	return (new SlicerT(static_cast<InstrumentTrack*>(m)));
}
} // extern
} // namespace lmms
