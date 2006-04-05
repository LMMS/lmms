/*
 * envelope_tab_widget.h - declaration of class envelopeTabWidget which
 *                         provides UI- and DSP-code for using envelopes, LFOs
 *                         and a filter
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _ENVELOPE_TAB_WIDGET_H
#define _ENVELOPE_TAB_WIDGET_H

#include "qt3support.h"

#ifdef QT4

#include <QWidget>

#else

#include <qwidget.h>

#endif


#include "basic_filters.h"
#include "envelope_and_lfo_widget.h"


class QLabel;

class instrumentTrack;
class comboBox;
class groupBox;
class knob;
class notePlayHandle;
class pixmapButton;
class tabWidget;

class flpImport;


class envelopeTabWidget : public QWidget, public journallingObject
{
	Q_OBJECT
public:
	envelopeTabWidget( instrumentTrack * _channel_track );
	virtual ~envelopeTabWidget();

	void FASTCALL processAudioBuffer( sampleFrame * _ab,
							const fpab_t _frames,
							notePlayHandle * _n );

	enum targets
	{
		VOLUME,
/*		PANNING,
		PITCH,*/
		CUT,
		RES,
		TARGET_COUNT
	} ;

	f_cnt_t envFrames( const bool _only_vol = FALSE );
	f_cnt_t releaseFrames( const bool _only_vol = FALSE );

	float FASTCALL volumeLevel( notePlayHandle * _n, const f_cnt_t _frame );


	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );
	inline virtual QString nodeName( void ) const
	{
		return( "eldata" );
	}


private:
	tabWidget * m_targetsTabWidget;
	envelopeAndLFOWidget * m_envLFOWidgets[TARGET_COUNT];

	// filter-stuff
	groupBox * m_filterGroupBox;
	comboBox * m_filterComboBox;
	knob * m_filterCutKnob;
	knob * m_filterResKnob;

	friend class flpImport;

} ;

#endif
