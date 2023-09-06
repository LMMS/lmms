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

// #include <QDomElement>
#include <stdio.h>
#include "SlicerT.h"
#include "fft_helpers.h"
#include <atomic>

// #include "AudioEngine.h"
// #include "base64.h"
#include "Engine.h"
// #include "Graph.h"
#include "InstrumentTrack.h"
// #include "Knob.h"
// #include "LedCheckBox.h"
// #include "NotePlayHandle.h"
// #include "PixmapButton.h"
#include "Song.h"
// #include "interpolation.h"
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
	noteThreshold( 0.6f, 0.0f, 2.0f, 0.01f, this, tr( "Note Threshold" ) )
{
	// connect( &noteThreshold, SIGNAL( dataChanged() ), this, SLOT( updateParams() ) );
	printf("Correctly loaded SlicerT!\n");
	warmupFFT();
}


void SlicerT::findSlices() {
	int c = 0;
	const int window = 1024;
	int peakIndex = 0;
	float lastPeak = 0;
	float currentPeak = 0;
	int minWindowsPassed = 0;
	slicePoints = {0};
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
				slicePoints.push_back(peakIndex);
				minWindowsPassed = 2; // wait at least one window for a new note
			}
			lastPeak = currentPeak;
			currentPeak = 0;
			
			
			minWindowsPassed--;

		}

	}
	slicePoints.push_back(originalSample.frames());
	// for (int i : slicePoints) {
	// 	printf("%i\n", i);
	// }
	
	// printf("Found %i notes\n", c);
	emit dataChanged();
}

// void SlicerT::updateParams() {

// }

void SlicerT::playNote( NotePlayHandle * _n, sampleFrame * _working_buffer ) {
	
	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

	// 0 at 440hz, or where the square thing is on the keyboard
	

	// init NotePlayHandle data
	if( !_n->m_pluginData )
	{
		_n->m_pluginData = new SampleBuffer::handleState( false, SRC_LINEAR );

		// 0th element is no sound, so play no sound when out of bounds
		// int noteIndex = _n->key() - 68;
		// if (noteIndex > slicePoints.size()-2 || noteIndex < 0) {
		// 	noteIndex = 0;
		// }

		// int sliceStart = slicePoints[noteIndex];
		// int sliceEnd = slicePoints[noteIndex+1];

		// m_sampleBuffer.setAllPointFrames( sliceStart, sliceEnd, sliceStart, sliceEnd );

		// printf("%i : %i -> %i\n", noteIndex, sliceStart, sliceEnd);
		// printf("%i\n", _n->oldKey());

	
	}

	// if play returns true (success I guess)
	if( timeShiftedSample.play( _working_buffer + offset,
				(SampleBuffer::handleState *)_n->m_pluginData,
				frames, 440,
				static_cast<SampleBuffer::LoopMode>( 0 ) ) )
	{

		applyRelease( _working_buffer, _n );
		instrumentTrack()->processAudioBuffer( _working_buffer,
								frames + offset, _n );

	}


}

// create thimeshifted samplebuffer and timeshifted slicePoints
// http://blogs.zynaptiq.com/bernsee/pitch-shifting-using-the-ft/
void SlicerT::timeShiftSample() {

	printf("starting sample timeshifting\n");

	using std::vector;




	bpm_t targetBPM = Engine::getSong()->getTempo();;
	// nothing to do here
	if (targetBPM == originalBPM) {
		timeShiftedSample = SampleBuffer(originalSample.data(), originalSample.frames());
		printf("BPM match for sample, finished timeshift\n");
		return;
	}



	float sampleRate = originalSample.sampleRate();
	float speedRatio = (double)targetBPM / originalBPM;
	int originalFrames = originalSample.frames();
	int targetFrames = originalFrames * speedRatio;
	


	vector<float> rawData(originalFrames, 0);
	vector<float> outData(originalFrames, 0);
	vector<sampleFrame> bufferData(originalFrames, sampleFrame());

	for (int i = 0;i<originalFrames;i++) {
		rawData[i] = (float)originalSample.data()[i][0];
	}

	phaseVocoder(rawData, outData, sampleRate, 1);

	for (int i = 0;i<originalFrames;i++) {
		bufferData.data()[i][0] = outData[i];
		bufferData.data()[i][1] = outData[i];
	}

	timeShiftedSample = SampleBuffer(bufferData.data(), originalFrames);

	printf("finished sample timeshifting\n");

}


void SlicerT::phaseVocoder(std::vector<float> &dataIn, std::vector<float> &dataOut, float sampleRate, float pitchScale) {
	using std::vector;
	// processing parameters, lower is faster
	const int windowSize = 2048;
	const int overSampling = 4;
	
	// values used
	const int stepSize = windowSize / overSampling; 
	const int windowLatency = windowSize - stepSize;
	const float freqPerBin = sampleRate/windowSize; 
	const float expectedPhase = 2.*M_PI*(float)stepSize/(float)windowSize;
	

	// audio data
	int inFrames = dataIn.size();
	int outFrames = dataOut.size();

	// initialize buffers
	fftwf_complex FFTSpectrum[windowSize];
	vector<float> IFFTReconstruction(windowSize, 0);
	vector<float> allMagnitudes(windowSize, 0);
	vector<float> allFrequencies(windowSize, 0);
	vector<float> lastPhase(windowSize, 0);
	vector<float> sumPhase(windowSize, 0);

	// declare vars
	float real, imag, phase, prevPhase, magnitude, freq, deltaPhase = 0;

	// fft plans
	fftwf_plan fftPlan;
	fftwf_plan ifftPlan;



	for (int i = 0;i < inFrames - windowSize;i+=stepSize) {
		// FFT
		fftPlan = fftwf_plan_dft_r2c_1d(windowSize, dataIn.data() + i, FFTSpectrum, FFTW_MEASURE);
		fftwf_execute(fftPlan);

		// analysis step
		for (int j = 0; j < windowSize; j++) {
			
			real = FFTSpectrum[j][0];
			imag = FFTSpectrum[j][1];

			// printf("freq: %3d %+9.5f %+9.5f I  original: %+9.5f \n", j, real, imag, dataIn.at(i+j));

			magnitude = 2.*sqrt(real*real + imag*imag);
			phase = atan2(imag,real); 

			freq = phase - lastPhase[j]; // subtract prev pahse to get phase diference
			lastPhase[j] = phase;

			freq -= (float)j*expectedPhase; // subtract expected phase

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
		// effects go here


		// synthesis, all the operations are the reverse of the analysis
		for (int j = 0; j < windowSize; j++) {
			magnitude = allMagnitudes[j];
			freq = allFrequencies[j];

			deltaPhase = freq - (float)j*freqPerBin;

			deltaPhase /= freqPerBin;

			deltaPhase = 2.*M_PI*deltaPhase/overSampling;;

			deltaPhase += (float)j*expectedPhase;

			sumPhase[j] += deltaPhase;
			deltaPhase = sumPhase[j]; // this is the bin phase

			FFTSpectrum[j][0] = magnitude*cos(deltaPhase);
			FFTSpectrum[j][1] = magnitude*sin(deltaPhase);
		}

		// inverse fft
		ifftPlan = fftwf_plan_dft_c2r_1d(windowSize, FFTSpectrum, IFFTReconstruction.data(), FFTW_MEASURE);
		fftwf_execute(ifftPlan);

		// windowing
		for (int j = 0; j < windowSize; j++) {
			
			if (i+j < windowLatency) {
				// calculate amount of windows overlapping
				float windowIndex =  i + j;
				int windowsOverlapping = windowIndex / windowSize * overSampling + 1;

				
				// since not all windows overlap, just take the average of the ones that do overlap
				// this should be improved
				// idk why the 4 is there but it works
				dataOut[j+i] += 4/windowsOverlapping*IFFTReconstruction[j]/(windowSize/2.0f*overSampling);
			} else {
				// this computes the weight of the window on the final output
				float window = -0.5f*cos(2.*M_PI*(float)j/(float)windowSize)+0.5f;
				dataOut[j+i] += 2.0f*window*IFFTReconstruction[j]/(windowSize/2.0f*overSampling);
			}


		}


	}

	// normalize 	
	float max = -1;
	for (int i = 0;i<inFrames;i++) {
		max = std::max(max, abs(dataOut[i]));
		dataOut[i] = dataOut[i];
	}

	for (int i = 0;i<outFrames;i++) {
		dataOut[i] = dataOut[i] / max;
	}

	fftwf_destroy_plan(fftPlan);
	fftwf_destroy_plan(ifftPlan);


	// fftwf_cleanup();

}

void SlicerT::normalizeSample(sampleFrame * data) {
	float max = -1;
	for (int i = 0;i<data->size();i++) {
		max = std::max(max, abs(data[i][0]));
		max = std::max(max, abs(data[i][1]));
	}

	printf("max: %f\n", max);

	for (int i = 0;i<data->size();i++) {
		data[i][0] = 0;
		data[i][1] = 0;
	}

}

void SlicerT::warmupFFT() {
	const int dataPoints = 512;

	std::vector<float> warmupData(dataPoints, sqrt(2));
	fftwf_complex fftOut[dataPoints];
	fftwf_plan p = fftwf_plan_dft_r2c_1d(dataPoints, warmupData.data(), fftOut, FFTW_MEASURE);
	fftwf_execute(p);

	fftwf_plan d = fftwf_plan_dft_c2r_1d(dataPoints, fftOut, warmupData.data(), FFTW_MEASURE);
	fftwf_execute(d);

	fftwf_destroy_plan(p);
	fftwf_destroy_plan(d);
}

void SlicerT::updateFile(QString file) {
	printf("updated audio file\n");
	originalSample.setAudioFile(file);
	findSlices();
	// updateBPM()
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



/* Implementation of Robust peak detection algorithm
Doesnt work that great, it is too sensitive 
https://stackoverflow.com/questions/22583391/peak-signal-detection-in-realtime-timeseries-data/46998001#46998001
void SlicerT::findSlices() {
	int c = 0;
	const int lag = 64;
	const float influence = 1;
	
	float peak = 0;
	float lastMean = 0;
	float lastStd = 0;
	int noteDuration = 0;
	// init vector with the start of the sample
	std::vector<float> lastValues = {};


	slicePoints = {};
	for (int i = 0; i<m_sampleBuffer.frames();i++) {
		// combine left and right and absolute it
		float sampleValue = abs(m_sampleBuffer.data()[i][0]) + abs(m_sampleBuffer.data()[i][1]) / 2;
		peak = std::max(sampleValue, peak);

		if (i%64==0) {

			if (lastValues.size() < lag) {
				lastValues.push_back(peak);
				continue;
			} 

			if (abs(peak - lastMean) > noteThreshold.value()) {
				if (noteDuration <= 0) {
					printf("%f : %f : %f\n", peak, lastMean, lastStd);
					// lastValues.push_back(influence*peak + (1-influence)* lastValues.back());
					lastValues.push_back(peak);
					lastValues.erase(lastValues.begin());
					c++;
					slicePoints.push_back(i);
					noteDuration = 20;
				}

			} else {

				lastValues.push_back(peak);
				lastValues.erase(lastValues.begin());
				
			}
			noteDuration--;

			lastMean = 0;
			for (float v : lastValues) {
				lastMean += v;
			}
			lastMean /= lag;

			lastStd = 0;
			for (float v : lastValues) {
				lastStd += pow(v - lastMean, 2);
			}

			lastStd = sqrt(lastStd / lag);
			peak = 0;
		}




	}
	// for (int i : slicePoints) {
	// 	printf("%i\n", i);
	// }
	
	printf("Found %i notes\n", c);
	emit dataChanged();
}
*/