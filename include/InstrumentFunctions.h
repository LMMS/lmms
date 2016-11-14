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

	/**
	* The new structure substitutes the int8_t key values. It now holds data
	* for volume and panning, too. In future we could manipulate the single
	* arpeggio note length and pitch
	*
	* @brief The ChordSemiTone struct
	*/
	struct ChordSemiTone {
		int8_t key; // the semitone
		volume_t vol; // its volume : in percentage of the volume, from 0% (silence)
		// to 200% or more, to emboss volume. No control for high volumes yet;
		panning_t pan; // the panning from -100 to +100

		bool active; // the note is played -> true: yes, false: skipped
		bool silenced; // the note is processed but silenced -> true: normally
		// played, false: muted
		bool bare; // The note only has the key value. True: bare, we will
		// discard al the rest, taking data from the base note
		// false: the note has all data, volume, panning, silenced...

		ChordSemiTone(QString s)
		{
			parseString(s); // the parseString Function
		}

		/**
		* The constructor which gets all the data.
		*
		* @brief ChordSemiTone
		* @param k key
		* @param v volume
		* @param p panning
		* @param a active
		* @param s silenced
		* @param b bare
		*/
		ChordSemiTone(int8_t k, volume_t v, panning_t p, bool a, bool s, bool b)
		{
			key = k;
			vol = v;
			pan = p;
			active = a;
			silenced = s;
			bare=b;
		}

		ChordSemiTone() {}

		// the operator of assignment of the chord note (all of it)
		inline ChordSemiTone operator=(ChordSemiTone a)
		{
			key = a.key;
			vol = a.vol;
			pan = a.pan;
			active = a.active;
			silenced = a.silenced;
			bare = a.bare;
			return a;
		}

		/**
		* Reads data into chord semitone, through a string
		* The format read will be three digits to get key, volume, panning, then
		* three boolean fields
		*
		* @brief readData
		* @param d : a string like 3,100,0,1,0,0
		* - key,
		* - volume (0-200 in percentage of the original volume. 100 is the
		* original
		* volume),
		* - panning (from -100 to 100),
		* - active (true: the note is played, false: omitted),
		* - silenced (true: the note is played but without sound),
		* - bare (true: all data ignored except key)
		*
		* @return
		*/
		void parseString(QString d)
		{
			QStringList l = d.remove(' ').split(','); // trims and splits the string
			key = (int8_t)l[0].toInt();
			vol = (volume_t)l[1].toInt();
			pan = (panning_t)l[2].toInt();
			active = (bool)l[3].toShort();
			silenced = (bool)l[4].toShort();
			bare = (bool)l[5].toShort();
		}

		/**
		* Assignment of only the key
		* @param a: the key to assign;
		*/
		inline int8_t operator=(int8_t a)
		{
			key = a;
			bare = true; // the note has only the key component, we discard all the rest
			return a;
		}

		/**
		* Comparison towards a ChordSemitone entity, checks for same key, vol, pan,
		* active, silenced, bare
		* @brief operator ==
		* @param a
		* @return
		*/
		inline bool operator==(ChordSemiTone a)
		{
			if (a.key == key && a.bare == bare && a.vol == vol && a.pan == pan &&
					a.active == active && a.silenced == silenced)
				{
					return true;
				}
			else
				{
					return false;
				}
		}

		/**
		* Comparison towards int8_t integers (key), maintained compatibility with other
		* modules
		*
		* @brief operator ==
		* @param a
		* @return
		*/
		inline bool operator==(int8_t a)
		{
			if (key == a) {
					return true;
				}
			return false;
		}
	};

private:
	/*
	* Originally ChordSemiTones was an array of int8_t keys
	* - typedef int8_t ChordSemiTones [MAX_CHORD_POLYPHONY];
	* And the MAX_CHORD_POLYPHONY was defining its maximum length of 13 elements.
	*
	* Now it's a QVector, with an overloaded [] operator to maintain compatibility
	*/
	struct ChordSemiTones : public QVector<ChordSemiTone> {

		/**
		* Takes the string, divides it and pushes the single ChordSemitone;
		* boundary char is ";"
		* @brief ChordSemiTones
		* @param s
		*/
		ChordSemiTones(QString s)
		{
			QStringList l = s.remove(' ').split(';');
			foreach (QString s, l)
				{
					if (s.isEmpty())
						{ // to eliminate the eventual empty QString derived from the last delimiter
							break;
						}
					// reads the data into semitone and pushes it into vector
					ChordSemiTone *cst = new ChordSemiTone(s);
					push_back(*cst);
				}
		}

		/**
		* Empty constructor
		* @brief ChordSemiTones
		*/
		ChordSemiTones() {}

		/**
		* Compatibility towards other modules, return the int8_t key
		* @brief operator []
		* @param index
		* @return
		*/
		int8_t inline operator[](int index) const {
			return at(index).key; // returns the key of the note
		}
	};

public:
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

		Chord( const char *n, const ChordSemiTones semi_tones);

		Chord( const char *n, const QString s);

		Chord( QString n, const QString s);

		Chord( QString n, const ChordSemiTones semi_tones);

		int size() const
		{
			return m_semiTones.size();
		}

		bool isScale() const
		{
			return size() > 6;
		}

		bool isEmpty() const
		{
			return size() == 0;
		}

		bool hasSemiTone(int8_t semiTone) const;

		const int8_t &last() const
		{
			return m_semiTones.last().key;
		}

		const QString &getName() const
		{
			return m_name;
		}

		int8_t operator[](int n) const {
			// recalling the ChordSemiTones overloaded operator [], which returns the
			// key, for the sake of compatibility
			return m_semiTones[n];
		}

		const ChordSemiTones getChordSemiTones() const
		{
			return m_semiTones;
		}


		/**
		* In opposition to the overloaded [] this gets the ChordSemiTone object
		* @brief at
		* @param i
		* @return
		*/
		const ChordSemiTone at(int i) const {
			return m_semiTones.at(i); // The note
		}
	};

	struct ChordTable : public QVector<Chord> {
	private:
		ChordTable();

		struct Init : public QVector<Chord>
		{
			const char *m_name;
			ChordSemiTones m_semiTones;

			Init(); // The init constructor will initialize the static variable
		};

//		static Init s_initTable[]; // Old initializer

		static Init s_initializer; // This is now going to get the data


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


#endif
