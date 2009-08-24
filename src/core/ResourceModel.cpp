/*
 * ResourceModel.cpp - implementation of ResourceModel
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

#include <QtGui/QPainter>

#include "ResourceModel.h"
#include "embed.h"
#include "Plugin.h"
#include "string_pair_drag.h"


ResourceModel::ResourceModel( ResourceDB * _db, QObject * _parent ) :
	QAbstractItemModel( _parent ),
	m_db( _db ),
	m_keywordFilter(),
	m_keywordFilterSet( false ),
	m_typeFilter( ResourceItem::TypeUnknown )
{
	setSupportedDragActions( Qt::CopyAction );

	connect( db(), SIGNAL( itemsChanged() ),
				this, SIGNAL( itemsChanged() ) );
	connect( db(), SIGNAL( itemsChanged() ),
				this, SLOT( updateFilters() ),
				Qt::DirectConnection );
}




QVariant ResourceModel::data( const QModelIndex & _idx, int _role ) const
{
	static QHash<ResourceItem::Type, QPixmap> pixmapCache;

	if( _idx.isValid() )
	{
		ResourceItem::Relation * relation = this->relation( _idx );
		ResourceItem * item = relation->item();
		if( _role == Qt::DisplayRole )
		{
			if( relation->parent() == db()->topLevelNode() )
			{
				switch( item->baseDir() )
				{
					case ResourceItem::BaseWorkingDir:
			return tr( "My LMMS files" );
					case ResourceItem::BaseDataDir:
			return tr( "Shipped LMMS files" );
					case ResourceItem::BaseURL:
			return item->provider()->url();
					default:
						break;
				}
			}
			return item->name();
		}
		else if( _role == Qt::DecorationRole )
		{
			if( relation->parent() == db()->topLevelNode() )
			{
				switch( item->baseDir() )
				{
					case ResourceItem::BaseWorkingDir:
	return embed::getIconPixmap( "mimetypes/folder-workingdir", 24, 24 );
					case ResourceItem::BaseDataDir:
	return embed::getIconPixmap( "mimetypes/folder-datadir", 24, 24 );
					case ResourceItem::BaseURL:
	return embed::getIconPixmap( "mimetypes/folder-web", 24, 24 );
					default:
						break;
				}
			}
			if( pixmapCache.contains( item->type() ) )
			{
				return pixmapCache[item->type()];
			}
			QPixmap pix;
			switch( item->type() )
			{
case ResourceItem::TypeDirectory:
	pix = embed::getIconPixmap( "mimetypes/folder", 24, 24 );
	break;
case ResourceItem::TypeSample:
	pix = embed::getIconPixmap( "mimetypes/sample", 24, 24 );
	break;
case ResourceItem::TypePreset:
	pix = embed::getIconPixmap( "mimetypes/preset", 24, 24 );
	break;
case ResourceItem::TypePluginSpecificResource:
	{
	// always cache plugin-specific pixmaps as their generation
	// is quite expensive compared to hash-lookup
	static QHash<QString, QPixmap> pluginPixmapCache;
	const QString ext = item->nameExtension();
	if( pluginPixmapCache.contains( ext ) )
	{
		return pluginPixmapCache[ext];
	}

	// iterate through all plugins
	QVector<Plugin::Descriptor> descriptors;
	Plugin::getDescriptorsOfAvailPlugins( descriptors );

	for( QVector<Plugin::Descriptor>::iterator it = descriptors.begin();
						it != descriptors.end(); ++it )
	{
		if( it->supportsFileType( ext ) )
		{
			pix = embed::getIconPixmap( "mimetypes/unknown" );
			QPainter p( &pix );
			const QPixmap logo = it->logo->pixmap().
				scaled( 40, 40, Qt::IgnoreAspectRatio,
						Qt::SmoothTransformation );
			p.drawPixmap( ( pix.width() - logo.width() ) / 2,
					( pix.height() - logo.height() ) / 2,
					logo );
			p.end();
			pix = pix.scaled( 24, 24, Qt::IgnoreAspectRatio,
						Qt::SmoothTransformation );
			pluginPixmapCache[ext] = pix;
			return pix;
		}
	}
	return embed::getIconPixmap( "mimetypes/preset", 24, 24 );
	}
case ResourceItem::TypeProject:
	pix = embed::getIconPixmap( "project_file", 24, 24 );
	break;
case ResourceItem::TypeMidiFile:
	pix = embed::getIconPixmap( "mimetypes/midi", 24, 24 );
	break;
case ResourceItem::TypeImage:
	pix = embed::getIconPixmap( "mimetypes/image", 24, 24 );
	break;
case ResourceItem::TypePlugin:
	pix = embed::getIconPixmap( "mimetypes/plugin", 24, 24 );
	break;
default:
	pix = embed::getIconPixmap( "mimetypes/unknown", 24, 24 );
	break;
			}	// end switch( item->type() )
			pixmapCache[item->type()] = pix;
			return pix;
		}	// end if( _role == Qt::DecorationRole )
	}
	return QVariant();
}




Qt::ItemFlags ResourceModel::flags( const QModelIndex & _index ) const
{
	Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	if( _index.isValid() )
	{
		switch( item( _index )->type() )
		{
			case ResourceItem::TypeSample:
			case ResourceItem::TypePreset:
			case ResourceItem::TypePluginSpecificResource:
			case ResourceItem::TypeProject:
			case ResourceItem::TypeMidiFile:
			case ResourceItem::TypeImage:
			case ResourceItem::TypePlugin:
				flags |= Qt::ItemIsDragEnabled;
				break;
			default:
				break;
		}
	}
	return flags;
}




QStringList ResourceModel::mimeTypes() const
{
	return QStringList() << ResourceItem::mimeKey();
}




QMimeData * ResourceModel::mimeData( const QModelIndexList & _list ) const
{
	// we'll only process first item - look whether it is valid
	if( !_list.first().isValid() )
	{
		return NULL;
	}

	// create a QMimeData object containing hash of current item
	return stringPairDrag::createMimeData( ResourceItem::mimeKey(),
					item( _list.first() )->hash() );
}




int ResourceModel::totalItems() const
{
	const ResourceDB::ItemHashMap & items = db()->items();
	int num = 0;
	foreach( const ResourceItem * i, items )
	{
		if( i->type() != ResourceItem::TypeDirectory )
		{
			++num;
		}
	}
	return num;
}




int ResourceModel::shownItems() const
{
	const ResourceDB::ItemHashMap & items = db()->items();
	int num = 0;
	foreach( const ResourceItem * i, items )
	{
		if( i->type() != ResourceItem::TypeDirectory &&
			i->isHidden( this ) == false )
		{
			++num;
		}
	}
	return num;
}




void ResourceModel::setKeywordFilter( const QString & _keywords )
{
	if( !_keywords.isEmpty() )
	{
		m_keywordFilter = _keywords.toLower().split( " " );
		m_keywordFilterSet = true;
	}
	else
	{
		m_keywordFilter.clear();
		m_keywordFilterSet = false;
	}
	updateFilters();
}




void ResourceModel::setTypeFilter( ResourceItem::Type _type )
{
	m_typeFilter = _type;
	updateFilters();
}



#include "moc_ResourceModel.cxx"

/* vim: set tw=0 noexpandtab: */
