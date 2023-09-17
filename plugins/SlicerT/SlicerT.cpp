/*
 * SlicerT.cpp - simple slicer plugin
 *
 * Copyright (c) 2006-2008 Andreas Brandmaier <andy/at/brandmaier/dot/de>
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

// TODO: fix changing sample while playing broken
// TODO: add cache to PhaseVocoder
// TODO: cleaup loading, saving
// TODO: maybe add grace period while changing velocity, there are artifacts when changing bpm repetedly
// TODO: general performance, set frames from getFrames to fixed, reduce vector creation as much as posible
// TODO: switch to arrayVector
// TODO: volume normalizazion, atm just reduce by x amount
// TODO: cleaunp UI classes
// TODO: implment roxas new UI

#include "SlicerT.h"

#include <fftw3.h>
#include <QDomElement>

#include "Engine.h"
#include "Song.h"
#include "InstrumentTrack.h"

#include "PathUtil.h"
#include "embed.h"
#include "plugin_export.h"


namespace lmms
{

extern "C"
{
Plugin::Descriptor PLUGIN_EXPORT slicert_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"SlicerT",
	QT_TRANSLATE_NOOP( "PluginBrowser",
				"Basic Slicer" ),
	"Daniel Kauss Serna <daniel.kauss.serna/at/gmail.com>",
	0x0100,
	Plugin::Type::Instrument,
	new PluginPixmapLoader( "logo" ),
	nullptr,
	nullptr,
} ;
} // end extern


PhaseVocoder::PhaseVocoder() :
	FFTInput(windowSize, 0),
	IFFTReconstruction(windowSize, 0),
	allMagnitudes(windowSize, 0),
	allFrequencies(windowSize, 0),
	processedFreq(windowSize, 0),
	processedMagn(windowSize, 0)
{
	fftPlan = fftwf_plan_dft_r2c_1d(windowSize, FFTInput.data(), FFTSpectrum, FFTW_MEASURE);
	ifftPlan = fftwf_plan_dft_c2r_1d(windowSize, FFTSpectrum, IFFTReconstruction.data(), FFTW_MEASURE);
	// loadSample(originalData, frames, sampleRate);
}

PhaseVocoder::~PhaseVocoder() {
	fftwf_destroy_plan(fftPlan);
	fftwf_destroy_plan(ifftPlan);
}

void PhaseVocoder::loadData(std::vector<float> originalData, int sampleRate, float newRatio) {
	originalBuffer = originalData;
	originalSampleRate = sampleRate;
	scaleRatio = -1; // force update, kinda hacky
	updateParams(newRatio);

	for (int i = 0;i<numWindows;i++) {
		if (!m_processedWindows[i]) {
			generateWindow(originalBuffer, processedBuffer, i);
			m_processedWindows[i] = true;
		}
	}
}

void PhaseVocoder::getFrames(std::vector<float> & outData, int start, int frames) {
	if (originalBuffer.size() < 2048) { return; }
	dataLock.lock();

	int windowMargin = overSampling / 2; // numbers of windows before full quality
	int startWindow = std::max(0.0f, (float)start / outStepSize - windowMargin);
	int endWindow = std::min((float)numWindows, (float)(start + frames) / outStepSize + windowMargin);
	// this encompases the minimum windows needed to get full quality,
	// which must be computed
	for (int i = startWindow;i<endWindow;i++) {
		if (!m_processedWindows[i]) {
			generateWindow(originalBuffer, processedBuffer, i);
			m_processedWindows[i] = true;
		}
	}

	for (int i = 0;i<frames;i++) {
		outData[i] = processedBuffer[start + i];
	}

	dataLock.unlock();
}

void PhaseVocoder::updateParams(float newRatio) {
	if (originalBuffer.size() < 2048) { return; }
	if (newRatio == scaleRatio) { return; }
	dataLock.lock();

	printf("ratio: %f\n", newRatio);

	scaleRatio = newRatio;
	stepSize = (float)windowSize / overSampling;
	numWindows = (float)originalBuffer.size() / stepSize - overSampling - 1;
	outStepSize = scaleRatio * (float)stepSize; // float, else inaccurate
	freqPerBin = originalSampleRate/windowSize;
	expectedPhaseIn = 2.*M_PI*(float)stepSize/(float)windowSize;
	expectedPhaseOut = 2.*M_PI*(float)outStepSize/(float)windowSize;

	// clear all buffers
	processedBuffer = {};
	m_processedWindows = {};
	lastPhase = {};
	sumPhase = {};

	// resize all buffers
	processedBuffer.resize(scaleRatio*originalBuffer.size(), 0);
	lastPhase.resize(numWindows*windowSize, 0);
	sumPhase.resize(numWindows*windowSize, 0);
	m_processedWindows.resize(numWindows, false);

	dataLock.unlock();
}

// basic phase vocoder implementation that time shifts without pitch change
// resources:
// http://blogs.zynaptiq.com/bernsee/pitch-shifting-using-the-ft/
// https://sethares.engr.wisc.edu/vocoders/phasevocoder.html
// https://dsp.stackexchange.com/questions/40101/audio-time-stretching-without-pitch-shifting/40367#40367
// https://www.guitarpitchshifter.com/
void PhaseVocoder::generateWindow(std::vector<float> &dataIn, std::vector<float> &dataOut, int start)
{
	// declare vars
	float real, imag, phase, magnitude, freq, deltaPhase = 0;
	int windowStart = (float)start*stepSize;
	int windowIndex = (float)start*windowSize;

	memcpy(FFTInput.data(), dataIn.data() + windowStart, windowSize*sizeof(float));

	// FFT
	fftwf_execute(fftPlan);

	// analysis step
	for (int j = 0; j < windowSize; j++)
	{
		real = FFTSpectrum[j][0];
		imag = FFTSpectrum[j][1];

		magnitude = 2.*sqrt(real*real + imag*imag);
		phase = atan2(imag,real);

		freq = phase;
		freq = phase - lastPhase[std::max(0, windowIndex + j - windowSize)]; // subtract prev pahse to get phase diference
		lastPhase[windowIndex + j] = phase;

		freq -= (float)j*expectedPhaseIn; // subtract expected phase

		// some black magic to get into +/- PI interval, revise later pls
		long qpd = freq/M_PI;
		if (qpd >= 0) qpd += qpd&1;
		else qpd -= qpd&1;
		freq -= M_PI*(float)qpd;

		freq = (float)overSampling*freq/(2.*M_PI); // idk

		freq = (float)j*freqPerBin + freq*freqPerBin; // "compute the k-th partials' true frequency" ok i guess

		allMagnitudes[j] = magnitude;
		allFrequencies[j] = freq;
	}


	// pitch shifting
	// takes all the values that are below the nyquist frequency (representable with our samplerate)
	// nyquist frequency = samplerate / 2
	// and moves them to a different bin
	// improve for larger pitch shift
	// memset(processedFreq.data(), 0, processedFreq.size()*sizeof(float));
	// memset(processedMagn.data(), 0, processedFreq.size()*sizeof(float));
	// for (int j = 0; j < windowSize/2; j++) {
	// 	int index = (float)j;// * m_noteThreshold.value();
	// 	if (index <= windowSize/2) {
	// 		processedMagn[index] += allMagnitudes[j];
	// 		processedFreq[index] = allFrequencies[j];// * m_noteThreshold.value();
	// 	}
	// }

	// synthesis, all the operations are the reverse of the analysis
	for (int j = 0; j < windowSize; j++)
	{
		magnitude = allMagnitudes[j];
		freq = allFrequencies[j];

		deltaPhase = freq - (float)j*freqPerBin;

		deltaPhase /= freqPerBin;

		deltaPhase = 2.*M_PI*deltaPhase/overSampling;;

		deltaPhase += (float)j*expectedPhaseOut;

		sumPhase[windowIndex + j] += deltaPhase;
		deltaPhase = sumPhase[windowIndex + j]; // this is the bin phase
		if (windowIndex + j + windowSize < sumPhase.size()) { // only if not last window
			sumPhase[windowIndex + j + windowSize] = deltaPhase; // copy to the next
		}

		FFTSpectrum[j][0] = magnitude*cos(deltaPhase);
		FFTSpectrum[j][1] = magnitude*sin(deltaPhase);
	}

	// inverse fft
	fftwf_execute(ifftPlan);

	// windowing
	for (int j = 0; j < windowSize; j++)
	{
		float outIndex = start * outStepSize + j;
		if (outIndex >= dataOut.size()) { break; }

		// calculate windows overlapping at index
		// float startWindowOverlap = ceil(outIndex / outStepSize + 0.00000001f);
		// float endWindowOverlap = ceil((float)(-outIndex + dataOut.size()) / outStepSize + 0.00000001f);
		// float totalWindowOverlap = std::min(
		// 							std::min(startWindowOverlap, endWindowOverlap),
		// 							(float)overSampling);

		// discrete windowing
		// dataOut[outIndex] += (float)overSampling/totalWindowOverlap*IFFTReconstruction[j]/(windowSize/2.0f*overSampling);
		// printf("timeshifted in phase: %f\n", m_timeshiftedBufferL[outIndex]);
		// continuos windowing
		float window = -0.5f*cos(2.*M_PI*(float)j/(float)windowSize)+0.5f;
		dataOut[outIndex] += 2.0f*window*IFFTReconstruction[j]/(windowSize/2.0f*overSampling);
	}

}

int PhaseVocoder::hashFttWindow(std::vector<float> & in)
{
	int hash = 0;
	for (float value : in)
	{
		hash += (324723947 + (int)(value * 10689354)) ^ 93485734985;;
	}
	return hash;
}

// ################################# SlicerT ####################################

SlicerT::SlicerT(InstrumentTrack * instrumentTrack) :
	Instrument( instrumentTrack, &slicert_plugin_descriptor ),
	m_noteThreshold(0.6f, 0.0f, 2.0f, 0.01f, this, tr( "Note threshold" ) ),
	m_fadeOutFrames(0.0f, 0.0f, 8192.0f, 4.0f, this, tr("FadeOut")),
	m_originalBPM(1, 1, 999, this, tr("Original bpm")),
	m_originalSample(),
	m_phaseVocoder()
{
}



void SlicerT::playNote( NotePlayHandle * handle, sampleFrame * workingBuffer )
{
	if (m_originalSample.frames() < 2048) { return; }

	const float speedRatio = (float)m_originalBPM.value() / Engine::getSong()->getTempo() ;

	const int noteIndex = handle->key() - 69;
	const fpp_t frames = handle->framesLeftForCurrentPeriod();
	const f_cnt_t offset = handle->noteOffset();
	const int playedFrames = handle->totalFramesPlayed();
	m_phaseVocoder.setScaleRatio(speedRatio);

	const int totalFrames = m_phaseVocoder.frames();

	int sliceStart, sliceEnd;
	if (noteIndex > m_slicePoints.size()-2 || noteIndex < 0)
	{
		sliceStart = 0;
		sliceEnd = totalFrames;
	} else {
		sliceStart = m_slicePoints[noteIndex] * speedRatio;
		sliceEnd = m_slicePoints[noteIndex+1] * speedRatio;
	}

	int sliceFrames = sliceEnd - sliceStart;
	int currentNoteFrame = sliceStart + playedFrames;
	int noteFramesLeft = sliceFrames - playedFrames;


	if( noteFramesLeft > 0)
	{
		int framesToCopy = std::min((int)frames, noteFramesLeft);
		m_phaseVocoder.getFrames(workingBuffer + offset, currentNoteFrame, framesToCopy);

		// exponential fade out, applyRelease kinda sucks
		if (noteFramesLeft < m_fadeOutFrames.value())
		{
			for (int i = 0;i<frames;i++)
			{
				float fadeValue = (float)(noteFramesLeft-i) / m_fadeOutFrames.value();
				// if the workingbuffer extends the sample
				fadeValue = std::clamp(fadeValue, 0.0f, 1.0f);
				fadeValue = pow(fadeValue, 2);

				workingBuffer[i][0] *= fadeValue;
				workingBuffer[i][1] *= fadeValue;
			}
		}

		instrumentTrack()->processAudioBuffer( workingBuffer, frames + offset, handle );

		// calculate absolute for the waveform
		float absoluteCurrentNote = (float)currentNoteFrame / totalFrames;
		float absoluteStartNote = (float)sliceStart / totalFrames;
		float abslouteEndNote = (float)sliceEnd / totalFrames;
		emit isPlaying(absoluteCurrentNote, absoluteStartNote, abslouteEndNote);
	} else {
		emit isPlaying(-1, 0, 0);
	}
}


void SlicerT::findSlices()
{
	if (m_originalSample.frames() < 2048) { return; }
	m_slicePoints = {};

	const int window = 1024;
	int minWindowsPassed = 1;
	int peakIndex = 0;

	float lastPeak = 0;
	float currentPeak = 0;

	for (int i = 0; i<m_originalSample.frames();i+=1)
	{
		float sampleValue = abs(m_originalSample.data()[i][0]) + abs(m_originalSample.data()[i][1]) / 2;

		if (sampleValue > currentPeak)
		{
			currentPeak = sampleValue;
			peakIndex = i;
		}

		if (i%window==0)
		{
			if (abs(currentPeak / lastPeak) > 1+m_noteThreshold.value() && minWindowsPassed <= 0)
			{
				m_slicePoints.push_back(std::max(0, peakIndex-window/2)); // slight offset
				minWindowsPassed = 2; // wait at least one window for a new note
			}
			lastPeak = currentPeak;
			currentPeak = 0;
			minWindowsPassed--;
		}
	}
	m_slicePoints.push_back(m_originalSample.frames());

	emit dataChanged();
}

// find the bpm of the sample by assuming its in 4/4 time signature ,
// and lies in the 100 - 200 bpm range
void SlicerT::findBPM()
{
	if (m_originalSample.frames() < 2048) { return; }
	int bpmSnap = 1; // 1 = disabled

	float sampleRate = m_originalSample.sampleRate();
	float totalFrames = m_originalSample.frames();
	float sampleLength = totalFrames / sampleRate;

	// this assumes the sample has a time signature of x/4
	float bpmEstimate = 240.0f / sampleLength;

	// deal with samlpes that are not 1 bar long
	while (bpmEstimate < 100)
	{
		bpmEstimate *= 2;
	}

	while (bpmEstimate > 200)
	{
		bpmEstimate /= 2;
	}

	// snap bpm
	int bpm = bpmEstimate;
	bpm += (float)bpmSnap / 2;
	bpm -= bpm % bpmSnap;

	m_originalBPM.setValue(bpm);
	m_originalBPM.setInitValue(bpm);
}

void SlicerT::writeToMidi(std::vector<Note> * outClip)
{
	if (m_originalSample.frames() < 2048) { return; }

	int ticksPerBar = DefaultTicksPerBar;
	float sampleRate = m_originalSample.sampleRate();

	float bpm = Engine::getSong()->getTempo();
	float samplesPerBeat = 60.0f / bpm * sampleRate;
	float beats = (float)m_phaseVocoder.frames() / samplesPerBeat;

	float barsInSample = beats / Engine::getSong()->getTimeSigModel().getDenominator();
	float totalTicks = ticksPerBar * barsInSample;

	for (int i = 0;i<m_slicePoints.size()-1;i++)
	{
		float sliceStart = (float)m_slicePoints[i] / m_originalSample.frames() * totalTicks;
		float sliceEnd = (float)m_slicePoints[i + 1] / m_originalSample.frames() * totalTicks;

		Note sliceNote = Note();
		sliceNote.setKey(i + 69);
		sliceNote.setPos(sliceStart);
		sliceNote.setLength(sliceEnd - sliceStart);
		outClip->push_back(sliceNote);
	}
}

void SlicerT::updateFile(QString file)
{
	m_originalSample.setAudioFile(file);
	if (m_originalSample.frames() < 2048) { return; }

	findSlices();
	findBPM();

	float speedRatio = (float)m_originalBPM.value() / Engine::getSong()->getTempo() ;
	m_phaseVocoder.loadSample(m_originalSample.data(),
							m_originalSample.frames(),
							m_originalSample.sampleRate(),
							speedRatio);

	emit dataChanged();
}

void SlicerT::updateSlices()
{
	findSlices();
}

void SlicerT::saveSettings(QDomDocument & document, QDomElement & element)
{
	element.setAttribute("src", m_originalSample.audioFile());
	if (m_originalSample.audioFile().isEmpty())
	{
		QString s;
		element.setAttribute("sampledata", m_originalSample.toBase64(s));
	}

	element.setAttribute("totalSlices", (int)m_slicePoints.size());

	for (int i = 0;i<m_slicePoints.size();i++)
	{
		element.setAttribute(tr("slice_%1").arg(i), m_slicePoints[i]);
	}

	m_fadeOutFrames.saveSettings(document, element, "fadeOut");
	m_noteThreshold.saveSettings(document, element, "threshold");
	m_originalBPM.saveSettings(document, element, "origBPM");
}

void SlicerT::loadSettings( const QDomElement & element )
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
		for (int i = 0;i<totalSlices;i++)
		{
			m_slicePoints.push_back(element.attribute(tr("slice_%1").arg(i)).toInt());
		}
	}

	m_fadeOutFrames.loadSettings(element, "fadeOut");
	m_noteThreshold.loadSettings(element, "threshold");
	m_originalBPM.loadSettings(element, "origBPM");

	float speedRatio = (float)m_originalBPM.value() / Engine::getSong()->getTempo() ;
	m_phaseVocoder.loadSample(m_originalSample.data(),
							m_originalSample.frames(),
							m_originalSample.sampleRate(),
							speedRatio);

	emit dataChanged();

}

QString SlicerT::nodeName() const
{
	return( slicert_plugin_descriptor.name );
}

gui::PluginView * SlicerT::instantiateView( QWidget * parent )
{
	return( new gui::SlicerTUI( this, parent ) );
}


extern "C"
{
// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model *m, void * )
{
	return( new SlicerT( static_cast<InstrumentTrack *>( m ) ) );
}
} // extern
} // namespace lmms

