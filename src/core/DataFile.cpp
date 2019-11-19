/*
 * DataFile.cpp - implementation of class DataFile
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2012-2013 Paul Giblock    <p/at/pgiblock.net>
 *
 * This file is part of LMMS - https://lmms.io
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


#include "DataFile.h"

#include <math.h>

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

#include "base64.h"
#include "ConfigManager.h"
#include "Effect.h"
#include "embed.h"
#include "GuiApplication.h"
#include "LocaleHelper.h"
#include "PluginFactory.h"
#include "ProjectVersion.h"
#include "SongEditor.h"
#include "TextFloat.h"

#include "lmmsversion.h"

static void findIds(const QDomElement& elem, QList<jo_id_t>& idList);




DataFile::typeDescStruct
		DataFile::s_types[DataFile::TypeCount] =
{
	{ DataFile::UnknownType, "unknown" },
	{ DataFile::SongProject, "song" },
	{ DataFile::SongProjectTemplate, "songtemplate" },
	{ DataFile::InstrumentTrackSettings, "instrumenttracksettings" },
	{ DataFile::DragNDropData, "dnddata" },
	{ DataFile::ClipboardData, "clipboard-data" },
	{ DataFile::JournalData, "journaldata" },
	{ DataFile::EffectSettings, "effectsettings" }
} ;




DataFile::DataFile( Type type ) :
	QDomDocument( "lmms-project" ),
	m_content(),
	m_head(),
	m_type( type )
{
	appendChild( createProcessingInstruction("xml", "version=\"1.0\""));
	QDomElement root = createElement( "lmms-project" );
	root.setAttribute( "version", LDF_VERSION_STRING );
	root.setAttribute( "type", typeName( type ) );
	root.setAttribute( "creator", "LMMS" );
	root.setAttribute( "creatorversion", LMMS_VERSION );
	appendChild( root );

	m_head = createElement( "head" );
	root.appendChild( m_head );

	m_content = createElement( typeName( type ) );
	root.appendChild( m_content );

}




DataFile::DataFile( const QString & _fileName ) :
	QDomDocument(),
	m_content(),
	m_head()
{
	QFile inFile( _fileName );
	if( !inFile.open( QIODevice::ReadOnly ) )
	{
		if( gui )
		{
			QMessageBox::critical( NULL,
				SongEditor::tr( "Could not open file" ),
				SongEditor::tr( "Could not open file %1. You probably "
						"have no permissions to read this "
						"file.\n Please make sure to have at "
						"least read permissions to the file "
						"and try again." ).arg( _fileName ) );
		}

		return;
	}

	loadData( inFile.readAll(), _fileName );
}




DataFile::DataFile( const QByteArray & _data ) :
	QDomDocument(),
	m_content(),
	m_head()
{
	loadData( _data, "<internal data>" );
}




DataFile::~DataFile()
{
}




bool DataFile::validate( QString extension )
{
	switch( m_type )
	{
	case Type::SongProject:
		if( extension == "mmp" || extension == "mmpz" )
		{
			return true;
		}
		break;
	case Type::SongProjectTemplate:
		if(  extension == "mpt" )
		{
			return true;
		}
		break;
	case Type::InstrumentTrackSettings:
		if ( extension == "xpf" || extension == "xml" )
		{
			return true;
		}
		break;
	case Type::UnknownType:
		if (! ( extension == "mmp" || extension == "mpt" || extension == "mmpz" ||
				extension == "xpf" || extension == "xml" ||
				( extension == "xiz" && ! pluginFactory->pluginSupportingExtension(extension).isNull()) ||
				extension == "sf2" || extension == "sf3" || extension == "pat" || extension == "mid" ||
				extension == "dll"
				) )
		{
			return true;
		}
		if( extension == "wav" || extension == "ogg" ||
				extension == "ds" )
		{
			return true;
		}
		break;
	default:
		return false;
	}
	return false;
}




QString DataFile::nameWithExtension( const QString & _fn ) const
{
	switch( type() )
	{
		case SongProject:
			if( _fn.section( '.', -1 ) != "mmp" &&
					_fn.section( '.', -1 ) != "mpt" &&
					_fn.section( '.', -1 ) != "mmpz" )
			{
				if( ConfigManager::inst()->value( "app",
						"nommpz" ).toInt() == 0 )
				{
					return _fn + ".mmpz";
				}
				return _fn + ".mmp";
			}
			break;
		case SongProjectTemplate:
			if( _fn.section( '.',-1 ) != "mpt" )
			{
				return _fn + ".mpt";
			}
			break;
		case InstrumentTrackSettings:
			if( _fn.section( '.', -1 ) != "xpf" )
			{
				return _fn + ".xpf";
			}
			break;
		default: ;
	}
	return _fn;
}




void DataFile::write( QTextStream & _strm )
{
	if( type() == SongProject || type() == SongProjectTemplate
					|| type() == InstrumentTrackSettings )
	{
		cleanMetaNodes( documentElement() );
	}

	save(_strm, 2);
}




bool DataFile::writeFile( const QString& filename )
{
	const QString fullName = nameWithExtension( filename );
	const QString fullNameTemp = fullName + ".new";
	const QString fullNameBak = fullName + ".bak";

	QFile outfile( fullNameTemp );

	if( !outfile.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
	{
		if( gui )
		{
			QMessageBox::critical( NULL,
				SongEditor::tr( "Could not write file" ),
				SongEditor::tr( "Could not open %1 for writing. You probably are not permitted to "
								"write to this file. Please make sure you have write-access to "
								"the file and try again." ).arg( fullName ) );
		}

		return false;
	}

	if( fullName.section( '.', -1 ) == "mmpz" )
	{
		QString xml;
		QTextStream ts( &xml );
		write( ts );
		outfile.write( qCompress( xml.toUtf8() ) );
	}
	else
	{
		QTextStream ts( &outfile );
		write( ts );
	}

	outfile.close();

	// make sure the file has been written correctly
	if( QFileInfo( outfile.fileName() ).size() > 0 )
	{
		if( ConfigManager::inst()->value( "app", "disablebackup" ).toInt() )
		{
			// remove current file
			QFile::remove( fullName );
		}
		else
		{
			// remove old backup file
			QFile::remove( fullNameBak );
			// move current file to backup file
			QFile::rename( fullName, fullNameBak );
		}
		// move temporary file to current file
		QFile::rename( fullNameTemp, fullName );

		return true;
	}

	return false;
}




DataFile::Type DataFile::type( const QString& typeName )
{
	for( int i = 0; i < TypeCount; ++i )
	{
		if( s_types[i].m_name == typeName )
		{
			return static_cast<DataFile::Type>( i );
		}
	}

	// compat code
	if( typeName == "channelsettings" )
	{
		return DataFile::InstrumentTrackSettings;
	}

	return UnknownType;
}




QString DataFile::typeName( Type type )
{
	if( type >= UnknownType && type < TypeCount )
	{
		return s_types[type].m_name;
	}

	return s_types[UnknownType].m_name;
}




void DataFile::cleanMetaNodes( QDomElement _de )
{
	QDomNode node = _de.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( node.toElement().attribute( "metadata" ).toInt() )
			{
				QDomNode ns = node.nextSibling();
				_de.removeChild( node );
				node = ns;
				continue;
			}
			if( node.hasChildNodes() )
			{
				cleanMetaNodes( node.toElement() );
			}
		}
		node = node.nextSibling();
	}
}


void DataFile::upgrade_0_2_1_20070501()
{
	// Upgrade to version 0.2.1-20070501
	QDomNodeList list = elementsByTagName( "arpandchords" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		if( el.hasAttribute( "arpdir" ) )
		{
			int arpdir = el.attribute( "arpdir" ).toInt();
			if( arpdir > 0 )
			{
				el.setAttribute( "arpdir", arpdir - 1 );
			}
			else
			{
				el.setAttribute( "arpdisabled", "1" );
			}
		}
	}

	list = elementsByTagName( "sampletrack" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		if( el.attribute( "vol" ) != "" )
		{
			el.setAttribute( "vol", LocaleHelper::toFloat(
					el.attribute( "vol" ) ) * 100.0f );
		}
		else
		{
			QDomNode node = el.namedItem(
						"automation-pattern" );
			if( !node.isElement() ||
				!node.namedItem( "vol" ).isElement() )
			{
				el.setAttribute( "vol", 100.0f );
			}
		}
	}

	list = elementsByTagName( "ladspacontrols" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		QDomNode anode = el.namedItem( "automation-pattern" );
		QDomNode node = anode.firstChild();
		while( !node.isNull() )
		{
			if( node.isElement() )
			{
				QString name = node.nodeName();
				if( name.endsWith( "link" ) )
				{
					el.setAttribute( name,
						node.namedItem( "time" )
						.toElement()
						.attribute( "value" ) );
					QDomNode oldNode = node;
					node = node.nextSibling();
					anode.removeChild( oldNode );
					continue;
				}
			}
			node = node.nextSibling();
		}
	}

	QDomNode node = m_head.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( node.nodeName() == "bpm" )
			{
				int value = node.toElement().attribute(
						"value" ).toInt();
				if( value > 0 )
				{
					m_head.setAttribute( "bpm",
								value );
					QDomNode oldNode = node;
					node = node.nextSibling();
					m_head.removeChild( oldNode );
					continue;
				}
			}
			else if( node.nodeName() == "mastervol" )
			{
				int value = node.toElement().attribute(
						"value" ).toInt();
				if( value > 0 )
				{
					m_head.setAttribute(
						"mastervol", value );
					QDomNode oldNode = node;
					node = node.nextSibling();
					m_head.removeChild( oldNode );
					continue;
				}
			}
			else if( node.nodeName() == "masterpitch" )
			{
				m_head.setAttribute( "masterpitch",
					-node.toElement().attribute(
						"value" ).toInt() );
				QDomNode oldNode = node;
				node = node.nextSibling();
				m_head.removeChild( oldNode );
				continue;
			}
		}
		node = node.nextSibling();
	}
}


void DataFile::upgrade_0_2_1_20070508()
{
	// Upgrade to version 0.2.1-20070508 from some version greater than or equal to 0.2.1-20070501
	QDomNodeList list = elementsByTagName( "arpandchords" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		if( el.hasAttribute( "chorddisabled" ) )
		{
			el.setAttribute( "chord-enabled",
				!el.attribute( "chorddisabled" )
							.toInt() );
			el.setAttribute( "arp-enabled",
				!el.attribute( "arpdisabled" )
							.toInt() );
		}
		else if( !el.hasAttribute( "chord-enabled" ) )
		{
			el.setAttribute( "chord-enabled", true );
			el.setAttribute( "arp-enabled",
				el.attribute( "arpdir" ).toInt() != 0 );
		}
	}

	while( !( list = elementsByTagName( "channeltrack" ) ).isEmpty() )
	{
		QDomElement el = list.item( 0 ).toElement();
		el.setTagName( "instrumenttrack" );
	}

	list = elementsByTagName( "instrumenttrack" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		if( el.hasAttribute( "vol" ) )
		{
			float value = LocaleHelper::toFloat( el.attribute( "vol" ) );
			value = roundf( value * 0.585786438f );
			el.setAttribute( "vol", value );
		}
		else
		{
			QDomNodeList vol_list = el.namedItem(
						"automation-pattern" )
					.namedItem( "vol" ).toElement()
					.elementsByTagName( "time" );
			for( int j = 0; !vol_list.item( j ).isNull();
								++j )
			{
				QDomElement timeEl = list.item( j )
							.toElement();
				int value = timeEl.attribute( "value" )
							.toInt();
				value = (int)roundf( value *
							0.585786438f );
				timeEl.setAttribute( "value", value );
			}
		}
	}
}


void DataFile::upgrade_0_3_0_rc2()
{
	// Upgrade to version 0.3.0-rc2 from some version greater than or equal to 0.2.1-20070508
	QDomNodeList list = elementsByTagName( "arpandchords" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		if( el.attribute( "arpdir" ).toInt() > 0 )
		{
			el.setAttribute( "arpdir",
				el.attribute( "arpdir" ).toInt() - 1 );
		}
	}
}


void DataFile::upgrade_0_3_0()
{
	// Upgrade to version 0.3.0 (final) from some version greater than or equal to 0.3.0-rc2
	QDomNodeList list;
	while( !( list = elementsByTagName(
				"pluckedstringsynth" ) ).isEmpty() )
	{
		QDomElement el = list.item( 0 ).toElement();
		el.setTagName( "vibedstrings" );
		el.setAttribute( "active0", 1 );
	}

	while( !( list = elementsByTagName( "lb303" ) ).isEmpty() )
	{
		QDomElement el = list.item( 0 ).toElement();
		el.setTagName( "lb302" );
	}

	while( !( list = elementsByTagName( "channelsettings" ) ).
							isEmpty() )
	{
		QDomElement el = list.item( 0 ).toElement();
		el.setTagName( "instrumenttracksettings" );
	}
}


void DataFile::upgrade_0_4_0_20080104()
{
	// Upgrade to version 0.4.0-20080104 from some version greater than or equal to 0.3.0 (final)
	QDomNodeList list = elementsByTagName( "fx" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		if( el.hasAttribute( "fxdisabled" ) &&
			el.attribute( "fxdisabled" ).toInt() == 0 )
		{
			el.setAttribute( "enabled", 1 );
		}
	}
}


void DataFile::upgrade_0_4_0_20080118()
{
	// Upgrade to version 0.4.0-20080118 from some version greater than or equal to 0.4.0-20080104
	QDomNodeList list;
	while( !( list = elementsByTagName( "fx" ) ).isEmpty() )
	{
		QDomElement fxchain = list.item( 0 ).toElement();
		fxchain.setTagName( "fxchain" );
		QDomNode rack = list.item( 0 ).firstChild();
		QDomNodeList effects = rack.childNodes();
		// move items one level up
		while( effects.count() )
		{
			fxchain.appendChild( effects.at( 0 ) );
		}
		fxchain.setAttribute( "numofeffects",
			rack.toElement().attribute( "numofeffects" ) );
		fxchain.removeChild( rack );
	}
}


void DataFile::upgrade_0_4_0_20080129()
{
	// Upgrade to version 0.4.0-20080129 from some version greater than or equal to 0.4.0-20080118
	QDomNodeList list;
	while( !( list =
		elementsByTagName( "arpandchords" ) ).isEmpty() )
	{
		QDomElement aac = list.item( 0 ).toElement();
		aac.setTagName( "arpeggiator" );
		QDomNode cloned = aac.cloneNode();
		cloned.toElement().setTagName( "chordcreator" );
		aac.parentNode().appendChild( cloned );
	}
}


void DataFile::upgrade_0_4_0_20080409()
{
	// Upgrade to version 0.4.0-20080409 from some version greater than or equal to 0.4.0-20080129
	QStringList s;
	s << "note" << "pattern" << "bbtco" << "sampletco" << "time";
	for( QStringList::iterator it = s.begin(); it < s.end(); ++it )
	{
		QDomNodeList list = elementsByTagName( *it );
		for( int i = 0; !list.item( i ).isNull(); ++i )
		{
			QDomElement el = list.item( i ).toElement();
			el.setAttribute( "pos",
				el.attribute( "pos" ).toInt()*3 );
			el.setAttribute( "len",
				el.attribute( "len" ).toInt()*3 );
		}
	}
	QDomNodeList list = elementsByTagName( "timeline" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		el.setAttribute( "lp0pos",
			el.attribute( "lp0pos" ).toInt()*3 );
		el.setAttribute( "lp1pos",
			el.attribute( "lp1pos" ).toInt()*3 );
	}
}


void DataFile::upgrade_0_4_0_20080607()
{
	// Upgrade to version 0.4.0-20080607 from some version greater than or equal to 0.3.0-20080409
	QDomNodeList list;
	while( !( list = elementsByTagName( "midi" ) ).isEmpty() )
	{
		QDomElement el = list.item( 0 ).toElement();
		el.setTagName( "midiport" );
	}
}


void DataFile::upgrade_0_4_0_20080622()
{
	// Upgrade to version 0.4.0-20080622 from some version greater than or equal to 0.3.0-20080607
	QDomNodeList list;
	while( !( list = elementsByTagName(
				"automation-pattern" ) ).isEmpty() )
	{
		QDomElement el = list.item( 0 ).toElement();
		el.setTagName( "automationpattern" );
	}

	list = elementsByTagName( "bbtrack" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		QString s = el.attribute( "name" );
		s.replace( QRegExp( "^Beat/Baseline " ),
						"Beat/Bassline " );
		el.setAttribute( "name", s );
	}
}


void DataFile::upgrade_0_4_0_beta1()
{
	// Upgrade to version 0.4.0-beta1 from some version greater than or equal to 0.4.0-20080622
	// convert binary effect-key-blobs to XML
	QDomNodeList list;
	list = elementsByTagName( "effect" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		QString k = el.attribute( "key" );
		if( !k.isEmpty() )
		{
			const QList<QVariant> l =
				base64::decode( k, QVariant::List ).toList();
			if( !l.isEmpty() )
			{
				QString name = l[0].toString();
				QVariant u = l[1];
				EffectKey::AttributeMap m;
				// VST-effect?
				if( u.type() == QVariant::String )
				{
					m["file"] = u.toString();
				}
				// LADSPA-effect?
				else if( u.type() == QVariant::StringList )
				{
					const QStringList sl = u.toStringList();
					m["plugin"] = sl.value( 0 );
					m["file"] = sl.value( 1 );
				}
				EffectKey key( NULL, name, m );
				el.appendChild( key.saveXML( *this ) );
			}
		}
	}
}


void DataFile::upgrade_0_4_0_rc2()
{
	// Upgrade to version 0.4.0-rc2 from some version greater than or equal to 0.4.0-beta1
	QDomNodeList list = elementsByTagName( "audiofileprocessor" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		QString s = el.attribute( "src" );
		s.replace( "drumsynth/misc ", "drumsynth/misc_" );
		s.replace( "drumsynth/r&b", "drumsynth/r_n_b" );
		s.replace( "drumsynth/r_b", "drumsynth/r_n_b" );
		el.setAttribute( "src", s );
	}
	list = elementsByTagName( "lb302" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		int s = el.attribute( "shape" ).toInt();
		if( s >= 1 )
		{
			s--;
		}
		el.setAttribute( "shape", QString("%1").arg(s) );
	}
}


void DataFile::upgrade_1_0_99()
{
	jo_id_t last_assigned_id = 0;
	
	QList<jo_id_t> idList;
	findIds(documentElement(), idList);
	
	QDomNodeList list = elementsByTagName("ladspacontrols");
	for(int i = 0; !list.item(i).isNull(); ++i)
	{
		for(QDomNode node = list.item(i).firstChild(); !node.isNull();
			node = node.nextSibling())
		{
			QDomElement el = node.toElement();
			QDomNode data_child = el.namedItem("data");
			if(!data_child.isElement())
			{
				if (el.attribute("scale_type") == "log")
				{
					QDomElement me = createElement("data");
					me.setAttribute("value", el.attribute("data"));
					me.setAttribute("scale_type", "log");
					
					jo_id_t id;
					for(id = last_assigned_id + 1;
						idList.contains(id); id++)
					{
					}
					
					last_assigned_id = id;
					idList.append(id);
					me.setAttribute("id", id);
					el.appendChild(me);
					
				}
			}
		}
	}	
}


void DataFile::upgrade_1_1_0()
{
	QDomNodeList list = elementsByTagName("fxchannel");
	for (int i = 1; !list.item(i).isNull(); ++i)
	{
		QDomElement el = list.item(i).toElement();
		QDomElement send = createElement("send");
		send.setAttribute("channel", "0");
		send.setAttribute("amount", "1");
		el.appendChild(send);
	}
}


void DataFile::upgrade_1_1_91()
{
	// Upgrade to version 1.1.91 from some version less than 1.1.91
	QDomNodeList list = elementsByTagName( "audiofileprocessor" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		QString s = el.attribute( "src" );
		s.replace( QRegExp("/samples/bassloopes/"), "/samples/bassloops/" );
		el.setAttribute( "src", s );
	}

	list = elementsByTagName( "attribute" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		if( el.attribute( "name" ) == "plugin" && el.attribute( "value" ) == "vocoder-lmms" ) {
			el.setAttribute( "value", "vocoder" );
		}
	}

	list = elementsByTagName( "crossoevereqcontrols" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		// invert the mute LEDs
		for( int j = 1; j <= 4; ++j ){
			QString a = QString( "mute%1" ).arg( j );
			el.setAttribute( a, ( el.attribute( a ) == "0" ) ? "1" : "0" );
		}
	}

	list = elementsByTagName( "arpeggiator" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		// Swap elements ArpDirRandom and ArpDirDownAndUp
		if( el.attribute( "arpdir" ) == "3" )
		{
			el.setAttribute( "arpdir", "4" );
		}
		else if( el.attribute( "arpdir" ) == "4" )
		{
			el.setAttribute( "arpdir", "3" );
		}
	}
}


void DataFile::upgrade_1_2_0_rc3()
{
	// Upgrade from earlier bbtrack beat note behaviour of adding
	// steps if a note is placed after the last step.
	QDomNodeList bbtracks = elementsByTagName( "bbtrack" );
	for( int i = 0; !bbtracks.item( i ).isNull(); ++i )
	{
		QDomNodeList patterns = bbtracks.item( i
				).toElement().elementsByTagName(
								"pattern" );
		for( int j = 0; !patterns.item( j ).isNull(); ++j )
		{
			int patternLength, steps;
			QDomElement el = patterns.item( j ).toElement();
			if( el.attribute( "len" ) != "" )
			{
				patternLength = el.attribute( "len" ).toInt();
				steps = patternLength / 12;
				el.setAttribute( "steps", steps );
			}
		}
	}
}


static void upgradeElement_1_2_0_rc2_42( QDomElement & el )
{
	if( el.hasAttribute( "syncmode" ) )
	{
		int syncmode = el.attribute( "syncmode" ).toInt();
		QStringList names;
		QDomNamedNodeMap atts = el.attributes();
		for( uint i = 0; i < atts.length(); i++ )
		{
			QString name = atts.item( i ).nodeName();
			if( name.endsWith( "_numerator" ) )
			{
				names << name.remove( "_numerator" )
								+ "_syncmode";
			}
		}
		for( QStringList::iterator it = names.begin(); it < names.end();
									++it )
		{
			el.setAttribute( *it, syncmode );
		}
	}

	QDomElement child = el.firstChildElement();
	while ( !child.isNull() )
	{
		upgradeElement_1_2_0_rc2_42( child );
		child = child.nextSiblingElement();
	}
}


void DataFile::upgrade_1_2_0_rc2_42()
{
	QDomElement el = firstChildElement();
	while ( !el.isNull() )
	{
		upgradeElement_1_2_0_rc2_42( el );
		el = el.nextSiblingElement();
	}
}


/**
 *  Helper function to call a functor for all effect ports' DomElements,
 *  providing the functor with lists to add and remove DomElements. Helpful for
 *  patching port values from savefiles.
 */
template<class Ftor>
void iterate_ladspa_ports(QDomElement& effect, Ftor& ftor)
{
	// Head back up the DOM to upgrade ports
	QDomNodeList ladspacontrols = effect.elementsByTagName( "ladspacontrols" );
	for( int m = 0; !ladspacontrols.item( m ).isNull(); ++m )
	{
		QList<QDomElement> addList, removeList;
		QDomElement ladspacontrol = ladspacontrols.item( m ).toElement();
		for( QDomElement port = ladspacontrol.firstChild().toElement();
			!port.isNull(); port = port.nextSibling().toElement() )
		{
			QStringList parts = port.tagName().split("port");
			// Not a "port"
			if ( parts.size() < 2 )
			{
				continue;
			}
			int num = parts[1].toInt();

			// From Qt's docs of QDomNode:
			// * copying a QDomNode is OK, they still have the same
			//   pointer to the "internal" QDomNodePrivate.
			// * Also, they are using linked lists, which means
			//   deleting or appending QDomNode does not invalidate
			//   any other pointers.
			// => Inside ftor, you can (and should) push back the
			//    QDomElements by value, not references
			// => The loops below for adding and removing don't
			//    invalidate any other QDomElements
			ftor(port, num, addList, removeList);
		}

		// Add ports marked for adding
		for ( QDomElement e : addList )
		{
			ladspacontrol.appendChild( e );
		}
		// Remove ports marked for removal
		for ( QDomElement e : removeList )
		{
			ladspacontrol.removeChild( e );
		}
	}
}

// helper function if you need to print a QDomNode
QDebug operator<<(QDebug dbg, const QDomNode& node)
{
	QString s;
	QTextStream str(&s, QIODevice::WriteOnly);
	node.save(str, 2);
	dbg << qPrintable(s);
	return dbg;
}

void DataFile::upgrade_1_3_0()
{
	QDomNodeList list = elementsByTagName( "instrument" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement el = list.item( i ).toElement();
		if( el.attribute( "name" ) == "papu" )
		{
			el.setAttribute( "name", "freeboy" );
			QDomNodeList children = el.elementsByTagName( "papu" );
			for( int j = 0; !children.item( j ).isNull(); ++j )
			{
				QDomElement child = children.item( j ).toElement();
				child.setTagName( "freeboy" );
			}
		}
		else if( el.attribute( "name" ) == "OPL2" )
		{
			el.setAttribute( "name", "opulenz" );
			QDomNodeList children = el.elementsByTagName( "OPL2" );
			for( int j = 0; !children.item( j ).isNull(); ++j )
			{
				QDomElement child = children.item( j ).toElement();
				child.setTagName( "opulenz" );
			}
		}
	}

	list = elementsByTagName( "effect" );
	for( int i = 0; !list.item( i ).isNull(); ++i )
	{
		QDomElement effect = list.item( i ).toElement();
		if( effect.attribute( "name" ) == "ladspaeffect" )
		{
			QDomNodeList keys = effect.elementsByTagName( "key" );
			for( int j = 0; !keys.item( j ).isNull(); ++j )
			{
				QDomElement key = keys.item( j ).toElement();
				QDomNodeList attributes = key.elementsByTagName( "attribute" );
				for( int k = 0; !attributes.item( k ).isNull(); ++k )
				{
					// Effect name changes

					QDomElement attribute = attributes.item( k ).toElement();
					if( attribute.attribute( "name" ) == "file" &&
							( attribute.attribute( "value" ) == "calf" || 
							attribute.attribute( "value" ) == "calf.so" ) )
					{
						attribute.setAttribute( "value", "veal" );
					}
					else if( attribute.attribute( "name" ) == "plugin" &&
							attribute.attribute( "value" ) == "Sidechaincompressor" )
					{
						attribute.setAttribute( "value", "SidechainCompressor" );
					}
					else if( attribute.attribute( "name" ) == "plugin" &&
							attribute.attribute( "value" ) == "Sidechaingate" )
					{
						attribute.setAttribute( "value", "SidechainGate" );
					}
					else if( attribute.attribute( "name" ) == "plugin" &&
							attribute.attribute( "value" ) == "Multibandcompressor" )
					{
						attribute.setAttribute( "value", "MultibandCompressor" );
					}
					else if( attribute.attribute( "name" ) == "plugin" &&
							attribute.attribute( "value" ) == "Multibandgate" )
					{
						attribute.setAttribute( "value", "MultibandGate" );
					}
					else if( attribute.attribute( "name" ) == "plugin" &&
							attribute.attribute( "value" ) == "Multibandlimiter" )
					{
						attribute.setAttribute( "value", "MultibandLimiter" );
					}

					// Handle port changes

					if( attribute.attribute( "name" ) == "plugin" &&
							( attribute.attribute( "value" ) == "MultibandLimiter" ||
							attribute.attribute( "value" ) == "MultibandCompressor" ||
							attribute.attribute( "value" ) == "MultibandGate" ) )
					{
						auto fn = [&](QDomElement& port, int num, QList<QDomElement>&, QList<QDomElement>& removeList)
						{
							// Mark ports for removal
							if ( num >= 18 && num <= 23 )
							{
								removeList << port;
							}
							// Bump higher ports up 6 positions
							else if ( num >= 24 )
							{
								// port01...port010, etc
								QString name( "port0" );
								name.append( QString::number( num -6 ) );
								port.setTagName( name );
							}
						};
						iterate_ladspa_ports(effect, fn);
					}

					if( attribute.attribute( "name" ) == "plugin" &&
							( attribute.attribute( "value" ) == "Pulsator" ) )
					{
						auto fn = [&](QDomElement& port, int num, QList<QDomElement>& addList, QList<QDomElement>& removeList)
						{
							switch(num)
							{
								case 16:
								{
									// old freq is now at port 25
									QDomElement portCopy = createElement("port025");
									portCopy.setAttribute("data", port.attribute("data"));
									addList << portCopy;
									// remove old freq port
									removeList << port;
									// set the "timing" port to choose port23+2=port25 (timing in Hz)
									QDomElement timing = createElement("port022");
									timing.setAttribute("data", 2);
									addList << timing;
									break;
								}
								// port 18 (modulation) => 17
								case 17:
									port.setTagName("port016");
									break;
								case 18:
								{
									// leave port 18 (offsetr), but add port 17 (offsetl)
									QDomElement offsetl = createElement("port017");
									offsetl.setAttribute("data", 0.0f);
									addList << offsetl;
									// additional: bash port 21 to 1
									QDomElement pulsewidth = createElement("port021");
									pulsewidth.setAttribute("data", 1.0f);
									addList << pulsewidth;
									break;
								}
							}


						};
						iterate_ladspa_ports(effect, fn);
					}


					if( attribute.attribute( "name" ) == "plugin" &&
							( attribute.attribute( "value" ) == "VintageDelay" ) )
					{
						auto fn = [&](QDomElement& port, int num, QList<QDomElement>& addList, QList<QDomElement>& )
						{
							switch(num)
							{
								case 4:
								{
									// BPM is now port028
									port.setTagName("port028");
									// bash timing to BPM
									QDomElement timing = createElement("port027");
									timing.setAttribute("data", 0);
									addList << timing;

									// port 5 and 6 (in, out gain) need to be bashed to 1:
									QDomElement input = createElement("port05");
									input.setAttribute("data", 1.0f);
									addList << input;
									QDomElement output = createElement("port06");
									output.setAttribute("data", 1.0f);
									addList << output;

									break;
								}
								default:
									// all other ports increase by 10
									QString name( "port0" );
									name.append( QString::number( num + 10 ) );
									port.setTagName( name );
							}


						};
						iterate_ladspa_ports(effect, fn);
					}

					if( attribute.attribute( "name" ) == "plugin" &&
						(	   ( attribute.attribute( "value" ) == "Equalizer5Band" )
							|| ( attribute.attribute( "value" ) == "Equalizer8Band" )
							|| ( attribute.attribute( "value" ) == "Equalizer12Band" ) ) )
					{
						// NBand equalizers got 4 q nobs inserted. We need to shift everything else...
						// HOWEVER: 5 band eq has only 2 q nobs inserted (no LS/HS filters)
						bool band5 = ( attribute.attribute( "value" ) == "Equalizer5Band" );
						auto fn = [&](QDomElement& port, int num, QList<QDomElement>& addList, QList<QDomElement>& )
						{
							if(num == 4)
							{
								// don't modify port 4, but some other ones:
								int zoom_port;
								if(attribute.attribute( "value" ) == "Equalizer5Band")
									zoom_port = 36;
								else if(attribute.attribute( "value" ) == "Equalizer8Band")
									zoom_port = 48;
								else // 12 band
									zoom_port = 64;
								// bash zoom to 0.25
								QString name( "port0" );
								name.append( QString::number( zoom_port ) );
								QDomElement timing = createElement(name);
								timing.setAttribute("data", 0.25f);
								addList << timing;
							}
							// the following code could be refactored, but I did careful code-reading
							// to prevent copy-paste-errors
							if(num == 18)
							{
								// 18 => 19
								port.setTagName("port019");
								// insert port 18 (q)
								QDomElement q = createElement("port018");
								q.setAttribute("data", 0.707f);
								addList << q;
							}
							else if(num >= 19 && num <= 20)
							{
								// num += 1
								QString name( "port0" );
								name.append( QString::number( num + 1 ) );
								port.setTagName( name );
							}
							else if(num == 21)
							{
								// 21 => 23
								port.setTagName("port023");
								// insert port 22 (q)
								QDomElement q = createElement("port022");
								q.setAttribute("data", 0.707f);
								addList << q;
							}
							else if(num >= 22 && (num <= 23 || band5))
							{
								// num += 2
								QString name( "port0" );
								name.append( QString::number( num + 2 ) );
								port.setTagName( name );
							}
							else if(num == 24 && !band5)
							{
								// 24 => 27
								port.setTagName("port027");
								// insert port 26 (q)
								QDomElement q = createElement("port026");
								q.setAttribute("data", 0.707f);
								addList << q;
							}
							else if(num >= 25 && num <= 26 && !band5)
							{
								// num += 3
								QString name( "port0" );
								name.append( QString::number( num + 3 ) );
								port.setTagName( name );
							}
							else if(num == 27 && !band5)
							{
								// 27 => 31
								port.setTagName("port031");
								// insert port 30 (q)
								QDomElement q = createElement("port030");
								q.setAttribute("data", 0.707f);
								addList << q;
							}
							else if(num >= 28 && !band5)
							{
								// num += 4
								QString name( "port0" );
								name.append( QString::number( num + 4 ) );
								port.setTagName( name );
							}
						};
						iterate_ladspa_ports(effect, fn);
					}

					if( attribute.attribute( "name" ) == "plugin" &&
						attribute.attribute( "value" ) == "Saturator" )
					{
						auto fn = [&](QDomElement& port, int num, QList<QDomElement>&, QList<QDomElement>& )
						{
							// These ports have been shifted a bit weird...
							if( num == 7 )
							{
								port.setTagName("port015");
							}
							else if(num == 12)
							{
								port.setTagName("port016");
							}
							else if(num == 13)
							{
								port.setTagName("port017");
							}
							else if ( num >= 15 )
							{
								QString name( "port0" );
								name.append( QString::number( num + 3 ) );
								port.setTagName( name );
							}
						};
						iterate_ladspa_ports(effect, fn);
					}
					
					if( attribute.attribute( "name" ) == "plugin" &&
						attribute.attribute( "value" ) == "StereoTools" )
					{
						auto fn = [&](QDomElement& port, int num, QList<QDomElement>&, QList<QDomElement>& )
						{
							// This effect can not be back-ported due to bugs in the old version,
							// or due to different behaviour. We thus port all parameters we can,
							// and bash all new parameters (in this case, s.level and m.level) to
							// their new defaults (both 1.0f in this case)

							if( num == 23 || num == 25 )
							{
								port.setAttribute("data", 1.0f);
							}
						};
						iterate_ladspa_ports(effect, fn);
					}
				}
			}
		}
	}
}


void DataFile::upgrade()
{
	ProjectVersion version =
		documentElement().attribute( "creatorversion" ).
							replace( "svn", "" );

	if( version < "0.2.1-20070501" )
	{
		upgrade_0_2_1_20070501();
	}

	if( version < "0.2.1-20070508" )
	{
		upgrade_0_2_1_20070508();
	}

	if( version < "0.3.0-rc2" )
	{
		upgrade_0_3_0_rc2();
	}

	if( version < "0.3.0" )
	{
		upgrade_0_3_0();
	}

	if( version < "0.4.0-20080104" )
	{
		upgrade_0_4_0_20080104();
	}

	if( version < "0.4.0-20080118" )
	{
		upgrade_0_4_0_20080118();
	}

	if( version < "0.4.0-20080129" )
	{
		upgrade_0_4_0_20080129();
	}

	if( version < "0.4.0-20080409" )
	{
		upgrade_0_4_0_20080409();
	}

	if( version < "0.4.0-20080607" )
	{
		upgrade_0_4_0_20080607();
	}

	if( version < "0.4.0-20080622" )
	{
		upgrade_0_4_0_20080622();
	}

	if( version < "0.4.0-beta1" )
	{
		upgrade_0_4_0_beta1();
	}
	if( version < "0.4.0-rc2" )
	{
		upgrade_0_4_0_rc2();
	}
	if( version < "1.0.99-0" )
	{
		upgrade_1_0_99();
	}
	if( version < "1.1.0-0" )
	{
		upgrade_1_1_0();
	}
	if( version < "1.1.91-0" )
	{
		upgrade_1_1_91();
	}
	if( version < "1.2.0-rc3" )
	{
		upgrade_1_2_0_rc3();
		upgrade_1_2_0_rc2_42();
	}
	if( version < "1.3.0" )
	{
		upgrade_1_3_0();
	}

	// update document meta data
	documentElement().setAttribute( "version", LDF_VERSION_STRING );
	documentElement().setAttribute( "type", typeName( type() ) );
	documentElement().setAttribute( "creator", "LMMS" );
	documentElement().setAttribute( "creatorversion", LMMS_VERSION );

	if( type() == SongProject || type() == SongProjectTemplate )
	{
		// Time-signature
		if ( !m_head.hasAttribute( "timesig_numerator" ) )
		{
			m_head.setAttribute( "timesig_numerator", 4 );
			m_head.setAttribute( "timesig_denominator", 4 );
		}

		if( !m_head.hasAttribute( "mastervol" ) )
		{
			m_head.setAttribute( "mastervol", 100 );
		}
	}
}




void DataFile::loadData( const QByteArray & _data, const QString & _sourceFile )
{
	QString errorMsg;
	int line = -1, col = -1;
	if( !setContent( _data, &errorMsg, &line, &col ) )
	{
		// parsing failed? then try to uncompress data
		QByteArray uncompressed = qUncompress( _data );
		if( !uncompressed.isEmpty() )
		{
			if( setContent( uncompressed, &errorMsg, &line, &col ) )
			{
				line = col = -1;
			}
		}
		if( line >= 0 && col >= 0 )
		{
			qWarning() << "at line" << line << "column" << errorMsg;
			if( gui )
			{
				QMessageBox::critical( NULL,
					SongEditor::tr( "Error in file" ),
					SongEditor::tr( "The file %1 seems to contain "
							"errors and therefore can't be "
							"loaded." ).
								arg( _sourceFile ) );
			}

			return;
		}
	}

	QDomElement root = documentElement();
	m_type = type( root.attribute( "type" ) );
	m_head = root.elementsByTagName( "head" ).item( 0 ).toElement();


	if( root.hasAttribute( "creatorversion" ) )
	{
		// compareType defaults to Build,so it doesn't have to be set here
		ProjectVersion createdWith = root.attribute( "creatorversion" );
		ProjectVersion openedWith = LMMS_VERSION;

		if ( createdWith != openedWith )
		{
			// only one compareType needs to be set, and we can compare on one line because setCompareType returns ProjectVersion
			if( createdWith.setCompareType( ProjectVersion::Minor )
								!= openedWith )
			{
				if( gui != nullptr && root.attribute( "type" ) == "song" )
				{
					TextFloat::displayMessage(
						SongEditor::tr( "Version difference" ),
						SongEditor::tr(
							"This %1 was created with "
							"LMMS %2."
						).arg(
							_sourceFile.endsWith( ".mpt" ) ?
								SongEditor::tr( "template" ) :
								SongEditor::tr( "project" )
						)
						.arg( root.attribute( "creatorversion" ) ),
						embed::getIconPixmap( "whatsthis", 24, 24 ),
						2500
					);
				}
			}

			// the upgrade needs to happen after the warning as it updates the project version.
			if( createdWith.setCompareType( ProjectVersion::Build )
								< openedWith )
			{
				upgrade();
			}
		}
	}

	m_content = root.elementsByTagName( typeName( m_type ) ).
							item( 0 ).toElement();
}


void findIds(const QDomElement& elem, QList<jo_id_t>& idList)
{
	if(elem.hasAttribute("id"))
	{
		idList.append(elem.attribute("id").toInt());
	}
	QDomElement child = elem.firstChildElement();
	while(!child.isNull()) 
	{
		findIds(child, idList);
		child = child.nextSiblingElement();
	}
}
