/*
 * vibed.h - combination of PluckedStringSynth and BitInvader
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/yahoo/com>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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
#ifndef _VIBED_STRINGS_H
#define _VIBED_STRINGS_H

#include "instrument.h"
#include "instrument_view.h"
#include "sample_buffer.h"
#include "graph.h"
#include "knob.h"
#include "pixmap_button.h"
#include "led_checkbox.h"
#include "nine_button_selector.h"

class vibedView;
class notePlayHandle;

class vibed : public instrument
{
	Q_OBJECT
public:
	vibed( instrumentTrack * _instrument_track );
	virtual ~vibed();

	virtual void playNote( notePlayHandle * _n, bool _try_parallelizing,
						sampleFrame * _working_buffer );
	virtual void deleteNotePluginData( notePlayHandle * _n );


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName( void ) const;

	virtual pluginView * instantiateView( QWidget * _parent );


private:
	QList<knobModel*> m_pickKnobs;
	QList<knobModel*> m_pickupKnobs;
	QList<knobModel*> m_stiffnessKnobs;
	QList<knobModel*> m_volumeKnobs;
	QList<knobModel*> m_panKnobs;
	QList<knobModel*> m_detuneKnobs;
	QList<knobModel*> m_randomKnobs;
	QList<knobModel*> m_lengthKnobs;
	QList<boolModel*> m_powerButtons;
	QList<graphModel*> m_graphs;
	QList<boolModel*> m_impulses;
	QList<nineButtonSelectorModel*> m_harmonics;

	static const int __sampleLength = 128;

	friend class vibedView;
} ;



class vibedView : public instrumentView
{
	Q_OBJECT
public:
	vibedView( instrument * _instrument,
					QWidget * _parent );
	virtual ~vibedView() {};

public slots:
	void showString( Uint8 _string );
	void contextMenuEvent( QContextMenuEvent * );
	void displayHelp( void );

protected slots:
	void sinWaveClicked( void );
	void triangleWaveClicked( void );
	void sawWaveClicked( void );
	void sqrWaveClicked( void );
	void noiseWaveClicked( void );
	void usrWaveClicked( void );
	void smoothClicked( void );
	void normalizeClicked( void );

private:
	virtual void modelChanged( void );


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
