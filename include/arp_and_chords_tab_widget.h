/*
 * arp_and_chords_tab_widget.h - declaration of class arpAndChordWidget which
 *                               provides code for using arpeggio and chords
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


#ifndef _ARP_AND_CHORDS_TAB_WIDGET_H
#define _ARP_AND_CHORDS_TAB_WIDGET_H

#include "qt3support.h"

#ifdef QT4

#include <QWidget>

#else

#include <qwidget.h>

#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "settings.h"
#include "types.h"


class QComboBox;
class QPixmap;

class channelTrack;
class groupBox;
class knob;
class ledCheckBox;
class notePlayHandle;
class pixmapButton;
class tempoSyncKnob;


const int MAX_CHORD_POLYPHONY = 10;


class arpAndChordsTabWidget : public QWidget, public settings
{
	Q_OBJECT
public:
	arpAndChordsTabWidget( channelTrack * _channel_track );
	virtual ~arpAndChordsTabWidget();

	static struct chord
	{
		const QString name;
		Sint8 interval[MAX_CHORD_POLYPHONY];
	} s_chords[];

	void FASTCALL processNote( notePlayHandle * _n );
	static inline int getChordSize( chord & _c )
	{
		int idx = 0;
		while( _c.interval[idx] != -1 )
		{
			++idx;
		}
		return( idx );
	}


	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );
	inline virtual QString nodeName( void ) const
	{
		return( "arpandchords" );
	}



protected slots:
	void arpUpToggled( bool );
	void arpDownToggled( bool );
	void arpUpAndDownToggled( bool );
	void arpRandomToggled( bool );


private:

	enum arpDirections
	{
		OFF,
		UP,
		DOWN,
		UP_AND_DOWN,
		RANDOM
	} m_arpDirection;

	enum arpModes
	{
		FREE,
		SORT,
		SYNC
	} ;

	// chord-stuff
	groupBox * m_chordsGroupBox;
	QComboBox * m_chordsComboBox;
	knob * m_chordRangeKnob;

	// arpeggio-stuff
	groupBox * m_arpGroupBox;
	QComboBox * m_arpComboBox;
	knob * m_arpRangeKnob;
	tempoSyncKnob * m_arpTimeKnob;
	knob * m_arpGateKnob;
	QLabel * m_arpDirectionLbl;

	pixmapButton * m_arpUpBtn;
	pixmapButton * m_arpDownBtn;
	pixmapButton * m_arpUpAndDownBtn;
	pixmapButton * m_arpRandomBtn;

	QComboBox * m_arpModeComboBox;

} ;


#endif
