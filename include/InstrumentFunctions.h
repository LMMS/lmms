/*
 * InstrumentFunctions.h - models for instrument-functions-tab
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _INSTRUMENT_FUNCTIONS_H
#define _INSTRUMENT_FUNCTIONS_H

#include "JournallingObject.h"
#include "lmms_basics.h"
#include "AutomatableModel.h"
#include "TempoSyncKnobModel.h"
#include "ComboBoxModel.h"


class InstrumentTrack;
class NotePlayHandle;



class InstrumentFunctionNoteStacking : public Model, public JournallingObject
{
	Q_OBJECT

public:
	static const int MAX_CHORD_POLYPHONY = 13;

private:
	typedef int8_t ChordSemiTones [MAX_CHORD_POLYPHONY];

public:
	InstrumentFunctionNoteStacking( Model * _parent );
	virtual ~InstrumentFunctionNoteStacking();

	void processNote( NotePlayHandle* n );


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	inline virtual QString nodeName() const
	{
		return "chordcreator";
	}


	struct Chord
	{
	private:
		QString m_name;
		ChordSemiTones m_semiTones;
		int m_size;

	public:
		Chord() : m_size( 0 ) {}

		Chord( const char * n, const ChordSemiTones & semi_tones );

		int size() const
		{
			return m_size;
		}

		bool isScale() const
		{
			return size() > 6;
		}

		bool isEmpty() const
		{
			return size() == 0;
		}

		bool hasSemiTone( int8_t semiTone ) const;

		int8_t last() const
		{
			return m_semiTones[size() - 1];
		}

		const QString & getName() const
		{
			return m_name;
		}

		int8_t operator [] ( int n ) const
		{
			return m_semiTones[n];
		}
	};


	struct ChordTable : public QVector<Chord>
	{
	private:
		ChordTable();

		struct Init
		{
			const char * m_name;
			ChordSemiTones m_semiTones;
		};

		static Init s_initTable[];

	public:
		static const ChordTable & getInstance()
		{
			static ChordTable inst;
			return inst;
		}

		const Chord & getByName( const QString & name, bool is_scale = false ) const;

		const Chord & getScaleByName( const QString & name ) const
		{
			return getByName( name, true );
		}

		const Chord & getChordByName( const QString & name ) const
		{
			return getByName( name, false );
		}
	};


private:
	BoolModel m_chordsEnabledModel;
	ComboBoxModel m_chordsModel;
	FloatModel m_chordRangeModel;


	friend class InstrumentFunctionNoteStackingView;

} ;




class InstrumentFunctionArpeggio : public Model, public JournallingObject
{
	Q_OBJECT
public:
	enum ArpDirections
	{
		ArpDirUp,
		ArpDirDown,
		ArpDirUpAndDown,
		ArpDirRandom,
		ArpDirDownAndUp,
		NumArpDirections
	} ;

	InstrumentFunctionArpeggio( Model * _parent );
	virtual ~InstrumentFunctionArpeggio();

	void processNote( NotePlayHandle* n );


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
	friend class InstrumentFunctionArpeggioView;

} ;


#endif
