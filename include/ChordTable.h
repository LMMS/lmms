/*
 * ChordTable.h - The notestacking/arpeggio chord classes
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2016-2017 Riki Sluga <rikislav/at/gmail.com>
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
 * License along with this program ( see COPYING ); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#ifndef CHORD_TABLE_H
#define CHORD_TABLE_H

#include <QStringList>

#include "ConfigManager.h"
#include "JournallingObject.h"
#include "AutomatableModel.h"

class Model;
class Chord;
class ChordTable;

//new constants for key notes in arpeggios, used by the Int Automated model
const int KeyMax = ( 0 + 30 );
const int KeyMin = -KeyMax;
const int KeyDefault = 0;


class EXPORT ChordSemiTone : public Model , public JournallingObject
{
	Q_OBJECT
public:

	// the semitone
	IntModel * key; 
	FloatModel * vol;
	FloatModel * pan;
	// the note is played -> true: yes, false: skipped
	//TO DO: has to be implemented yet
	BoolModel * active;
	// the note is processed but silenced -> true: normally played, false: muted
	BoolModel * silenced;
	// random: the note will be a random value within the abs(key) range
	BoolModel * rand;
	// The note only has the key value. True: bare, we will discard al the rest, taking data from the base note
	BoolModel * bare;

	//The semitone will always belong to a chord
	ChordSemiTone( Chord * _parent );

	//Creates a new semitone which gets data from the argument
	ChordSemiTone( ChordSemiTone * _copy );

	//constructs the object from a given string by calling the parseString function
	ChordSemiTone( Chord * _parent, QString _string );

	virtual ~ChordSemiTone();

	//return the parent chord
	Chord * getChord()
	{
		return qobject_cast< Chord * >( parent() );
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

class EXPORT Chord : public Model, public JournallingObject, public QVector<ChordSemiTone * >
{
	Q_OBJECT
public:
	QString m_name;

	//creates an empty chord
	Chord( Model * _parent );

	//creates a chord by parsing the semitone string
	Chord( Model * _parent, QString _name, QString _string );

	//creates a new Chord without semitones
	Chord( Model * _parent, QString _name );

	//Creates a copy of the chord, changing name too;
	Chord( Chord * _copy, QString _name );

	virtual ~Chord();

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	inline virtual QString nodeName() const
	{
		return "Chord";
	}

	//TO DO : check for duplicate keys
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
		if ( size() > 0 )
		{
			return at( size() - 1 )->key->value();
		} else {
			return 0;
		}
	}

	const QString & getName() const
	{
		return m_name;
	}

	// returning the key
	int8_t operator[]( int n ) const
	{
		return at( n )->key->value();
	}

	//Adds at the end a standard semitone
	void addSemiTone();

	//inserts the semitone in the stated position
	void insertSemiTone( ChordSemiTone * csm, int position );

	//deletes the semitone
	void removeSemiTone( int i );

	//parses the string into data by adding the Semitones to the vector after clearing it
	void parseString( QString _string );
};



class EXPORT ChordTable : public Model, public JournallingObject, public QVector<Chord * >
{
	Q_OBJECT
public:
	ChordTable( Model * _parent );
	virtual ~ChordTable() {}

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	inline virtual QString nodeName() const
	{
		return "ChordTable";
	}

	//old function, not used in the new version which makes use of the available data functions.
	bool readXML();

	//loads the factory preset
	void loadFactoryPreset();

	//adds or clones a chord to the chord table. If -1 adds a chord else clones the given
	void cloneChord( int i = -1 );

	//removes the chord and emits the datachanged signal;
	void removeChord( int i );


	const Chord & getByName( const QString & name, bool is_scale = false ) const;

	const Chord & getScaleByName( const QString & name ) const
	{
		return getByName( name, true );
	}

	const Chord & getChordByName( const QString & name ) const
	{
		return getByName( name, false );
	}


	//tricking compiler
	//the compiler before qt5 treats signals as private
	void emitChordTableChangedSignal()
	{
		emit chordTableChanged();
	}

	void emitChordNameChanged()
	{
		emit chordNameChanged();
	}

public slots:

	//reloads the chordtable from the file
	void reset();

signals:

	//emitted only when chord names are changed to reflect in other classes
	void chordNameChanged();
	//emitted when the chord table data has changed
	void chordTableChanged();

};

#endif
