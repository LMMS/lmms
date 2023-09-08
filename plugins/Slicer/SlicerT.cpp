/*
 * Slicer.cpp - instrument which uses a usereditable wavetable
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

/*
!! always work on the original sample for slicing and whatever,
only use the sped up sample and slices in playNote(). This will keep things organized



load file / slcies get updated
find slices
find bpm / if bpm gets updated
update matchingSpeed sampleBuffer
update matchingSlices vector

the matchingSpeed buffer will always get initialzed freshly using a buffer, this seems to be the only way 
give it the raw data
*/

// TODO: add, remove slices; add gui values (fade out, window sizes?); fix timeshift lag (bpm change)
// TODO: midi dragging, open file button
// TODO: fix empty sample, small sample edge cases, general stability stuff

// #include <QDomElement>
#include "SlicerT.h"
#include <stdio.h>

#include "fft_helpers.h"
#include <atomic>

// #include "AudioEngine.h"
// #include "base64.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "Song.h"
#include <fftw3.h>

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
				"A cool testPlugin" ),
	"Daniel Kauss Serna>",
	0x0100,
	Plugin::Type::Instrument,
	new PluginPixmapLoader( "logo" ),
	nullptr,
	nullptr,
} ;

}



SlicerT::SlicerT(InstrumentTrack * _instrument_track) : 
	Instrument( _instrument_track, &slicert_plugin_descriptor ),
	originalSample(),
	timeShiftedSample(),
	noteThreshold( 0.6f, 0.0f, 2.0f, 0.01f, this, tr( "Note Threshold" ) ),
	originalBPM(1, 1, 999, this, tr("original sample bpm"))
{
	// connect( &noteThreshold, SIGNAL( dataChanged() ), this, SLOT( updateSlices() ) );
	// TODO: either button to timeshift, threading or generating samples on the fly
	connect( &originalBPM, SIGNAL( dataChanged() ), this, SLOT( updateTimeShift() ) );

	printf("Correctly loaded SlicerT!\n");
}

void SlicerT::playNote( NotePlayHandle * _n, sampleFrame * _working_buffer ) {
	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const int playedFrames = _n->totalFramesPlayed();
	const f_cnt_t offset = _n->noteOffset();
	


	int totalFrames = timeShiftedSample.frames();
	int sliceStart = timeShiftedSample.startFrame();
	int sliceEnd = timeShiftedSample.endFrame();
	int sliceFrames = timeShiftedSample.endFrame() - timeShiftedSample.startFrame();
	int noteFramesLeft = sliceFrames - playedFrames;

	int fadeOutFrames = (float)sliceFrames/4;

	// init NotePlayHandle data
	if( !_n->m_pluginData )
	{
		_n->m_pluginData = new SampleBuffer::handleState( false, SRC_LINEAR );

		float speedRatio = (float)originalBPM.value() / Engine::getSong()->getTempo() ;
		int noteIndex = _n->key() - 69;
		int sliceStart, sliceEnd;
		
		// 0th element is no sound, so play full sample
		if (noteIndex > slicePoints.size()-2 || noteIndex < 0) {
			sliceStart = 0;
			sliceEnd = timeShiftedSample.frames();
		} else {
			sliceStart = slicePoints[noteIndex] * speedRatio;
			sliceEnd = slicePoints[noteIndex+1] * speedRatio;
		}


		timeShiftedSample.setAllPointFrames( sliceStart, sliceEnd, sliceStart, sliceEnd );

		printf("%i : %i -> %i\n", noteIndex, sliceStart, sliceEnd);
		printf("%i\n", _n->oldKey());

	
	}


	// if play returns true (success I guess)
	if( ! _n->isFinished() ) {
		if( timeShiftedSample.play( _working_buffer + offset,
					(SampleBuffer::handleState *)_n->m_pluginData,
					frames, 440,
					static_cast<SampleBuffer::LoopMode>( 0 ) ) )
		{

			applyRelease( _working_buffer, _n );
			instrumentTrack()->processAudioBuffer( _working_buffer,
									frames + offset, _n );

			// exponential fade out
			if (noteFramesLeft < fadeOutFrames) {
				// printf("fade out started. frames: %i, framesLeft: %i\n", frames, noteFramesLeft);
				for (int i = 0;i<frames;i++) {
					float fadeValue = (float)(noteFramesLeft-i) / fadeOutFrames;
					// if the workingbuffer extends the sample
					fadeValue = std::clamp(fadeValue, 0.0f, 1.0f);
					fadeValue = pow(fadeValue, 2);

					_working_buffer[i][0] *= fadeValue;
					_working_buffer[i][1] *= fadeValue;
				}
			}

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
	int c = 0;
	const int window = 1024;
	int peakIndex = 0;
	float lastPeak = 0;
	float currentPeak = 0;
	int minWindowsPassed = 0;
	slicePoints = {};
	for (int i = 0; i<originalSample.frames();i+=1) {
		// combine left and right and absolute it
		float sampleValue = abs(originalSample.data()[i][0]) + abs(originalSample.data()[i][1]) / 2;
		if (sampleValue > currentPeak) {
			currentPeak = sampleValue;
			peakIndex = i;
		}
		
		if (i%window==0) {
								//printf("%i -> %f : %f\n", i, currentAvg, lastAvg);
			if (abs(currentPeak / lastPeak) > 1+noteThreshold.value() && minWindowsPassed <= 0) {
				c++;
				slicePoints.push_back(std::max(0, peakIndex-window/2)); // slight offset
				minWindowsPassed = 2; // wait at least one window for a new note
			}
			lastPeak = currentPeak;
			currentPeak = 0;
			
			
			minWindowsPassed--;

		}

	}
	slicePoints.push_back(originalSample.frames());

	emit dataChanged();
}

// void SlicerT::updateParams() {

// }

// find the bpm of the sample by assuming its in 4/4 time signature ,
// and lies in the 100 - 200 bpm range
void SlicerT::findBPM() {
	int bpmSnap = 1;

	float sampleRate = originalSample.sampleRate();
	float totalFrames = originalSample.frames();
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

	originalBPM.setValue(bpm);
 }

// create thimeshifted samplebuffer and timeshifted slicePoints
void SlicerT::timeShiftSample() {
	using std::vector;
	printf("starting sample timeshifting\n");

	// original sample data
	float sampleRate = originalSample.sampleRate();
	int originalFrames = originalSample.frames();

	// target data TODO: fix this mess
	bpm_t targetBPM = Engine::getSong()->getTempo();
	float speedRatio = (float)originalBPM.value() / targetBPM ;
	int samplesPerBeat = 60.0f / targetBPM * sampleRate;
	int outFrames = speedRatio * originalFrames;

	// snap to a beat, this should be in the UI
	outFrames += (float)samplesPerBeat;
	outFrames -= outFrames%samplesPerBeat;

	// nothing to do here
	if (targetBPM == originalBPM.value()) {
		timeShiftedSample = SampleBuffer(originalSample.data(), originalSample.frames());
		printf("BPM match for sample, finished timeshift\n");
		return;
	}

	// buffers
	vector<float> rawData(originalFrames, 0);
	vector<float> outData(outFrames, 0);

	vector<sampleFrame> bufferData(outFrames, sampleFrame());

	// copy channels for processing
	for (int i = 0;i<originalFrames;i++) {
		rawData[i] = (float)originalSample.data()[i][0];
	}
	printf("startign PV\n");
	// process channels
	phaseVocoder(rawData, outData, sampleRate, 1);

	printf("starting buffer copy\n");
	// write processed channels 
	for (int i = 0;i<outFrames;i++) {
		bufferData.data()[i][0] = outData[i];
		bufferData.data()[i][1] = outData[i];
	}

	printf("creating timeshifted sample\n");
	// new sample
	timeShiftedSample = SampleBuffer(bufferData.data(), bufferData.size());


	printf("finished sample timeshifting\n");


}


// basic phase vocoder implementation that time shifts without pitch change
// resources:
// http://blogs.zynaptiq.com/bernsee/pitch-shifting-using-the-ft/
// https://sethares.engr.wisc.edu/vocoders/phasevocoder.html
// https://dsp.stackexchange.com/questions/40101/audio-time-stretching-without-pitch-shifting/40367#40367
// https://www.guitarpitchshifter.com/

// a lot of stuff needs improvement, mostly the windowing and
// the pitch shifting implemention
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
	const int stepSize = windowSize / overSampling; 
	const int outStepSize = stepSize * lengthRatio;
	const int numWindows = inFrames / stepSize;
	const int windowLatency = (overSampling-1)*stepSize;
	const float freqPerBin = sampleRate/windowSize; 
	// very important
	const float expectedPhaseIn = 2.*M_PI*(float)stepSize/(float)windowSize;
	const float expectedPhaseOut = 2.*M_PI*(float)outStepSize/(float)windowSize;


	printf("frames: %i , out frames: %i , ratio: %f\n", inFrames, outFrames, (float)outFrames / inFrames);
	printf("will drop %i\n", inFrames%(inFrames/numWindows));

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
		
		// TODO: pitch shifting
		// takes all the values that are below the nyquist frequency (representable with our samplerate)
		// nyquist frequency = samplerate / 2
		// and moves them to a different bin
		// improve for larger pitch shift
		// memset(processedFreq.data(), 0, processedFreq.size()*sizeof(float));
		// memset(processedMagn.data(), 0, processedFreq.size()*sizeof(float));
		// for (int j = 0; j < windowSize/2; j++) {
		// 	int index = (float)j;// * noteThreshold.value();
		// 	if (index <= windowSize/2) {
		// 		processedMagn[index] += allMagnitudes[j];
		// 		processedFreq[index] = allFrequencies[j];// * noteThreshold.value();
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
		// this is very bad, audible click at the beginning if we take the average,
		// but else there is a windowSized delay...
		// solution would be to take the average but blend it together better
		// TODO: make better
		for (int j = 0; j < windowSize; j++) {

			float outIndex = i * outStepSize + j;
			if (outIndex > outFrames) {
				printf("too long window size, breaking\n");
				break;
			}
			if (outIndex < windowLatency) {
				// calculate amount of windows overlapping
				float windowsOverlapping =  outIndex / windowSize * overSampling + 1;

				// since not all windows overlap, just take the average of the ones that do overlap
				outBuffer[outIndex] += overSampling/windowsOverlapping*IFFTReconstruction[j]/(windowSize/2.0f*overSampling);

				// no averaging, probably worse
				// dataOut[outIndex] = overSampling*IFFTReconstruction[j]/(windowSize/2.0f*overSampling);
			} else {
				// this computes the weight of the window on the final output
				float window = -0.5f*cos(2.*M_PI*(float)j/(float)windowSize)+0.5f;

				outBuffer[outIndex] += 2.0f*window*IFFTReconstruction[j]/(windowSize/2.0f*overSampling);
			}

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

	printf("finished pitch scaling sample\n");


}



void SlicerT::updateFile(QString file) {
	printf("updated audio file\n");
	originalSample.setAudioFile(file);
	findSlices();
	findBPM(); // this also updates timeshift because bpm change
	
}

void SlicerT::updateSlices() {
	findSlices();
}

void SlicerT::updateTimeShift() {
	timeShiftSample();
}


void SlicerT::saveSettings(QDomDocument & _doc, QDomElement & _parent) {}
void SlicerT::loadSettings( const QDomElement & _this ) {}

QString SlicerT::nodeName() const
{
	return( slicert_plugin_descriptor.name );
}

gui::PluginView * SlicerT::instantiateView( QWidget * _parent )
{
	return( new gui::SlicerTUI( this, _parent ) );
}



extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model *m, void * )
{
	
	return( new SlicerT( static_cast<InstrumentTrack *>( m ) ) );
}


}


} // namespace lmms

