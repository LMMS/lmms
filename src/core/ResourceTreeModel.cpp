/*
 * ResourceTreeModel.cpp - implementation of ResourceTreeModel
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "ResourceTreeModel.h"
#include "embed.h"
#include "plugin.h"
#include "string_pair_drag.h"


ResourceTreeModel::ResourceTreeModel( ResourceDB * _db, QObject * _parent ) :
	QAbstractItemModel( _parent ),
	m_db( _db )
{
	setSupportedDragActions( Qt::CopyAction );

	connect( m_db, SIGNAL( itemsChanged() ),
			this, SIGNAL( itemsChanged() ) );
}




QVariant ResourceTreeModel::data( const QModelIndex & _idx, int _role ) const
{
	static QHash<ResourceItem::Type, QPixmap> pixmapCache;

	if( _idx.isValid() )
	{
		ResourceTreeItem * treeItem = this->treeItem( _idx );
		ResourceItem * item = treeItem->item();
		if( _role == Qt::DisplayRole )
		{
			if( treeItem->parent() == m_db->topLevelNode() )
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
			if( treeItem->parent() == m_db->topLevelNode() )
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
	QVector<plugin::descriptor> descriptors;
	plugin::getDescriptorsOfAvailPlugins( descriptors );

	for( QVector<plugin::descriptor>::iterator it = descriptors.begin();
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




Qt::ItemFlags ResourceTreeModel::flags( const QModelIndex & _index ) const
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




int ResourceTreeModel::rowCount( const QModelIndex & _parent ) const
{
	ResourceTreeItem * parentItem;

	if( _parent.column() > 0 )
	{
		return 0;
	}

	if( !_parent.isValid() )
	{
		parentItem = m_db->topLevelNode();
	}
	else
	{
		parentItem = treeItem( _parent );
	}
	return parentItem->rowCount();
}




QModelIndex ResourceTreeModel::index( int _row, int _col,
					const QModelIndex & _parent ) const
{
	if( !hasIndex( _row, _col, _parent ) )
	{
		return QModelIndex();
	}

	ResourceTreeItem * parentItem;

	if( !_parent.isValid() )
	{
		parentItem = m_db->topLevelNode();
	}
	else
	{
		parentItem = treeItem( _parent );
	}

	if( _row < parentItem->rowCount() )
	{
		return createIndex( _row, _col, parentItem->getChild( _row ) );
	}
	return QModelIndex();
}




QModelIndex ResourceTreeModel::parent( const QModelIndex & _idx ) const
{
	if( !_idx.isValid() )
	{
		return QModelIndex();
	}

	ResourceTreeItem * childItem = treeItem( _idx );
	ResourceTreeItem * parentItem = childItem->parent();
	if( parentItem == m_db->topLevelNode() )
	{
		return QModelIndex();
	}

	int row = 0;
	if( parentItem )
	{
		row = parentItem->row();
	}
	return createIndex( row, 0, parentItem );
}




QStringList ResourceTreeModel::mimeTypes() const
{
	return QStringList() << ResourceItem::mimeKey();
}




QMimeData * ResourceTreeModel::mimeData( const QModelIndexList & _list ) const
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




void ResourceTreeModel::setFilter( const QString & _s )
{
	filterItems( m_db->topLevelNode(),
				createIndex( 0, 0, m_db->topLevelNode() ),
						_s.toLower().split( " " ) );
	if( _s.isEmpty() )
	{
		emit layoutChanged();
	}
}




int ResourceTreeModel::totalItems() const
{
	const ResourceDB::ItemHashMap & items = m_db->items();
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




int ResourceTreeModel::shownItems() const
{
	const ResourceDB::ItemHashMap & items = m_db->items();
	int num = 0;
	foreach( const ResourceItem * i, items )
	{
		if( i->type() != ResourceItem::TypeDirectory &&
			i->treeItem()->isHidden() == false )
		{
			++num;
		}
	}
	return num;
}





bool ResourceTreeModel::filterItems( ResourceTreeItem * _item,
						const QModelIndex & _parent,
						const QStringList & _keywords )
{
	if( _item->item() )
	{
		ResourceItem * i = _item->item();
		bool accept = true;
		for( QStringList::ConstIterator it = _keywords.begin();
						it != _keywords.end(); ++it )
		{
			if( !( i->name().toLower().contains( *it ) ||
				i->path().toLower().contains( *it ) ||
				i->author().toLower().contains( *it ) ||
				i->tags().toLower().contains( *it ) ) )
			{
				accept = false;
				break;
			}
		}
		if( accept )
		{
			setHidden( _item, _parent, false );
			return true;
		}
	}

	int row = 0;
	bool hide = true;
	for( ResourceTreeItemList::Iterator it = _item->children().begin();
					it != _item->children().end(); ++it )
	{
		QModelIndex idx = createIndex( row, 0, *it );
		if( filterItems( *it, idx , _keywords ) )
		{
			hide = false;
		}
		++row;
	}

	setHidden( _item, _parent, hide, false );
	return !hide;
}




void ResourceTreeModel::setHidden( ResourceTreeItem * _item,
						const QModelIndex & _parent,
						bool _hide, bool _recursive )
{
	if( _recursive )
	{
		int row = 0;
		for( ResourceTreeItemList::Iterator it =
						_item->children().begin();
					it != _item->children().end(); ++it )
		{
			setHidden( *it, createIndex( row, 0, *it ), _hide );
			++row;
		}
	}
	if( _item->isHidden() != _hide )
	{
		_item->setHidden( _hide );

/*		if( _hide )
		{
			int row = _item->row();
			beginRemoveRows( _parent, row, row );
			_item->setHidden( true );
			endRemoveRows();
		}
		else
		{
			_item->setHidden( false );
			int row = _item->row();
			beginInsertRows( _parent, row, row );
			endInsertRows();
		}*/
	}
}




#include "moc_ResourceTreeModel.cxx"

/* vim: set tw=0 noexpandtab: */
