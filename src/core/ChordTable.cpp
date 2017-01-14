/*
 * InstrumentFunctions.cpp - models for instrument-function-tab
 *
 * Copyright ( c ) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or ( at your option ) any later version.
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

#include <QDir>
#include <QFile>
#include <QDomElement>
#include <QDomDocument>
#include <QDomNodeList>
#include <QDomNode>

#include "ChordTable.h"
#include "Engine.h"
#include "InstrumentTrack.h"

/*****************************************************************************************************
 *
 * The ChordSemiTone class
 *
******************************************************************************************************/
ChordSemiTone::ChordSemiTone( Chord *_parent ) :
	Model( _parent ),
	key( new IntModel( KeyDefault, KeyMin, KeyMax, _parent, tr( "Key " )+_parent->m_name ) ),
	vol( new FloatModel( DefaultVolume, MinVolume, MaxVolume, 0.1f, _parent, tr( "Volume " )+_parent->m_name ) ),
	pan( new FloatModel( PanningCenter, PanningLeft, PanningRight, 0.1f, _parent, tr( "Panning " )+_parent->m_name ) ),
	active( new BoolModel( true, _parent, tr( "Active " )+_parent->m_name ) ),
	silenced( new BoolModel( false, _parent, tr( "Silenced " )+_parent->m_name ) ),
	bare( new BoolModel( false, _parent, tr( "Bare " )+_parent->m_name ) )
{
}

ChordSemiTone::ChordSemiTone( ChordSemiTone *_copy ) :
	Model( _copy->parentModel() ),
	key( new IntModel( KeyDefault, KeyMin, KeyMax, _copy->getChord(), tr( "Key " )+_copy->getChord()->m_name ) ),
	vol( new FloatModel( DefaultVolume, MinVolume, MaxVolume, 0.1f, _copy->getChord(), tr( "Volume " )+_copy->getChord()->m_name ) ),
	pan( new FloatModel( PanningCenter, PanningLeft, PanningRight, 0.1f, _copy->getChord(), tr( "Panning " )+_copy->getChord()->m_name ) ),
	active( new BoolModel( true, _copy->getChord(), tr( "Active " )+_copy->getChord()->m_name ) ),
	silenced( new BoolModel( false, _copy->getChord(), tr( "Silenced " )+_copy->getChord()->m_name ) ),
	bare( new BoolModel( false, _copy->getChord(), tr( "Bare " )+_copy->getChord()->m_name ) )
{
	key->setValue( _copy->key->value() );
	vol->setValue( _copy->vol->value() );
	pan->setValue( _copy->pan->value() );
	active->setValue( _copy->active->value() );
	silenced->setValue( _copy->silenced->value() );
	bare->setValue( _copy->bare->value() );
}

ChordSemiTone::ChordSemiTone( Chord *_parent, QString _string ) :
	Model( _parent ),
	key( new IntModel( KeyDefault, KeyMin, KeyMax, _parent, tr( "Key " )+_parent->m_name ) ),
	vol( new FloatModel( DefaultVolume, MinVolume, MaxVolume, 0.1f, _parent, tr( "Volume " )+_parent->m_name ) ),
	pan( new FloatModel( PanningCenter, PanningLeft, PanningRight, 0.1f, _parent, tr( "Panning " )+_parent->m_name ) ),
	active( new BoolModel( true, _parent, tr( "Active " )+_parent->m_name ) ),
	silenced( new BoolModel( false, _parent, tr( "Silenced " )+_parent->m_name ) ),
	bare( new BoolModel( false, _parent, tr( "Bare " )+_parent->m_name ) )
{
	parseString( _string );
}

ChordSemiTone::~ChordSemiTone()
{
	delete bare;
	delete silenced;
	delete active;
	delete pan;
	delete vol;
	delete key;
}

void ChordSemiTone::saveSettings( QDomDocument &_doc, QDomElement &_parent )
{
	key->saveSettings( _doc, _parent, "key" );
	vol->saveSettings( _doc, _parent, "vol" );
	pan->saveSettings( _doc, _parent, "pan" );
	active->saveSettings( _doc, _parent, "active" );
	silenced->saveSettings( _doc, _parent, "silenced" );
	bare->saveSettings( _doc, _parent, "bare" );
}

void ChordSemiTone::loadSettings( const QDomElement &_this )
{
	key->loadSettings( _this, "key" );
	vol->loadSettings( _this, "vol" );
	pan->loadSettings( _this, "pan" );
	active->loadSettings( _this, "active" );
	silenced->loadSettings( _this, "silenced" );
	bare->loadSettings( _this, "bare" );
}

void ChordSemiTone::parseString( QString _string )
{
	QStringList l = _string.remove( ' ' ).split( ', ' ); // trims and splits the string
	key->setValue( l[0].toInt() );
	vol->setValue( l[1].toFloat() );
	pan->setValue( l[2].toFloat() );
	active->setValue( l[3].toShort() );
	silenced->setValue( l[4].toShort() );
	bare->setValue( l[5].toShort() );
}

/*****************************************************************************************************
 *
 * The Chord class
 *
******************************************************************************************************/
Chord::Chord( Model *_parent ) :
	Model ( _parent ),
	m_name( "empty" )
{
}

Chord::Chord( Model *_parent, QString _name, QString _string ) :
	Model ( _parent )
{
	m_name =_name;
	parseString( _string );
}

Chord::Chord( Model *_parent, QString _name ) :
	Model ( _parent )
{
	m_name =_name;
}


Chord::Chord( Chord *_copy, QString _name ) :
	Model( _copy->parentModel() )
{
	m_name = _name;
	ChordSemiTone *csm;
	for ( int i=0;i<_copy->size();i++ )
	{
		csm= new ChordSemiTone( _copy->at( i ) );
		push_back( csm );
	}
}

Chord::~Chord()
{
	for( int i=0;i<this->size();i++ )
	{
		delete at( i );
	}
}

void Chord::saveSettings( QDomDocument &_doc, QDomElement &_parent )
{
	_parent.setAttribute( "name", m_name );
	ChordSemiTone *cst;
	for( int i=0;i<this->size();i++ )
	{
		QDomElement semitone = _doc.createElement( QString( "semitone" ) );
		_parent.appendChild( semitone );
		cst=this->at( i );
		cst->saveSettings( _doc, semitone );
	}

}

void Chord::loadSettings( const QDomElement &_this )
{
	//getting the first chordsemitone data
	m_name=_this.attribute( "name" );

	//the vector element counter
	int i=0;

	//the first child node
	QDomNode node = _this.firstChild();
	//the child element
	QDomElement semitone;

	//the chord to be read into
	ChordSemiTone *cst;
	while ( !node.isNull() )
	{
		semitone = node.toElement();
		//if the vector is empty creates new semitone, otherwise it uses the existing one.
		if ( i<size() )
		{
			cst=at( i );
			cst->loadSettings( semitone );
		} else
		{
			cst=new ChordSemiTone( this );
			cst->loadSettings( semitone );
			push_back( cst );
		}
		i++;
		node = node.nextSibling();
	}
	//removing the extra semitones if they are present
	if ( i<size() )
	{
		for ( int j = size() - 1; j >= i; j-- )
		{
			cst=at( j );
			remove( j );
			delete cst;
		}
	}

}

bool Chord::hasSemiTone( int8_t semiTone ) const
{
	for( int i = 0; i < size(); ++i )
	{
		if( semiTone == at( i )->key->value() )
		{
			return true;
		}
	}
	return false;
}

void Chord::addSemiTone()
{
	ChordSemiTone * csm=new ChordSemiTone( this );
	push_back( csm );
	//emits the data changed signal
	Engine::chordTable()->emitChordTableChangedSignal();
}

void Chord::insertSemiTone( ChordSemiTone * csm, int position )
{
	insert( position, csm );
	//emits the data changed signal
	Engine::chordTable()->emitChordTableChangedSignal();
}

void Chord::removeSemiTone( int i )
{
	ChordSemiTone * cst=at( i );
	remove( i );
	delete cst;
	//emits the data changed signal
	Engine::chordTable()->emitChordTableChangedSignal();
}

void Chord::parseString( QString _string )
{
	//clears the vector;
	this->clear();
	//elaborates the input string
	QStringList l = _string.remove( ' ' ).split( ';' );
	foreach ( QString s, l )
	{
		if ( s.isEmpty() )
		{ // to eliminate the eventual empty QString derived from the last delimiter
			break;
		}
		// reads the data into semitone and pushes it into the vector
		ChordSemiTone *cst = new ChordSemiTone( this );
		push_back( cst );
	}
}

/*****************************************************************************************************
 *
 * The ChordTable class
 *
******************************************************************************************************/
ChordTable::ChordTable( Model * _parent ) :
	Model( _parent )
{
	//reads the preset original data, emits the data changed signal
loadFactoryPreset();
//	readXML();
}

void ChordTable::saveSettings( QDomDocument &_doc, QDomElement &_parent )
{
	Chord * chord;
	for( int i = 0; i < this->size(); i++ )
	{
		chord=this->at( i );
		QDomElement chord_element = _doc.createElement( QString( "chord" ) );
		_parent.appendChild( chord_element );
		chord->saveSettings( _doc, chord_element );
	}
}

void ChordTable::loadSettings( const QDomElement &_this )
{

	int i = 0;

	Chord * chord;

	//getting the first chordsemitone data
	QDomNode node = _this.firstChild();
	while ( !node.isNull() )
	{
		QDomElement chord_element = node.toElement();

		//if the vector is empty creates new chord, otherwise it uses the existing one.
		if ( i < this->size() )
		{
			chord = at( i );
			chord->loadSettings( chord_element );
		} else
		{
			chord = new Chord( this );
			chord->loadSettings( chord_element );
			push_back( chord );
		}
		i++;
		node = node.nextSibling();
	}
	//removing the extra chords if they are present
	if ( i<size() )
	{
		for ( int j = size()-1; j >= i; j-- )
		{
			chord = at( j );
			remove( j );
			delete chord;
		}
	}
	//emits the data changed signal
	emit chordTableChanged();
	//emits the chordTable names are changed
	emit chordNameChanged();

}

bool ChordTable::readXML()
{
	ConfigManager * confMgr = ConfigManager::inst();
	//Path to the presets file
	QString m_path= confMgr->factoryPresetsDir() + "ChordTable/arpeggio.xpf";

	//The xml document
	QDomDocument m_doc;

	//The xml file itself
	QFile file( m_path );

	//the lists of chords and keys
	QDomNodeList m_chords_list, m_key_list;

	//The xml chord and single key nodes
	QDomNode m_chords_node, m_keys_node, m_key_node;

	//The xml single element
	QDomElement m_chord_element, m_key_element;

	//placeholders for name and sngle key texts
	QString m_nameString, m_keyString;

	//placeholder for the chord semitone
	ChordSemiTone * m_semitone;

	//The single chord
	Chord * m_chord;

	//Check for file
	if ( !file.open( QIODevice::ReadOnly ) || !m_doc.setContent( &file ) )
	{
		return false;
	}

	//Getting the list of chords available in the object
	m_chords_list = m_doc.elementsByTagName( "chord" );
	//for each row of chords
	for ( int i = 0; i < m_chords_list.size(); i++ )
	{
		//getting the single chord node
		m_chords_node = m_chords_list.item( i );
		//getting the "name" element
		m_chord_element = m_chords_node.firstChildElement( "name" );
		m_nameString=m_chord_element.text();
		//and the and the node representing the key sequence of the chord
		m_keys_node = m_chords_node.firstChildElement( "keys" );
		//getting the keys inside the element as a list
		m_key_list = m_keys_node.childNodes();

		//creating the new chord
		m_chord = new Chord( this, m_nameString );

		//processing the keys
		for ( int i = 0; i < m_key_list.size(); i++ )
		{
			//getting the single key node
			m_key_node = m_key_list.item( i );
			//and its element
			m_key_element = m_key_node.toElement();
			//and the string of the chord
			m_keyString = m_key_element.text();
			//initializing the single semitone from the key
			m_semitone = new ChordSemiTone( m_chord, m_keyString );
			//pushing it to the semitones vector
			m_chord->push_back( m_semitone );
		}

		//adding it to the chordtable structure
		push_back( m_chord );
	}
	return true;
}


void ChordTable::loadFactoryPreset()
{
	ConfigManager* confMgr = ConfigManager::inst();

	QString m_path = confMgr->factoryPresetsDir() + "ChordTable/ChordTable.ctd";

	DataFile dataFile( m_path );

	//already emits the data changed signal
	loadSettings( dataFile.content() );
}

void ChordTable::cloneChord( int i )
{
	Chord * cst;
	if ( i == -1 || i > size() )
	{ //adding a new Chord
		cst = new Chord( this );
		cst->m_name = tr( "New Chord" );
		cst->addSemiTone();
	} else
	{ //cloning the Chord
		cst = new Chord( at( i ), at( i )->m_name + " " + tr( "copy" ) );
	}
	push_back( cst );
	//emits the data changed signal
	emit chordTableChanged();
	//emits the chordTable names are changed
	emit chordNameChanged();
}

void ChordTable::removeChord( int i )
{
	//removes chord from the vector
	if ( i < size() )
	{
		Chord * c = at( i );
		remove( i );
		delete c;
		//emits the data changed signal
		emit chordTableChanged();
		//emits the chordTable names are changed
		emit chordNameChanged();
	}
}

const Chord & ChordTable::getByName( const QString & name, bool is_scale ) const
{
	for( int i = 0; i < size(); i++ )
	{
		if( at( i )->getName() == name && is_scale == at( i )->isScale() )
			return * at( i );
	}

	static Chord empty( NULL );
	return empty;
}


//loads the factory preset
void ChordTable::reset()
{
	loadFactoryPreset();
}
