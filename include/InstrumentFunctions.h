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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#ifndef INSTRUMENT_FUNCTIONS_H
#define INSTRUMENT_FUNCTIONS_H

#include <QStringList>

#include "ConfigManager.h"
#include "volume.h"
#include "panning.h"

#include "JournallingObject.h"
#include "lmms_basics.h"
#include "AutomatableModel.h"
#include "TempoSyncKnobModel.h"
#include "ComboBoxModel.h"


class InstrumentTrack;
class NotePlayHandle;
class Model;
class ChordTable;

//new constants for key notes in arpeggios, used by the Int Automated model
const int KeyMax = ( 0 + 30 );
const int KeyMin = - KeyMax;
const int KeyDefault = 0;


/*****************************************************************************************************
 *
 * The InstrumentFunctionNoteStacking class
 *
******************************************************************************************************/
class InstrumentFunctionNoteStacking : public Model, public JournallingObject
{
	Q_OBJECT

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

private:

	ChordTable *m_chordTable;
	BoolModel m_chordsEnabledModel;
	ComboBoxModel m_chordsModel;
	FloatModel m_chordRangeModel;

	friend class InstrumentFunctionNoteStackingView;

} ;


/*****************************************************************************************************
 *
 * The InstrumentFunctionArpeggio class
 *
******************************************************************************************************/
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

	//reloads the chord table into the widget model, for example when data changes
	void reloadChordTable();

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

	ChordTable *m_chordTable;

	BoolModel m_arpEnabledModel;
	ComboBoxModel m_arpModel;
	FloatModel m_arpRangeModel;
	FloatModel m_arpCycleModel;
	FloatModel m_arpSkipModel;
	FloatModel m_arpMissModel;
	TempoSyncKnobModel m_arpTimeModel;
	FloatModel m_arpGateModel;
	ComboBoxModel m_arpDirectionModel;
	ComboBoxModel m_arpModeModel;


	friend class InstrumentTrack;
	friend class InstrumentFunctionArpeggioView;

};

/*****************************************************************************************************
 *
 * The ChordSemiTone class
 *
******************************************************************************************************/
class ChordSemiTone : public Model , public JournallingObject
{
	Q_OBJECT
public:

	//the name which propagates from the chord and position for automation
	//TODO : to be implemented!!
	QString m_name;

	IntModel *key; //  the semitone
	FloatModel *vol;
	FloatModel *pan;
	// the note is played -> true: yes, false: skipped
	BoolModel *active;
	// the note is processed but silenced -> true: normally played, false: muted
	BoolModel *silenced;
	// The note only has the key value. True: bare, we will discard al the rest, taking data from the base note
	BoolModel *bare;

	ChordSemiTone(Model *_parent);

	ChordSemiTone(ChordSemiTone *_copy);

	//constructs the object from a given string by calling the parseString function
	ChordSemiTone(Model *_parent,QString _string);

	virtual ~ChordSemiTone() {}

	//parses a string into data
	void parseString( QString _string );

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	inline virtual QString nodeName() const
	{
		return "chordSemiTone";
	}

	QString name() const;
	void setName(const QString &name);
};


/*****************************************************************************************************
 *
 * The ChordSemiTones class
 *
******************************************************************************************************/
class ChordSemiTones : public Model, public JournallingObject, public QVector<ChordSemiTone*>
{
	Q_OBJECT
public:
	ChordSemiTones(Model *_parent);

	//Calls parseString
	ChordSemiTones(Model *_parent,QString _string);

	virtual ~ChordSemiTones() {}

	//parses the string into data by adding the Semitones to the vector after clearing it
	void parseString( QString _string );

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	inline virtual QString nodeName() const
	{
		return "chordSemiTones";
	}

};


/*****************************************************************************************************
 *
 * The Chord class
 *
******************************************************************************************************/
class Chord : public Model, public JournallingObject
{
	Q_OBJECT
public:

	ChordSemiTones *m_chordSemiTones;
	QString m_name;

	Chord(Model *_parent);

	Chord(Model *_parent, QString _name, QString _string);

	Chord(Model *_parent, QString _name, ChordSemiTones * _semitones );

	virtual ~Chord() {}

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	inline virtual QString nodeName() const
	{
		return "Chord";
	}

	int size() const
	{
		return m_chordSemiTones->size();
	}

	bool isScale() const
	{
		return m_chordSemiTones->size() > 6;
	}

	bool isEmpty() const
	{
		return m_chordSemiTones->size() == 0;
	}

	bool hasSemiTone( int8_t semiTone ) const;

	int8_t last() const
	{
		return m_chordSemiTones->last()->key->value();
	}

	const QString &getName() const
	{
		return m_name;
	}

	int8_t operator[]( int n ) const
	{
		// returning the key
		return m_chordSemiTones->at(n)->key->value();
	}

	const ChordSemiTones *getChordSemiTones() const
	{
		return m_chordSemiTones;
	}

	ChordSemiTones *getChordSemiTones()
	{
		return m_chordSemiTones;
	}


	/**
	* In opposition to the overloaded [] this gets the ChordSemiTone object
	* @brief at
	* @param i
	* @return
	*/
//	const ChordSemiTone *at( int i ) const
//	{
//		return m_chordSemiTones->at( i ); // The note
//	}

	ChordSemiTone *at( int i )
	{
		return m_chordSemiTones->at( i ); // The note
	}

	//Adds at the end a standard semitone
	void addSemiTone();

	//inserts the semitone in the stated position
	void insertSemiTone(ChordSemiTone *csm, int position);

	//deletes the semitone
	void removeSemiTone(int i);

signals:

	//whenever a semitone is added/deleted
	void emitStructureEdited();


};



/*****************************************************************************************************
 *
 * The ChordTable class
 *
******************************************************************************************************/
class ChordTable : public Model, public JournallingObject, public QVector<Chord*>
{
	Q_OBJECT
public:
	ChordTable(Model *_parent);
	virtual ~ChordTable() {}

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	inline virtual QString nodeName() const
	{
		return "ChordTable";
	}

	/**
	 * Reads data from the predefined file
	 * @brief readXML
	 */
	bool readXML();

	const Chord &getByName( const QString & name, bool is_scale = false ) const;
	Chord &getByName( const QString & name, bool is_scale = false );

	const Chord & getScaleByName( const QString & name ) const
	{
		return getByName( name, true );
	}

	const Chord & getChordByName( const QString & name ) const
	{
		return getByName( name, false );
	}

	//getting the instance of the class. Guarantees only one instance of the class
	static ChordTable *getInstance(Model *_parent);
private:
	static ChordTable *instance;

public slots:

	//reloads the chordtable from the file
	void reset();

};

#endif
