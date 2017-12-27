/*
 * PadsGDXView.h - sample player for pads
 *
 * Copyright (c) 2017 gi0e5b06
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


#ifndef PADS_GDX_VIEW_H
#define PADS_GDX_VIEW_H

#include <QObject>
#include <QPixmap>
#include <QWidget>

#include "PadsGDX.h"
#include "gui_templates.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "SampleBuffer.h"
#include "Knob.h"
#include "PixmapButton.h"
#include "AutomatableButton.h"
#include "ComboBox.h"
#include "StringPairDrag.h"


class PadsGDXWaveView;


class PadsGDXView : public InstrumentView
{
	Q_OBJECT

 public:
	PadsGDXView(Instrument* _instrument, QWidget* _parent);
	virtual ~PadsGDXView();
        virtual void doConnections();

 signals:

 public slots:
	virtual void onModelChanged();
	virtual void onSampleUpdated();
	virtual void openAudioFile();
	virtual void onKeyUpdated(int);

 protected:
	virtual void dragEnterEvent(QDragEnterEvent* _dee);
	virtual void dropEvent(QDropEvent* _de);
	virtual void paintEvent(QPaintEvent* _pe);

 private:
	void updateWaveView(bool _full);

	static QPixmap* s_artwork;

	PadsGDXWaveView*        m_waveView;
	Knob*                   m_startKnob;
	Knob*                   m_endKnob;
	Knob*                   m_loopStartKnob;
	Knob*                   m_loopEndKnob;
	//Knob*                   m_ampKnob;
	PixmapButton*           m_openAudioFileButton;
	PixmapButton*           m_reverseButton;
	automatableButtonGroup* m_loopGroup;
	PixmapButton*           m_stutterButton;
	//ComboBox*               m_interpBox;
} ;


#endif
