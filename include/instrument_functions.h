/*
 * instrument_functions.h - models for instrument-functions-tab
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _INSTRUMENT_FUNCTIONS_H
#define _INSTRUMENT_FUNCTIONS_H

#include "journalling_object.h"
#include "types.h"
#include "automatable_model.h"
#include "tempo_sync_knob.h"
#include "combobox_model.h"


class instrumentTrack;
class notePlayHandle;


const int MAX_CHORD_POLYPHONY = 10;


class chordCreator : public model, public journallingObject
{
	Q_OBJECT
public:
	chordCreator( model * _parent );
	virtual ~chordCreator();

	void processNote( notePlayHandle * _n );


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	inline virtual QString nodeName( void ) const
	{
		return( "chordcreator" );
	}


	static struct chord
	{
		const QString name;
		Sint8 interval[MAX_CHORD_POLYPHONY];
	} s_chordTable[];


	static inline int getChordSize( chord & _c )
	{
		int idx = 0;
		while( _c.interval[idx] != -1 )
		{
			++idx;
		}
		return( idx );
	}


private:
	boolModel m_chordsEnabledModel;
	comboBoxModel m_chordsModel;
	floatModel m_chordRangeModel;


	friend class chordCreatorView;

} ;




class arpeggiator : public model, public journallingObject
{
	Q_OBJECT
public:
	enum ArpDirections
	{
		ArpDirUp,
		ArpDirDown,
		ArpDirUpAndDown,
		ArpDirRandom,
		NumArpDirections
	} ;

	arpeggiator( model * _parent );
	virtual ~arpeggiator();

	void processNote( notePlayHandle * _n );


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	inline virtual QString nodeName( void ) const
	{
		return( "arpeggiator" );
	}


private:
	enum ArpModes
	{
		FreeMode,
		SortMode,
		SyncMode
	} ;

	boolModel m_arpEnabledModel;
	comboBoxModel m_arpModel;
	floatModel m_arpRangeModel;
	tempoSyncKnobModel m_arpTimeModel;
	floatModel m_arpGateModel;
	intModel m_arpDirectionModel;
	comboBoxModel m_arpModeModel;


	friend class flpImport;
	friend class instrumentTrack;
	friend class arpeggiatorView;

} ;


#endif
