/*
 * vibed.h - combination of PluckedStringSynth and BitInvader
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/yahoo/com>
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
#ifndef _VIBED_H
#define _VIBED_H

#include "Instrument.h"
#include "InstrumentView.h"
#include "Graph.h"
#include "Knob.h"
#include "PixmapButton.h"
#include "LedCheckbox.h"
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



class vibedView : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	vibedView( Instrument * _instrument,
					QWidget * _parent );
	virtual ~vibedView() {};

public slots:
	void showString( int _string );
	void contextMenuEvent( QContextMenuEvent * );

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
	Knob * m_pickKnob;
	Knob * m_pickupKnob;
	Knob * m_stiffnessKnob;
	Knob * m_volumeKnob;
	Knob * m_panKnob;
	Knob * m_detuneKnob;
	Knob * m_randomKnob;
	Knob * m_lengthKnob;
	Graph * m_graph;
	nineButtonSelector * m_harmonic;
	LedCheckBox * m_impulse;
	LedCheckBox * m_power;

	// Not in model
	nineButtonSelector * m_stringSelector;
	PixmapButton * m_smoothBtn;
	PixmapButton * m_normalizeBtn;

	// From impulse editor
	PixmapButton * m_sinWaveBtn;
	PixmapButton * m_triangleWaveBtn;
	PixmapButton * m_sqrWaveBtn;
	PixmapButton * m_sawWaveBtn;
	PixmapButton * m_whiteNoiseWaveBtn;
	PixmapButton * m_usrWaveBtn;


};

#endif
