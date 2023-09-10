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
		std::vector<sampleFrame> mainBuffer;

		int frames() { return mainBuffer.size(); };
		sampleFrame * data() { return mainBuffer.data(); };

		void setData(const sampleFrame * data, int newFrames) 
		{
			mainBuffer = {};
			mainBuffer.resize(newFrames); 
			memcpy(mainBuffer.data(), data, newFrames * sizeof(sampleFrame));
		};
		void setData(std::vector<float> & leftData, std::vector<float> & rightData) {
			int newFrames = std::min(leftData.size(), rightData.size());
			mainBuffer = {};
			mainBuffer.resize(newFrames);

			for (int i = 0;i < newFrames;i++) {
				mainBuffer[i][0] = leftData[i];
				mainBuffer[i][1] = rightData[i];
			}
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
		void updateTimeShift();
		void updateSlices();

	signals:
		void isPlaying(float current, float start, float end);

	private:
		FloatModel m_noteThreshold;
		FloatModel m_fadeOutFrames;
		IntModel m_originalBPM;

		SampleBuffer m_originalSample;
		PlaybackBuffer m_timeShiftedSample;
		std::vector<int> m_slicePoints;
		
		float m_currentSpeedRatio = 0;
		bool m_timeshiftLock; // dont run timeshifting at the same time, instant crash
		// std::unordered_map<int, std::vector<float> > m_fftWindowCache;

		void findSlices();
		void findBPM();
		void timeShiftSample();
		void phaseVocoder(std::vector<float> &in, std::vector<float> &out, float sampleRate, float pitchScale);
		int hashFttWindow(std::vector<float> & in);

		friend class gui::SlicerTUI;
		friend class gui::WaveForm;
};
} // namespace lmms
#endif // SLICERT_H
