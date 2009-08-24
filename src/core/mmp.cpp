/*
 * mmp.cpp - implementation of class multimediaProject
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


#include "mmp.h"

#include <math.h>

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtGui/QMessageBox>


#include "config_mgr.h"
#include "project_version.h"
#include "song_editor.h"
#include "Effect.h"
#include "lmmsversion.h"



multimediaProject::typeDescStruct
		multimediaProject::s_types[multimediaProject::NumProjectTypes] =
{
	{ multimediaProject::UnknownType, "unknown" },
	{ multimediaProject::SongProject, "song" },
	{ multimediaProject::SongProjectTemplate, "songtemplate" },
	{ multimediaProject::InstrumentTrackSettings,
						"instrumenttracksettings" },
	{ multimediaProject::DragNDropData, "dnddata" },
	{ multimediaProject::ClipboardData, "clipboard-data" },
	{ multimediaProject::JournalData, "journaldata" },
	{ multimediaProject::EffectSettings, "effectsettings" },
	{ multimediaProject::ResourceDatabase, "resourcedatabase" },
	{ multimediaProject::VideoProject, "videoproject" },
	{ multimediaProject::BurnProject, "burnproject" },
	{ multimediaProject::Playlist, "playlist" }
} ;



multimediaProject::multimediaProject( ProjectTypes _project_type ) :
	QDomDocument( "multimedia-project" ),
	m_content(),
	m_head(),
	m_type( _project_type )
{
	QDomElement root = createElement( "multimedia-project" );
	root.setAttribute( "version", MMP_VERSION_STRING );
	root.setAttribute( "type", typeName( _project_type ) );
	root.setAttribute( "creator", "Linux MultiMedia Studio (LMMS)" );
	root.setAttribute( "creatorversion", LMMS_VERSION );
	appendChild( root );

	m_head = createElement( "head" );
	root.appendChild( m_head );

	m_content = createElement( typeName( _project_type ) );
	root.appendChild( m_content );

}




multimediaProject::multimediaProject( const QString & _fileName ) :
	QDomDocument(),
	m_content(),
	m_head()
{
	QFile inFile( _fileName );
	if( !inFile.open( QIODevice::ReadOnly ) )
	{
		QMessageBox::critical( NULL,
			songEditor::tr( "Could not open file" ),
			songEditor::tr( "Could not open file %1. You probably "
					"have no permissions to read this "
					"file.\n Please make sure to have at "
					"least read permissions to the file "
					"and try again." ).arg( _fileName ) );
			return;
	}

	loadData( inFile.readAll(), _fileName );
}




multimediaProject::multimediaProject( const QByteArray & _data ) :
	QDomDocument(),
	m_content(),
	m_head()
{
	loadData( _data, "<internal data>" );
}




multimediaProject::~multimediaProject()
{
}




QString multimediaProject::nameWithExtension( const QString & _fn ) const
{
	switch( type() )
	{
		case SongProject:
			if( _fn.section( '.', -1 ) != "mmp" &&
					_fn.section( '.', -1 ) != "mpt" &&
					_fn.section( '.', -1 ) != "mmpz" )
			{
				if( configManager::inst()->value( "app",
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




bool multimediaProject::writeFile( const QString & _fn )
{
	if( type() == SongProject || type() == SongProjectTemplate
					|| type() == InstrumentTrackSettings )
	{
		cleanMetaNodes( documentElement() );
	}


	QString fn = nameWithExtension( _fn );
	QFile outfile( fn );
	if( !outfile.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
	{
		QMessageBox::critical( NULL,
				songEditor::tr( "Could not write file" ),
					songEditor::tr( "Could not write file "
							"%1. You probably are "
							"not permitted to "
							"write to this file.\n"
							"Please make sure you "
							"have write-access to "
							"the file and try "
							"again."
						).arg( fn ) );
		return false;
	}
	QString xml = "<?xml version=\"1.0\"?>\n" + toString( 2 );
	if( fn.section( '.', -1 ) == "mmpz" )
	{
		outfile.write( qCompress( xml.toUtf8() ) );
	}
	else
	{
		QTextStream( &outfile ) << xml;
	}
	outfile.close();

	return true;
}




multimediaProject::ProjectTypes multimediaProject::type(
						const QString & _type_name )
{
	for( int i = 0; i < NumProjectTypes; ++i )
	{
		if( s_types[i].m_name == _type_name )
		{
			return static_cast<multimediaProject::ProjectTypes>( i );
		}
	}
	if( _type_name == "channelsettings" )
	{
		return multimediaProject::InstrumentTrackSettings;
	}
	return UnknownType;
}




QString multimediaProject::typeName( ProjectTypes _project_type )
{
	if( _project_type >= UnknownType && _project_type < NumProjectTypes )
	{
		return s_types[_project_type].m_name;
	}
	return s_types[UnknownType].m_name;
}




void multimediaProject::cleanMetaNodes( QDomElement _de )
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




void multimediaProject::upgrade( void )
{
	projectVersion version =
		documentElement().attribute( "creatorversion" ).
							replace( "svn", "" );

	if( version < "0.2.1-20070501" )
	{
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
				el.setAttribute( "vol", el.attribute(
						"vol" ).toFloat() * 100.0f );
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

	if( version < "0.2.1-20070508" )
	{
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
				float value = el.attribute( "vol" ).toFloat();
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


	if( version < "0.3.0-rc2" )
	{
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

	if( version < "0.3.0" )
	{
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

	if( version < "0.4.0-20080104" )
	{
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

	if( version < "0.4.0-20080118" )
	{
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

	if( version < "0.4.0-20080129" )
	{
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

	if( version < "0.4.0-20080409" )
	{
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

	if( version < "0.4.0-20080607" )
	{
		QDomNodeList list;
		while( !( list = elementsByTagName( "midi" ) ).isEmpty() )
		{
			QDomElement el = list.item( 0 ).toElement();
			el.setTagName( "midiport" );
		}
	}

	if( version < "0.4.0-20080622" )
	{
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

	if( version < "0.4.0-beta1" )
	{
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
	if( version < "0.4.0-rc2" )
	{
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
//printf("%s\n", toString( 2 ).toUtf8().constData());
}




void multimediaProject::loadData( const QByteArray & _data,
					const QString & _sourceFile )
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
			QMessageBox::critical( NULL,
				songEditor::tr( "Error in file" ),
				songEditor::tr( "The file %1 seems to contain "
						"errors and therefore can't be "
						"loaded." ).
							arg( _sourceFile ) );
			return;
		}
	}

	QDomElement root = documentElement();
	m_type = type( root.attribute( "type" ) );
	m_head = root.elementsByTagName( "head" ).item( 0 ).toElement();

	if( root.hasAttribute( "creatorversion" ) &&
		root.attribute( "creatorversion" ) != LMMS_VERSION )
	{
		upgrade();
	}

	m_content = root.elementsByTagName( typeName( m_type ) ).
							item( 0 ).toElement();
}

