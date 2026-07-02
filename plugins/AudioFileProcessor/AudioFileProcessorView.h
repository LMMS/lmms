/*
 * AudioFileProcessorView.h - View of the AFP
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_AUDIO_FILE_PROCESSOR_VIEW_H
#define LMMS_AUDIO_FILE_PROCESSOR_VIEW_H

#include "InstrumentView.h"


namespace lmms
{

namespace gui
{

class AutomatableButtonGroup;
class Knob;
class PixmapButton;
class ComboBox;
class AudioFileProcessorWaveView;


class AudioFileProcessorView : public gui::InstrumentViewFixedSize
{
	Q_OBJECT
public:
	AudioFileProcessorView(Instrument* instrument, QWidget* parent);
	virtual ~AudioFileProcessorView() = default;

	void newWaveView();

protected slots:
	void sampleUpdated();
	void openAudioFile();

protected:
	virtual void dragEnterEvent(QDragEnterEvent* dee);
	virtual void dropEvent(QDropEvent* de);
	virtual void paintEvent(QPaintEvent*);

	// Private methods
private:
	virtual void modelChanged();

	// Private members
private:
	AudioFileProcessorWaveView* m_waveView;
	Knob* m_ampKnob;
	Knob* m_startKnob;
	Knob* m_endKnob;
	Knob* m_loopKnob;

	gui::PixmapButton* m_openAudioFileButton;
	PixmapButton* m_reverseButton;
	AutomatableButtonGroup* m_loopGroup;
	PixmapButton* m_stutterButton;
	ComboBox* m_interpBox;
} ;

} // namespace gui

} // namespace lmms

#endif // LMMS_AUDIO_FILE_PROCESSOR_VIEW_H
