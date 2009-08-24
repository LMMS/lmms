/*
 * InstrumentFunctions.h - models for instrument-functions-tab
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "JournallingObject.h"
#include "lmms_basics.h"
#include "AutomatableModel.h"
#include "TempoSyncKnobModel.h"
#include "ComboBoxModel.h"


class InstrumentTrack;
class notePlayHandle;


const int MAX_CHORD_POLYPHONY = 10;


class ChordCreator : public Model, public JournallingObject
{
	Q_OBJECT
public:
	ChordCreator( Model * _parent );
	virtual ~ChordCreator();

	void processNote( notePlayHandle * _n );


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	inline virtual QString nodeName() const
	{
		return "chordcreator";
	}


	static struct Chord
	{
		const QString name;
		Sint8 interval[MAX_CHORD_POLYPHONY];
	} s_chordTable[];


	static inline int getChordSize( Chord & _c )
	{
		int idx = 0;
		while( _c.interval[idx] != -1 )
		{
			++idx;
		}
		return idx;
	}


private:
	BoolModel m_chordsEnabledModel;
	ComboBoxModel m_chordsModel;
	FloatModel m_chordRangeModel;


	friend class ChordCreatorView;

} ;




class Arpeggiator : public Model, public JournallingObject
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

	Arpeggiator( Model * _parent );
	virtual ~Arpeggiator();

	void processNote( notePlayHandle * _n );


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	inline virtual QString nodeName() const
	{
		return "arpeggiator";
	}


private:
	enum ArpModes
	{
		FreeMode,
		SortMode,
		SyncMode
	} ;

	BoolModel m_arpEnabledModel;
	ComboBoxModel m_arpModel;
	FloatModel m_arpRangeModel;
	TempoSyncKnobModel m_arpTimeModel;
	FloatModel m_arpGateModel;
	ComboBoxModel m_arpDirectionModel;
	ComboBoxModel m_arpModeModel;


	friend class FlpImport;
	friend class InstrumentTrack;
	friend class ArpeggiatorView;

} ;


#endif
