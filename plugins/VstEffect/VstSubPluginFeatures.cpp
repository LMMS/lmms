/*
 * VstSubPluginFeatures.cpp - derivation from
 *                            Plugin::Descriptor::SubPluginFeatures for
 *                            hosting VST-plugins
 *
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QDir>
#include <QLabel>

#include "VstSubPluginFeatures.h"
#include "ConfigManager.h"
#include "Effect.h"

namespace lmms
{


VstSubPluginFeatures::VstSubPluginFeatures( Plugin::Type _type ) :
	SubPluginFeatures( _type )
{
}




void VstSubPluginFeatures::fillDescriptionWidget( QWidget * _parent,
													const Key * _key  ) const
{
	new QLabel( QWidget::tr( "Name: " ) + _key->name, _parent );
	new QLabel( QWidget::tr( "File: " ) + _key->attributes["file"], _parent );
}




void VstSubPluginFeatures::listSubPluginKeys( const Plugin::Descriptor * _desc,
														KeyList & _kl ) const
{
	auto dlls = new QStringList();
	const QString path = QString("");
	addPluginsFromDir(dlls, path );
	// TODO: eval m_type
	for( QStringList::ConstIterator it = dlls->begin();
							it != dlls->end(); ++it )
	{
		EffectKey::AttributeMap am;
		am["file"] = *it;
		_kl.push_back( Key( _desc, QFileInfo( *it ).baseName(), am ) );
	}
	delete dlls;
}

void VstSubPluginFeatures::addPluginsFromDir( QStringList* filenames, QString path ) const
{
	QStringList dirs = QDir ( ConfigManager::inst()->vstDir() + path ).
				entryList( QStringList() << "*" ,
						   QDir::Dirs, QDir::Name );
	for( int i = 0; i < dirs.size(); i++ )
	{
		if( dirs.at( i )[0] != '.' )
		{
			addPluginsFromDir( filenames, path+QDir::separator() + dirs.at( i ) );
		}
	}
	QStringList dlls = QDir( ConfigManager::inst()->vstDir() + path ).
				entryList( QStringList() << "*.dll"
#ifdef LMMS_BUILD_LINUX
										 << "*.so"
#endif
						,
						QDir::Files, QDir::Name );
	for( int i = 0; i < dlls.size(); i++ )
	{
		QString fName = path + QDir::separator() + dlls.at( i );
		fName.remove( 0, 1 );
		filenames->append( fName );
	}
}


} // namespace lmms
