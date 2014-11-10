/*
 * vibed.h - combination of PluckedStringSynth and BitInvader
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/yahoo/com>
 * 
 * This file is part of LMMS - http://lmms.io
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
#ifndef _VIBED_H
#define _VIBED_H

#include "Instrument.h"
#include "InstrumentView.h"
#include "graph.h"
#include "knob.h"
#include "pixmap_button.h"
#include "led_checkbox.h"
#include "nine_button_selector.h"

class vibedView;
class NotePlayHandle;

class vibed : public Instrument
{
	Q_OBJECT
public:
	vibed( InstrumentTrack * _instrument_track );
	virtual ~vibed();

	virtual void playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer );
	virtual void deleteNotePluginData( NotePlayHandle * _n );


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName() const;

	virtual Flags flags() const
	{
		return IsNotBendable;
	}


	virtual PluginView * instantiateView( QWidget * _parent );


private:
	QList<FloatModel*> m_pickKnobs;
	QList<FloatModel*> m_pickupKnobs;
	QList<FloatModel*> m_stiffnessKnobs;
	QList<FloatModel*> m_volumeKnobs;
	QList<FloatModel*> m_panKnobs;
	QList<FloatModel*> m_detuneKnobs;
	QList<FloatModel*> m_randomKnobs;
	QList<FloatModel*> m_lengthKnobs;
	QList<BoolModel*> m_powerButtons;
	QList<graphModel*> m_graphs;
	QList<BoolModel*> m_impulses;
	QList<nineButtonSelectorModel*> m_harmonics;

	static const int __sampleLength = 128;

	friend class vibedView;
} ;



class vibedView : public InstrumentView
{
	Q_OBJECT
public:
	vibedView( Instrument * _instrument,
					QWidget * _parent );
	virtual ~vibedView() {};

public slots:
	void showString( int _string );
	void contextMenuEvent( QContextMenuEvent * );
	void displayHelp();

protected slots:
	void sinWaveClicked();
	void triangleWaveClicked();
	void sawWaveClicked();
	void sqrWaveClicked();
	void noiseWaveClicked();
	void usrWaveClicked();
	void smoothClicked();
	void normalizeClicked();

private:
	virtual void modelChanged();


	// String-related
	knob * m_pickKnob;
	knob * m_pickupKnob;
	knob * m_stiffnessKnob;
	knob * m_volumeKnob;
	knob * m_panKnob;
	knob * m_detuneKnob;
	knob * m_randomKnob;
	knob * m_lengthKnob;
	graph * m_graph;
	nineButtonSelector * m_harmonic;
	ledCheckBox * m_impulse;
	ledCheckBox * m_power;

	// Not in model
	nineButtonSelector * m_stringSelector;
	pixmapButton * m_smoothBtn;
	pixmapButton * m_normalizeBtn;

	// From impulse editor
	pixmapButton * m_sinWaveBtn;
	pixmapButton * m_triangleWaveBtn;
	pixmapButton * m_sqrWaveBtn;
	pixmapButton * m_sawWaveBtn;
	pixmapButton * m_whiteNoiseWaveBtn;
	pixmapButton * m_usrWaveBtn;


};

#endif
