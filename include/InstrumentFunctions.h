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
class Chord;

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

public slots:
	//reloads the chords combo model
	void updateChordTable();


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
		ArpDirDownAndUp,
		ArpDirRandom,
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

public slots:
	//reloads the chord table into the widget model, for example when data changes
	void updateChordTable();

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

} ;

/*****************************************************************************************************
 *
 * The ChordSemiTone class
 *
******************************************************************************************************/
class ChordSemiTone : public Model , public JournallingObject
{
	Q_OBJECT
public:

	IntModel *key; //  the semitone
	FloatModel *vol;
	FloatModel *pan;
	// the note is played -> true: yes, false: skipped
	BoolModel *active;
	// the note is processed but silenced -> true: normally played, false: muted
	BoolModel *silenced;
	// The note only has the key value. True: bare, we will discard al the rest, taking data from the base note
	BoolModel *bare;

	//The semitone will always belong to a chord
	ChordSemiTone(Chord *_parent);

	//Creates a new semitone which gets data from the argument
	ChordSemiTone(ChordSemiTone *_copy);

	//constructs the object from a given string by calling the parseString function
	ChordSemiTone(Chord *_parent,QString _string);

	virtual ~ChordSemiTone();

	//return the parent chord
	Chord *getChord()
	{
		return qobject_cast<Chord*>(parent());
	}


	//parses a string into data
	void parseString( QString _string );

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	inline virtual QString nodeName() const
	{
		return "chordSemiTone";
	}
};

/*****************************************************************************************************
 *
 * The Chord class
 *
******************************************************************************************************/
class Chord : public Model, public JournallingObject, public QVector<ChordSemiTone*>
{
	Q_OBJECT
public:
	//the chord Name;
	QString m_name;

	//creates an empty chord
	Chord(Model *_parent);

	//creates a chord by parsing the semitone string
	Chord(Model *_parent, QString _name, QString _string);

	//creates a new Chord without semitones
	Chord(Model *_parent, QString _name );

	//Creates a copy of the chord, changing name too;
	Chord(Chord *_copy, QString _name);

	virtual ~Chord();

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	inline virtual QString nodeName() const
	{
		return "Chord";
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

	//returns the key of the last chordsemitone
	int8_t last() const
	{
		if (size()>0)
		{
			return at(size()-1)->key->value();
		} else {
			return 0;
		}
	}

	const QString &getName() const
	{
		return m_name;
	}

	int8_t operator[]( int n ) const
	{
		// returning the key
		return at(n)->key->value();
	}


	//Adds at the end a standard semitone
	void addSemiTone();

	//inserts the semitone in the stated position
	void insertSemiTone(ChordSemiTone *csm, int position);

	//deletes the semitone
	void removeSemiTone(int i);

	//parses the string into data by adding the Semitones to the vector after clearing it
	void parseString( QString _string );


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

	//old function, not used in the new version which makes use of the available xml functions.
	bool readXML();

	//loads the factory preset
	void loadFactoryPreset();

	//adds or clones a chord to the chord table. If -1 adds a chord else clones the given
	void cloneChord(int i=-1);

	//removes the chord and emits the datachanged signal;
	void removeChord(int i);


	const Chord &getByName( const QString & name, bool is_scale = false ) const;

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

	//the compiler before qt5 treats signals as private
	void emitChordTableChangedSignal()
	{
		emit chordTableChanged();
	}
	//tricking compiler
	void emitChordNameChanged()
	{
		emit chordNameChanged();
	}

private:
	static ChordTable *instance;

public slots:

	//reloads the chordtable from the file
	void reset();

signals:
	//emitted only when chord names are changed to reflect in piano roll
	void chordNameChanged();
	//emitted when the chord table has changed
	void chordTableChanged();

};

#endif
