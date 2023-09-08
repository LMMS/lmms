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

#include "SlicerT.h"
#include <stdio.h>
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
	LMMS_STRINGIFY( PLUGINhandleAME ),
	"SlicerT",
	QT_TRANSLATE_NOOP( "PluginBrowser",
				"Basic Slicer" ),
	"Daniel Kauss Serna",
	0x0100,
	Plugin::Type::Instrument,
	new PluginPixmapLoader( "logo" ),
	nullptr,
	nullptr,
} ;
} // end extern

SlicerT::SlicerT(InstrumentTrack * instrumentTrack) : 
	Instrument( instrumentTrack, &slicert_plugin_descriptor ),
	m_noteThreshold(0.6f, 0.0f, 2.0f, 0.01f, this, tr( "Note threshold" ) ),
	m_fadeOutFrames(0.0f, 0.0f, 8192.0f, 4.0f, this, tr("FadeOut")),
	m_originalBPM(1, 1, 999, this, tr("Original bpm")),

	m_originalSample(),
	m_timeShiftedSample()
{}


void SlicerT::playNote( NotePlayHandle * handle, sampleFrame * workingBuffer ) {
	if (m_originalSample.frames() < 2048) {
		return;
	}

	const fpp_t frames = handle->framesLeftForCurrentPeriod();
	const int playedFrames = handle->totalFramesPlayed();
	const f_cnt_t offset = handle->noteOffset();
	
	int totalFrames = m_timeShiftedSample.frames();
	int sliceStart = m_timeShiftedSample.startFrame();
	int sliceEnd = m_timeShiftedSample.endFrame();
	int sliceFrames = m_timeShiftedSample.endFrame() - m_timeShiftedSample.startFrame();
	int noteFramesLeft = sliceFrames - playedFrames;

	// init NotePlayHandle data
	if( !handle->m_pluginData )
	{
		handle->m_pluginData = new SampleBuffer::handleState( false, SRC_LINEAR );

		float speedRatio = (float)m_originalBPM.value() / Engine::getSong()->getTempo() ;
		int noteIndex = handle->key() - 69;
		int sliceStart, sliceEnd;
		
		// 0th element is no sound, so play full sample
		if (noteIndex > m_slicePoints.size()-2 || noteIndex < 0) {
			sliceStart = 0;
			sliceEnd = m_timeShiftedSample.frames();
		} else {
			sliceStart = m_slicePoints[noteIndex] * speedRatio;
			sliceEnd = m_slicePoints[noteIndex+1] * speedRatio;
		}

		m_timeShiftedSample.setAllPointFrames( sliceStart, sliceEnd, sliceStart, sliceEnd );
	}

	if( ! handle->isFinished() ) {
		if( m_timeShiftedSample.play( workingBuffer + offset,
					(SampleBuffer::handleState *)handle->m_pluginData,
					frames, 440,
					static_cast<SampleBuffer::LoopMode>( 0 ) ) )
		{
			// exponential fade out, applyRelease kinda sucks
			if (noteFramesLeft < m_fadeOutFrames.value()) {
				for (int i = 0;i<frames;i++) {
					float fadeValue = (float)(noteFramesLeft-i) / m_fadeOutFrames.value();
					// if the workingbuffer extends the sample
					fadeValue = std::clamp(fadeValue, 0.0f, 1.0f);
					fadeValue = pow(fadeValue, 2);

					workingBuffer[i][0] *= fadeValue;
					workingBuffer[i][1] *= fadeValue;
				}
			}

			instrumentTrack()->processAudioBuffer( workingBuffer,
									frames + offset, handle );


			float absoluteCurrentNote = (float)(sliceStart + playedFrames) / totalFrames;
			float absoluteStartNote = (float)sliceStart / totalFrames;
			float abslouteEndNote = (float)sliceEnd / totalFrames;
			emit isPlaying(absoluteCurrentNote, absoluteStartNote, abslouteEndNote);

		} else {
			emit isPlaying(0, 0, 0);			
		}
	} else {
		emit isPlaying(0, 0, 0);
	}
}


void SlicerT::findSlices() {
	if (m_originalSample.frames() < 2048) {
		return;
	}
	m_slicePoints = {};

	const int window = 1024;
	int minWindowsPassed = 0;
	int peakIndex = 0;

	float lastPeak = 0;
	float currentPeak = 0;
	
	for (int i = 0; i<m_originalSample.frames();i+=1) {
		float sampleValue = abs(m_originalSample.data()[i][0]) + abs(m_originalSample.data()[i][1]) / 2;

		if (sampleValue > currentPeak) {
			currentPeak = sampleValue;
			peakIndex = i;
		}
		
		if (i%window==0) {
			if (abs(currentPeak / lastPeak) > 1+m_noteThreshold.value() && minWindowsPassed <= 0) {
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
void SlicerT::findBPM() {
	if (m_originalSample.frames() < 2048) {
		return;
	}
	int bpmSnap = 1;

	float sampleRate = m_originalSample.sampleRate();
	float totalFrames = m_originalSample.frames();
	float sampleLength = totalFrames / sampleRate;

	// this assumes the sample has a time signature of x/4
	float bpmEstimate = 240.0f / sampleLength;

	// deal with samlpes that are not 1 bar long
	while (bpmEstimate < 100) {
		bpmEstimate *= 2;
	}

	while (bpmEstimate > 200) {
		bpmEstimate /= 2;
	}

	// snap bpm
	int bpm = bpmEstimate;
	bpm += (float)bpmSnap / 2;
	bpm -= bpm % bpmSnap;

	m_originalBPM.setValue(bpm);
	m_originalBPM.setInitValue(bpm);
}

// create timeshifted samplebuffer and timeshifted m_slicePoints
void SlicerT::timeShiftSample() {
	if (m_originalSample.frames() < 2048) {
		return;
	}
	using std::vector;

	// original sample data
	float sampleRate = m_originalSample.sampleRate();
	int originalFrames = m_originalSample.frames();

	// target data TODO: fix this mess
	bpm_t targetBPM = Engine::getSong()->getTempo();
	float speedRatio = (float)m_originalBPM.value() / targetBPM ;
	int outFrames = speedRatio * originalFrames;

	// nothing to do here
	if (targetBPM == m_originalBPM.value()) {
		m_timeShiftedSample = SampleBuffer(m_originalSample.data(), m_originalSample.frames());
		return;
	}

	// buffers
	vector<float> rawDataL(originalFrames, 0);
	vector<float> rawDataR(originalFrames, 0);
	vector<float> outDataL(outFrames, 0);
	vector<float> outDataR(outFrames, 0);

	vector<sampleFrame> bufferData(outFrames, sampleFrame());

	// copy channels for processing
	for (int i = 0;i<originalFrames;i++) {
		rawDataL[i] = (float)m_originalSample.data()[i][0];
		rawDataR[i] = (float)m_originalSample.data()[i][1];
	}
	// process channels
	phaseVocoder(rawDataL, outDataL, sampleRate, 1);
	phaseVocoder(rawDataR, outDataR, sampleRate, 1);

	// write processed channels 
	for (int i = 0;i<outFrames;i++) {
		bufferData.data()[i][0] = outDataL[i];
		bufferData.data()[i][1] = outDataR[i];
	}

	// new sample
	m_timeShiftedSample = SampleBuffer(bufferData.data(), bufferData.size());
}

// basic phase vocoder implementation that time shifts without pitch change
// resources:
// http://blogs.zynaptiq.com/bernsee/pitch-shifting-using-the-ft/
// https://sethares.engr.wisc.edu/vocoders/phasevocoder.html
// https://dsp.stackexchange.com/questions/40101/audio-time-stretching-without-pitch-shifting/40367#40367
// https://www.guitarpitchshifter.com/
void SlicerT::phaseVocoder(std::vector<float> &dataIn, std::vector<float> &dataOut, float sampleRate, float pitchScale) {
	using std::vector;
	// processing parameters, lower is faster
	// lower windows size seems to work better for time scaling,
	// this is because the step site is scaled, but not the window size
	// this causes slight timing differences between windows
	// sadly, lower windowsize also reduces audio quality in general
	// TODO: find solution
	// oversampling is better if higher always (probably)
	const int windowSize = 512;
	const int overSampling = 32;
	
	// audio data
	int inFrames = dataIn.size();
	int outFrames = dataOut.size();

	float lengthRatio = (float)outFrames / inFrames;

	// values used
	const int stepSize = (float)windowSize / overSampling; 
	const int numWindows = (float)inFrames / stepSize;
	const float outStepSize = lengthRatio * (float)stepSize; // float, else inaccurate
	const float freqPerBin = sampleRate/windowSize; 
	// very important
	const float expectedPhaseIn = 2.*M_PI*(float)stepSize/(float)windowSize;
	const float expectedPhaseOut = 2.*M_PI*(float)outStepSize/(float)windowSize;

	// initialize buffers
	fftwf_complex FFTSpectrum[windowSize];
	vector<float> FFTInput(windowSize, 0);
	vector<float> IFFTReconstruction(windowSize, 0);
	vector<float> allMagnitudes(windowSize, 0);
	vector<float> allFrequencies(windowSize, 0);
	vector<float> processedFreq(windowSize, 0);
	vector<float> processedMagn(windowSize, 0);
	vector<float> lastPhase(windowSize, 0);
	vector<float> sumPhase(windowSize, 0);

	vector<float> outBuffer(outFrames, 0);

	// declare vars
	float real, imag, phase, magnitude, freq, deltaPhase = 0;
	int windowIndex = 0;

	// fft plans
	fftwf_plan fftPlan;
	fftPlan = fftwf_plan_dft_r2c_1d(windowSize, FFTInput.data(), FFTSpectrum, FFTW_MEASURE);
	fftwf_plan ifftPlan;
	ifftPlan = fftwf_plan_dft_c2r_1d(windowSize, FFTSpectrum, IFFTReconstruction.data(), FFTW_MEASURE);

	// remove oversampling, because the actual window is overSampling* bigger than stepsize
	for (int i = 0;i < numWindows-overSampling;i++) {
		windowIndex = i * stepSize;

		// FFT
		memcpy(FFTInput.data(), dataIn.data() + windowIndex, windowSize*sizeof(float));
		fftwf_execute(fftPlan);

		// analysis step
		for (int j = 0; j < windowSize; j++) {
			
			real = FFTSpectrum[j][0];
			imag = FFTSpectrum[j][1];

			magnitude = 2.*sqrt(real*real + imag*imag);
			phase = atan2(imag,real); 

			freq = phase - lastPhase[j]; // subtract prev pahse to get phase diference
			lastPhase[j] = phase;

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
		for (int j = 0; j < windowSize; j++) {
			magnitude = allMagnitudes[j];
			freq = allFrequencies[j];

			deltaPhase = freq - (float)j*freqPerBin;

			deltaPhase /= freqPerBin;

			deltaPhase = 2.*M_PI*deltaPhase/overSampling;;

			deltaPhase += (float)j*expectedPhaseOut;

			sumPhase[j] += deltaPhase;
			deltaPhase = sumPhase[j]; // this is the bin phase

			FFTSpectrum[j][0] = magnitude*cos(deltaPhase);
			FFTSpectrum[j][1] = magnitude*sin(deltaPhase);
		}

		// inverse fft
		fftwf_execute(ifftPlan);

		// windowing
		for (int j = 0; j < windowSize; j++) {

			float outIndex = i * outStepSize + j;
			if (outIndex > outFrames) {
				break;
			}

			// calculate windows overlapping at index
			float startWindowOverlap = ceil(outIndex / outStepSize + 0.00000001f);
			float endWindowOverlap = ceil((float)(-outIndex + outFrames) / outStepSize + 0.00000001f);
			float totalWindowOverlap = std::min(
										std::min(startWindowOverlap, endWindowOverlap), 
										(float)overSampling);

			// discrete windowing
			outBuffer[outIndex] += (float)overSampling/totalWindowOverlap*IFFTReconstruction[j]/(windowSize/2.0f*overSampling);

			// continuos windowing
			// float window = -0.5f*cos(2.*M_PI*(float)j/(float)windowSize)+0.5f;
			// outBuffer[outIndex] += 2.0f*window*IFFTReconstruction[j]/(windowSize/2.0f*overSampling);
		}
	}

	fftwf_destroy_plan(fftPlan);
	fftwf_destroy_plan(ifftPlan);

	// normalize 	
	float max = -1;
	for (int i = 0;i<outFrames;i++) {
		max = std::max(max, abs(outBuffer[i]));
	}

	for (int i = 0;i<outFrames;i++) {
		outBuffer[i] = outBuffer[i] / max;
	}

	memcpy(dataOut.data(), outBuffer.data(), outFrames*sizeof(float));
}

void SlicerT::writeToMidi(std::vector<Note> * outClip) {
	if (m_originalSample.frames() < 2048) {
		return;
	}
	int ticksPerBar = DefaultTicksPerBar;
	float sampleRate = m_timeShiftedSample.sampleRate();

	float bpm = Engine::getSong()->getTempo();
	float samplesPerBeat = 60.0f / bpm * sampleRate;
	float beats = (float)m_timeShiftedSample.frames() / samplesPerBeat;

	float barsInSample = beats / Engine::getSong()->getTimeSigModel().getDenominator();
	float totalTicks = ticksPerBar * barsInSample;

	for (int i = 0;i<m_slicePoints.size()-1;i++) {
		float sliceStart = (float)m_slicePoints[i] / m_originalSample.frames() * totalTicks;
		float sliceEnd = (float)m_slicePoints[i + 1] / m_originalSample.frames() * totalTicks;

		Note sliceNote = Note();
		sliceNote.setKey(i + 69);
		sliceNote.setPos(sliceStart);
		sliceNote.setLength(sliceEnd - sliceStart);
		outClip->push_back(sliceNote);
	}
}

void SlicerT::updateFile(QString file) {
	m_originalSample.setAudioFile(file);
	if (m_originalSample.frames() < 2048) {
		return;
	}
	findSlices();
	findBPM(); 
	timeShiftSample();
	
	emit dataChanged();
}

void SlicerT::updateSlices() {
	findSlices();
}

void SlicerT::updateTimeShift() {
	timeShiftSample();
}


void SlicerT::saveSettings(QDomDocument & document, QDomElement & element) {
	element.setAttribute("src", m_originalSample.audioFile());
	if (m_originalSample.audioFile().isEmpty())
	{
		QString s;
		element.setAttribute("sampledata", m_originalSample.toBase64(s));
	}

	element.setAttribute("totalSlices", (int)m_slicePoints.size());

	for (int i = 0;i<m_slicePoints.size();i++) {
		element.setAttribute(tr("slice_%1").arg(i), m_slicePoints[i]);
	}

	m_fadeOutFrames.saveSettings(document, element, "fadeOut");
	m_noteThreshold.saveSettings(document, element, "threshold");
	m_originalBPM.saveSettings(document, element, "origBPM");

}
void SlicerT::loadSettings( const QDomElement & element ) {
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

	if (!element.attribute("totalSlices").isEmpty()) {
		int totalSlices = element.attribute("totalSlices").toInt();
		m_slicePoints = {};
		for (int i = 0;i<totalSlices;i++) {
			
			m_slicePoints.push_back(element.attribute(tr("slice_%1").arg(i)).toInt());
		}
	}

	m_fadeOutFrames.loadSettings(element, "fadeOut");
	m_noteThreshold.loadSettings(element, "threshold");
	m_originalBPM.loadSettings(element, "origBPM");

	timeShiftSample();

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

