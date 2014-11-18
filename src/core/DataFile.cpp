/*
 * DataFile.cpp - implementation of class DataFile
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2012-2013 Paul Giblock    <p/at/pgiblock.net>
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


#include "DataFile.h"

#include <math.h>

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QMessageBox>


#include "ConfigManager.h"
#include "project_version.h"
#include "SongEditor.h"
#include "Effect.h"
#include "lmmsversion.h"
#include "base64.h"

// bbTCO::defaultColor()
#include "bb_track.h"



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



DataFile::LocaleHelper::LocaleHelper( Mode mode )
{
	switch( mode )
	{
		case ModeLoad:
			// set a locale for which QString::fromFloat() returns valid values if
			// floating point separator is a comma - otherwise we would fail to load
			// older projects made by people from various countries due to their
			// locale settings
		    QLocale::setDefault( QLocale::German );
			break;

		case ModeSave:
			// set default locale to C so that floating point decimals are rendered to
			// strings with periods as decimal point instead of commas in some countries
			QLocale::setDefault( QLocale::C );

		default: break;
	}
}



DataFile::LocaleHelper::~LocaleHelper()
{
	// revert to original locale
	QLocale::setDefault( QLocale::system() );
}




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
		QMessageBox::critical( NULL,
			SongEditor::tr( "Could not open file" ),
			SongEditor::tr( "Could not open file %1. You probably "
					"have no permissions to read this "
					"file.\n Please make sure to have at "
					"least read permissions to the file "
					"and try again." ).arg( _fileName ) );
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
		QMessageBox::critical( NULL,
			SongEditor::tr( "Could not write file" ),
			SongEditor::tr( "Could not open %1 for writing. You probably are not permitted to "
							"write to this file. Please make sure you have write-access to "
							"the file and try again." ).arg( fullName ) );
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
		// remove old backup file
		QFile::remove( fullNameBak );
		// move current file to backup file
		QFile::rename( fullName, fullNameBak );
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




void DataFile::upgrade()
{
	projectVersion version =
		documentElement().attribute( "creatorversion" );

	// update document meta data
	documentElement().setAttribute( "version", LDF_VERSION_STRING );
	documentElement().setAttribute( "type", typeName( type() ) );
	documentElement().setAttribute( "creator", "LMMS" );
	documentElement().setAttribute( "creatorversion", LMMS_VERSION );

	// future backwards compat for post-2.0 projects will come here
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
			QMessageBox::critical( NULL,
				SongEditor::tr( "Error in file" ),
				SongEditor::tr( "The file %1 seems to contain "
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

