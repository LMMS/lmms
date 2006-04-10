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

#include <QtGui/QWidget>

#else

#include <qwidget.h>

#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "journalling_object.h"
#include "types.h"
#include "engine.h"


class QPixmap;

class automatableButtonGroup;
class flpImport;
class instrumentTrack;
class comboBox;
class groupBox;
class knob;
class notePlayHandle;
class tempoSyncKnob;


const int MAX_CHORD_POLYPHONY = 10;


class arpAndChordsTabWidget : public QWidget, public journallingObject
{
	Q_OBJECT
public:
	arpAndChordsTabWidget( instrumentTrack * _channel_track );
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


private:

	enum arpDirections
	{
		OFF,
		UP,
		DOWN,
		UP_AND_DOWN,
		RANDOM
	} ;

	enum arpModes
	{
		FREE,
		SORT,
		SYNC
	} ;

	// chord-stuff
	groupBox * m_chordsGroupBox;
	comboBox * m_chordsComboBox;
	knob * m_chordRangeKnob;

	// arpeggio-stuff
	groupBox * m_arpGroupBox;
	comboBox * m_arpComboBox;
	knob * m_arpRangeKnob;
	tempoSyncKnob * m_arpTimeKnob;
	knob * m_arpGateKnob;
	QLabel * m_arpDirectionLbl;

	automatableButtonGroup * m_arpDirectionBtnGrp;

	comboBox * m_arpModeComboBox;

	friend class flpImport;

} ;


#endif
