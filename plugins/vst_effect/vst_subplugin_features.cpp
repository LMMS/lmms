/*
 * vst_subplugin_features.cpp - derivation from
 *                              plugin::descriptor::subPluginFeatures for
 *                              hosting VST-plugins
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "vst_subplugin_features.h"

#include <QtCore/QDir>
#include <QtGui/QLabel>

#include "config_mgr.h"


vstSubPluginFeatures::vstSubPluginFeatures( plugin::PluginTypes _type ) :
	subPluginFeatures( _type )
{
}




void vstSubPluginFeatures::fillDescriptionWidget( QWidget * _parent,
							const key * _key  )
{
	new QLabel( QWidget::tr( "Name: " ) + _key->name, _parent );
	new QLabel( QWidget::tr( "File: " ) + _key->attributes["file"],
								_parent );
}




void vstSubPluginFeatures::listSubPluginKeys( plugin::descriptor * _desc,
								keyList & _kl )
{
	QStringList dlls = QDir( configManager::inst()->vstDir() ).
				entryList( QStringList() << "*.dll",
						QDir::Files, QDir::Name );
	// TODO: eval m_type
	for( QStringList::const_iterator it = dlls.begin();
							it != dlls.end(); ++it )
	{
		effectKey::attributeMap am;
		am["file"] = *it;
		_kl.push_back( key( _desc, QFileInfo( *it ).baseName(), am ) );
	}

}

