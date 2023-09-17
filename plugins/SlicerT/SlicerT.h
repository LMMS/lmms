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

// main class that handles everything audio related
class PhaseVocoder {
	public:
		PhaseVocoder();
		~PhaseVocoder();
		void loadSample(const sampleFrame* originalData, int frames, int sampleRate, float newRatio);
		void setScaleRatio(float newRatio) { updateParams(newRatio); }
		void getFrames(sampleFrame * outData, int start, int frames);
		int frames() { return leftBuffer.size(); }
	private:
		QMutex dataLock;
		// original data
		std::vector<float> originalBufferL;
		std::vector<float> originalBufferR;
		int originalSampleRate = 0;

		float scaleRatio = -1; // to force on fisrt load

		// output data
		std::vector<float> leftBuffer;
		std::vector<float> rightBuffer;
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

		// fftw plans
		fftwf_plan fftPlan;
		fftwf_plan ifftPlan;

		void updateParams(float newRatio);
		void generateWindow(std::vector<float> &in, std::vector<float> &out, int start);
		int hashFttWindow(std::vector<float> & in);
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
		PhaseVocoder m_phaseVocoder;

		std::vector<int> m_slicePoints;

		void findSlices();
		void findBPM();

		friend class gui::SlicerTUI;
		friend class gui::WaveForm;
};
} // namespace lmms
#endif // SLICERT_H
