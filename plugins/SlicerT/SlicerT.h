/*
 * SlicerT.h - declaration of class SlicerT
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

#ifndef SLICERT_H
#define SLICERT_H

#include "SlicerTUI.h"

#include <fftw3.h>

#include "Note.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "AutomatableModel.h"
#include "SampleBuffer.h"


namespace lmms
{

// takes one audio-channel and timeshifts it
class PhaseVocoder {
	public:
		PhaseVocoder();
		~PhaseVocoder();
		void loadData(std::vector<float> originalData, int sampleRate, float newRatio);
		void setScaleRatio(float newRatio) { updateParams(newRatio); }
		void getFrames(std::vector<float> & outData, int start, int frames);
		int frames() { return processedBuffer.size(); }
		float scaleRatio() { return m_scaleRatio; }
	private:
		QMutex dataLock;
		// original data
		std::vector<float> originalBuffer;
		int originalSampleRate = 0;

		float m_scaleRatio = -1; // to force on fisrt load

		// output data
		std::vector<float> processedBuffer;
		std::vector<bool> m_processedWindows; // marks a window processed

		// timeshift stuff
		static const int windowSize = 512;
		static const int overSampling = 32;

		// depending on scaleRatio
		int stepSize = 0;
		int numWindows = 0;
		float outStepSize = 0;
		float freqPerBin = 0;
		float expectedPhaseIn = 0;
		float expectedPhaseOut = 0;

		// buffers
		fftwf_complex FFTSpectrum[windowSize];
		std::vector<float> FFTInput;
		std::vector<float> IFFTReconstruction;
		std::vector<float> allMagnitudes;
		std::vector<float> allFrequencies;
		std::vector<float> processedFreq;
		std::vector<float> processedMagn;
		std::vector<float> lastPhase;
		std::vector<float> sumPhase;

		// cache
		std::vector<float> freqCache;
		std::vector<float> magCache;

		// fftw plans
		fftwf_plan fftPlan;
		fftwf_plan ifftPlan;

		void updateParams(float newRatio);
		void generateWindow(int windowNum, bool useCache);
};

// simple helper class that handles the different audio channels
class dinamicPlaybackBuffer {
	public:
		dinamicPlaybackBuffer() :
			leftChannel(),
			rightChannel()
		{}
		void loadSample(const sampleFrame * outData, int frames, int sampleRate, float newRatio) {
			std::vector<float> leftData(frames, 0);
			std::vector<float> rightData(frames, 0);
			for (int i = 0;i<frames;i++) {
				leftData[i] = outData[i][0];
				rightData[i] = outData[i][1];
			}
			leftChannel.loadData(leftData, sampleRate, newRatio);
			rightChannel.loadData(rightData, sampleRate, newRatio);
		}
		void getFrames(sampleFrame * outData, int startFrame, int frames) {
			std::vector<float> leftOut(frames, 0); // not a huge performance issue
			std::vector<float> rightOut(frames, 0);

			leftChannel.getFrames(leftOut, startFrame, frames);
			rightChannel.getFrames(rightOut, startFrame, frames);

			for (int i = 0;i<frames;i++) {
				outData[i][0] = leftOut[i];
				outData[i][1] = rightOut[i];
			}
		}
		int frames() { return leftChannel.frames(); }
		float scaleRatio() { return leftChannel.scaleRatio(); }
		void setScaleRatio(float newRatio) {
			leftChannel.setScaleRatio(newRatio);
			rightChannel.setScaleRatio(newRatio);
		}
	private:
		PhaseVocoder leftChannel;
		PhaseVocoder rightChannel;
};

class SlicerT : public Instrument{
	Q_OBJECT

	public:
		SlicerT(InstrumentTrack * instrumentTrack);
		~SlicerT() override = default;

		void playNote( NotePlayHandle * handle, sampleFrame * workingBuffer ) override;

		void saveSettings( QDomDocument & document, QDomElement & element ) override;
		void loadSettings( const QDomElement & element ) override;

		QString nodeName() const override;
		gui::PluginView * instantiateView( QWidget * parent ) override;

		void writeToMidi(std::vector<Note> * outClip);

	public slots:
		void updateFile(QString file);
		void updateSlices();

	signals:
		void isPlaying(float current, float start, float end);

	private:
		FloatModel m_noteThreshold;
		FloatModel m_fadeOutFrames;
		IntModel m_originalBPM;

		SampleBuffer m_originalSample;
		dinamicPlaybackBuffer m_phaseVocoder;

		std::vector<int> m_slicePoints;

		void findSlices();
		void findBPM();

		friend class gui::SlicerTUI;
		friend class gui::WaveForm;
};
} // namespace lmms
#endif // SLICERT_H
