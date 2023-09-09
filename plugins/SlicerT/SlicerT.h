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
		SampleBuffer m_timeShiftedSample;
		std::vector<int> m_slicePoints;
		
		void findSlices();
		void findBPM();
		void timeShiftSample();
		void phaseVocoder(std::vector<float> &in, std::vector<float> &out, float sampleRate, float pitchScale);

		friend class gui::SlicerTUI;
		friend class gui::WaveForm;
};
}

#endif
