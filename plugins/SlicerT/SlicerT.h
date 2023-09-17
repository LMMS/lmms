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

// small helper class, since SampleBuffer is inadequate (not thread safe, no dinamic startpoint)
class PlaybackBuffer {
	public:
		QMutex dataLock;
		std::vector<sampleFrame> mainBuffer;
		std::vector<float> leftBuffer;
		std::vector<float> rightBuffer;
		float sampleMax = -1;

		int frames() { return mainBuffer.size(); }; // not thread safe yet, but shouldnt be a big issue
		sampleFrame * data() {  return mainBuffer.data(); };
		void copyFrames(sampleFrame * outData, int start, int framesToCopy) {
			dataLock.lock();
			memcpy(outData, mainBuffer.data() + start, framesToCopy * sizeof(sampleFrame));
			dataLock.unlock();
		}
		void resetAndResize(int newFrames) {
			mainBuffer = {};
			mainBuffer.resize(newFrames);
			leftBuffer = {};
			leftBuffer.resize(newFrames);
			rightBuffer = {};
			rightBuffer.resize(newFrames);
		}
		void loadSample(const sampleFrame * data, int newFrames)
		{
			dataLock.lock();
			resetAndResize(newFrames);
			memcpy(mainBuffer.data(), data, newFrames * sizeof(sampleFrame));
			for (int i = 0;i<newFrames;i++) {
				leftBuffer[i] = mainBuffer[i][0];
				sampleMax = std::max(sampleMax, leftBuffer[i]);
				rightBuffer[i] = mainBuffer[i][1];
				sampleMax = std::max(sampleMax, rightBuffer[i]);
			}
			dataLock.unlock();
		};
		void setFrames(std::vector<float> & leftData, std::vector<float> & rightData)
		{
			dataLock.lock();
			int newFrames = std::min(leftData.size(), rightData.size());
			mainBuffer = {};
			mainBuffer.resize(newFrames);

			for (int i = 0;i < newFrames;i++)
			{
				mainBuffer[i][0] = leftData[i];
				mainBuffer[i][1] = rightData[i];
			}
			dataLock.unlock();
		}
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
		std::vector<float> m_originalBufferL;
		std::vector<float> m_originalBufferR;
		int originalMax; // for later rescaling

		PlaybackBuffer m_timeShiftedSample;
		std::vector<float> m_timeshiftedBufferL;
		std::vector<float> m_timeshiftedBufferR;
		std::vector<bool> m_processedFrames; // check if a frame is processed


		std::vector<int> m_slicePoints;

		float m_currentSpeedRatio = -1;
		QMutex m_timeshiftLock; // should be unecesaty since playbackBuffer is safe
		// std::unordered_map<int, std::vector<float> > m_fftWindowCache;

		void updateParams();
		void extractOriginalData();
		void findSlices();
		void findBPM();
		void timeShiftSample(int windowsToProcess);
		void phaseVocoder(std::vector<float> &in, std::vector<float> &out, int start, int steps);
		int hashFttWindow(std::vector<float> & in);

		// timeshift stuff
		static const int windowSize = 512;
		static const int overSampling = 32;

		int stepSize = 0;
		int numWindows = 0;
		float outStepSize = 0;
		float freqPerBin = 0;
		// very important
		float expectedPhaseIn = 0;
		float expectedPhaseOut = 0;

		fftwf_complex FFTSpectrum[windowSize];
		std::vector<float> FFTInput;
		std::vector<float> IFFTReconstruction;
		std::vector<float> allMagnitudes;
		std::vector<float> allFrequencies;
		std::vector<float> processedFreq;
		std::vector<float> processedMagn;
		std::vector<float> lastPhase;
		std::vector<float> sumPhase;

		fftwf_plan fftPlan;
		fftwf_plan ifftPlan;

		friend class gui::SlicerTUI;
		friend class gui::WaveForm;
};
} // namespace lmms
#endif // SLICERT_H
