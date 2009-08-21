/*
 * ResourceItem.cpp - implementation of ResourceItem
 *
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtCore/QCryptographicHash>
#include <QtCore/QHash>
#include <QtCore/QDir>

#include "ResourceItem.h"
#include "ResourceProvider.h"
#include "config_mgr.h"
#include "engine.h"



ResourceItem::ResourceItem( ResourceProvider * _provider,
				const QString & _name,
				Type _type,
				BaseDirectory _base_dir,
				const QString & _path,
				const QString & _hash,
				const QString & _author,
				const QString & _tags,
				int _size,
				const QDateTime & _last_mod ) :
	m_provider( _provider ),
	m_name( _name ),
	m_nameHash( 0 ),
	m_type( _type ),
	m_baseDir( _base_dir ),
	m_path( _path ),
	m_hash( _hash ),
	m_author( _author ),
	m_size( _size ),
	m_lastMod( _last_mod ),
	m_tags( _tags ),
	m_relation( NULL )
{
	init();
}




// copy constructor
ResourceItem::ResourceItem( const ResourceItem & _item ) :
	m_provider( _item.m_provider ),
	m_name( _item.m_name ),
	m_nameHash( _item.m_nameHash ),
	m_type( _item.m_type ),
	m_baseDir( _item.m_baseDir ),
	m_path( _item.m_path ),
	m_hash( _item.m_hash),
	m_author( _item.m_author ),
	m_size( _item.m_size ),
	m_lastMod( _item.m_lastMod ),
	m_tags( _item.m_tags ),
	m_relation( NULL )
{
	init();
}




void ResourceItem::reload()
{
	m_hash.clear();
	m_size = -1;
	init();
}




bool ResourceItem::keywordMatch( const QStringList & _keywords ) const
{
	for( QStringList::ConstIterator it = _keywords.begin();
								it != _keywords.end(); ++it )
	{
		if( !( name().toLower().contains( *it ) ||
				path().toLower().contains( *it ) ||
				author().toLower().contains( *it ) ||
				tags().toLower().contains( *it ) ) )
		{
			return false;
		}
	}
	return true;
}




bool ResourceItem::operator==( const ResourceItem & _other ) const
{
	return m_nameHash == _other.m_nameHash &&
		m_name == _other.m_name &&
		m_type == _other.m_type &&
		m_path == _other.m_path &&
		m_hash == _other.m_hash &&
		m_author == _other.m_author &&
		m_size == _other.m_size &&
		m_tags == _other.m_tags;
}




int ResourceItem::equalityLevel( const ResourceItem & _other ) const
{
	int l = 0;
	if( m_nameHash == _other.m_nameHash &&
		m_name == _other.m_name &&
		m_path == _other.m_path )
	{
		l += 40;
	}
	else if( m_nameHash == _other.m_nameHash && m_name == _other.m_name )
	{
		l += 30;
	}
	else if( m_path == _other.m_path )
	{
		l += 10;
	}

	if( m_type == _other.m_type )
	{
		l += 5;
	}

	if( m_author == _other.m_author )
	{
		l += 5;
	}

	if( !m_tags.isEmpty() && !_other.m_tags.isEmpty() )
	{
		QStringList my_tags = m_tags.split( " " );
		QStringList o_tags = _other.m_tags.split( " " );
		foreach( const QString & tag, o_tags )
		{
			if( my_tags.contains( tag ) )
			{
				l += 10;
			}
		}
	}

	if( m_size == _other.m_size )
	{
		l += 20;
	}

	if( m_hash == _other.m_hash )
	{
		l += 100;
	}
	return l;
}




ResourceItem::Type ResourceItem::guessType() const
{
	static QMap<QString, Type> typeMap;
	if( typeMap.isEmpty() )
	{
		typeMap["wav"] = TypeSample;
		typeMap["ogg"] = TypeSample;
		typeMap["mp3"] = TypeSample;
		typeMap["ds"] = TypeSample;
		typeMap["flac"] = TypeSample;
		typeMap["spx"] = TypeSample;
		typeMap["voc"] = TypeSample;
		typeMap["au"] = TypeSample;
		typeMap["raw"] = TypeSample;
		typeMap["aif"] = TypeSample;
		typeMap["aiff"] = TypeSample;

		typeMap["xpf"] = TypePreset;

		typeMap["mmp"] = TypeProject;
		typeMap["mmpz"] = TypeProject;

		typeMap["mid"] = TypeMidiFile;

		typeMap["flp"] = TypeForeignProject;

		typeMap["dll"] = TypePlugin;
		typeMap["so"] = TypePlugin;

		typeMap["png"] = TypeImage;
		typeMap["jpg"] = TypeImage;
		typeMap["jpeg"] = TypeImage;

		for( QMap<QString, QString>::ConstIterator it =
					engine::pluginFileHandling().begin();
			it != engine::pluginFileHandling().end(); ++it )
		{
			if( !typeMap.contains( it.key() ) )
			{
				typeMap[it.key()] = TypePluginSpecificResource;
			}
		}
	}

	const QString s = QFileInfo( name() ).suffix().toLower();
	QMap<QString, Type>::ConstIterator it = typeMap.find( s );
	if( it != typeMap.end() )
	{
		return it.value();
	}

	return TypeUnknown;
}




void ResourceItem::init()
{
	// ensure trailing slash for path property
	if( !m_path.isEmpty() && m_path.right( 1 ) != QDir::separator() )
	{
		m_path += QDir::separator();
	}

	if( m_type == TypeUnknown )
	{
		m_type = guessType();
	}

	// if item is a directory, ensure a trailing slash
	if( m_type == TypeDirectory )
	{
		if( !m_name.isEmpty() &&
				m_name.right( 1 ) != QDir::separator() )
		{
			m_name += QDir::separator();
		}
		if( m_hash.isEmpty() )
		{
			QCryptographicHash h( QCryptographicHash::Sha1 );
			h.addData( fullName().toUtf8() );
			m_hash = h.result().toHex();
		}
	}
	// only stat file if we really need to
	else if( ( m_hash.isEmpty() || m_size < 0 ) &&
						QFile::exists( fullName() ) )
	{
		if( m_size < 0 )
		{
			m_size = realSize();
		}
		if( m_hash.isEmpty() )
		{
			QCryptographicHash h( QCryptographicHash::Sha1 );

			// fetch at most 1 MB for creating hash
			h.addData( fetchData( 1 * 1024 * 1024 ) );

			m_hash = h.result().toHex();
		}
	}

	if( !m_name.isEmpty() )
	{
		m_nameHash = qHash( m_name );
	}
}




QString ResourceItem::getBaseDirectory( BaseDirectory _bd,
						const ResourceItem * _item )
{
	QString d;
	switch( _bd )
	{
		case BaseRoot:
			d = QDir::rootPath();
			break;

		case BaseWorkingDir:
			d = configManager::inst()->workingDir();
			break;

		case BaseDataDir:
			d = configManager::inst()->dataDir();
			break;

		case BaseURL:
			if( _item )
			{
				d = _item->provider()->url();
				break;
			}

		case BaseHome:
		default:
			d = QDir::homePath();
			break;
	}
	if( !d.isEmpty() && d.right( 1 ) != QDir::separator() )
	{
		d += QDir::separator();
	}

	return d;
}



