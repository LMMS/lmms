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

#include "Model.h"
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

//new constants for key notes in arpeggios, used by the Int Automated model
const int KeyMax = ( 0 + 20 );
const int KeyMin = - KeyMax;
const int KeyDefault = 0;


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
	struct ChordSemiTone
	{
		IntModel *key = new IntModel(KeyDefault,KeyMin,KeyMax,NULL,"Key"); //  the semitone
		FloatModel *vol = new FloatModel(DefaultVolume,MinVolume, MaxVolume, 0.1f, NULL, tr( "Volume" ) ); // its volume : in percentage of the volume, from 0% (silence)
		// to 200% or more, to emboss volume. No control for high volumes yet;
		FloatModel *pan = new FloatModel( PanningCenter, PanningLeft, PanningRight, 0.1f, NULL, tr( "Panning" )); // the panning from -100 to +100

		BoolModel *active= new BoolModel(true,NULL,tr("Active")); // the note is played -> true: yes, false: skipped
		BoolModel *silenced= new BoolModel(true,NULL,tr("Silenced")); // the note is processed but silenced -> true: normally
		// played, false: muted
		BoolModel *bare= new BoolModel(true,NULL,tr("Bare")); // The note only has the key value. True: bare, we will
		// discard al the rest, taking data from the base note
		// false: the note has all data, volume, panning, silenced...

		//reference to the parent model for propagation
		//		InstrumentFunctionNoteStacking *outerclass= NULL;

		ChordSemiTone( QString s )
		{
			//			outerclass=parent;
			parseString( s ); // the parseString Function
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
		ChordSemiTone( int8_t k, volume_t v, panning_t p, bool a, bool s, bool b )
		{
			//			outerclass=parent;
			key->setValue(k);
			//			key = k;
			vol->setValue(v);
			pan->setValue(p);
			active->setValue( a);
			silenced->setValue( s);
			bare->setValue(b);
		}

		ChordSemiTone() {}

		// the operator of assignment of the chord note (all of it)
		inline ChordSemiTone operator=( ChordSemiTone a )
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
		void parseString( QString d )
		{
			QStringList l = d.remove(' ').split(','); // trims and splits the string
			//			key = (int8_t)l[0].toInt();
			key->setValue(l[0].toInt());
			vol->setValue(l[1].toFloat());
			pan->setValue(l[2].toFloat());
			active->setValue(l[3].toShort());
			silenced->setValue(l[4].toShort());
			bare->setValue(l[5].toShort());
		}

		/**
		* Assignment of only the key
		* @param a: the key to assign;
		*/
		inline int8_t operator=( int8_t a )
		{
			//			key = a;
			key->setValue(a);
			bare->setValue(true); // the note has only the key component, we discard all the rest
			return a;
		}

		/**
		* Comparison towards a ChordSemitone entity, checks for same key, vol, pan,
		* active, silenced, bare
		* @brief operator ==
		* @param a
		* @return
		*/
		inline bool operator==( ChordSemiTone a )
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
		inline bool operator==( int8_t a )
		{
			//			if (key == a)
			if (key->value() == a)
			{
				return true;
			}
			return false;
		}

		//sets the model to all model data. Decided to do so not to complicate things
		void setModel(InstrumentFunctionNoteStacking *model)
		{
			key->setParent(model);
			pan->setParent(model);
			vol->setParent(model);
			silenced->setParent(model);
			active->setParent(model);
			bare->setParent(model);
		}

		//checks if the model has been set to all model data
		bool isModelSet()
		{
			if (key->parentModel()==NULL || vol->parentModel()==NULL ||
					pan->parentModel()==NULL || active->parentModel()==NULL ||
					silenced->parentModel()==NULL || bare->parentModel()==NULL)
			{
				return false;
			}
			return true;
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
	struct ChordSemiTones : public QVector<ChordSemiTone>
	{


		//reference to the parent model for propagation
		//		InstrumentFunctionNoteStacking *outerclass= NULL;

		/**
		* Takes the string, divides it and pushes the single ChordSemitone;
		* boundary char is ";"
		* @brief ChordSemiTones
		* @param s
		*/
		ChordSemiTones( QString s )
		{
			//			outerclass=parent;
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
		ChordSemiTones()
		{

		}


		/**
		* Compatibility towards other modules, return the int8_t key
		* @brief operator []
		* @param index
		* @return
		*/
		int8_t inline operator[]( int index ) const
		{
			return at(index).key->value(); // returns the key of the note
		}

		//Checks if the model is set for all semitones
		bool isModelSet()
		{
			InstrumentFunctionNoteStacking::ChordSemiTone *st=NULL;
			for(int i=0;i<this->size(),i++;)
			{
				*st=at(i);
				if (st->isModelSet())
				{
					return false;
				}
			}
			return true;
		}

		//Sets the model to all underlying semitones
		void setModel(InstrumentFunctionNoteStacking *model)
		{
			InstrumentFunctionNoteStacking::ChordSemiTone *st=NULL;
			for(int i=0;i<this->size(),i++;)
			{
				*st=at(i);
				st->setModel(model);
			}
		}
	};

public:
	InstrumentFunctionNoteStacking( Model * _parent );
	virtual ~InstrumentFunctionNoteStacking();


	void processNote( NotePlayHandle* n );

	//reads the chord table from the xml file and emits the signal it has read it
	void loadChordTable();

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

		Chord() : m_size( 0 )
		{}

		Chord( const char * n, const ChordSemiTones & semi_tones );

		Chord( const char * n, const ChordSemiTones semi_tones );

		Chord( const char * n, const QString s );

		Chord( QString n, const QString s );

		Chord( QString n, const ChordSemiTones semi_tones );

		Chord( QString n, const ChordSemiTones * semi_tones );

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

		bool hasSemiTone( int8_t semiTone ) const;

		const int8_t &last() const
		{
			return m_semiTones.last().key->value();
		}

		const QString &getName() const
		{
			return m_name;
		}

		int8_t operator[]( int n ) const
		{
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
		const ChordSemiTone at( int i ) const
		{
			return m_semiTones.at( i ); // The note
		}

		//Checks if the model is set for all the chord semitones
		bool isModelSet()
		{
			return m_semiTones.isModelSet();
		}

		//Sets the model to all underlying chord semitones
		void setModel(InstrumentFunctionNoteStacking *model)
		{
			m_semiTones.setModel(model);
		}

	};

	struct  ChordTable : public QVector<Chord>
	{
	public:
		ChordTable();
	private:

		struct Init : public QVector<Chord>
		{
			const char *m_name;
			ChordSemiTones m_semiTones;

			//Checks if the model is set for all the chord semitones
			bool isModelSet()
			{
				return m_semiTones.isModelSet();
			}

			//Sets the model to all underlying chord semitones
			void setModel(InstrumentFunctionNoteStacking *model)
			{
				m_semiTones.setModel(model);
			}

			/**
			 * Reads data from the predefined file
			 * @brief readXML
			 */
			bool readXML();
			// The init constructor will initialize the static variable by reading data from
			//the XML presets
			Init();
		};

		static ChordTable *m_chordTable;// This is now holding the data

	public:


		//swaps the m_chordtable vector pointer with the Init structure parameter
		static void swapInit(Init initializer);

		//reads data from the presets file and reinitializes the ChordTable m_chordtable pointer;
		static void readDataFromXML();

		static ChordTable &getInstance()
		{
			if (m_chordTable==NULL)
			{
				Init *m_init=new Init();
				if (m_init->isModelSet())
				{
					int zz=0;
				}
				swapInit(*m_init);
			}
			return *m_chordTable;
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
	} chord_table;




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


#endif
