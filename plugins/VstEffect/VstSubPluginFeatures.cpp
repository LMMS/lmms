/*
 * VstSubPluginFeatures.cpp - derivation from
 *                            Plugin::Descriptor::SubPluginFeatures for
 *                            hosting VST-plugins
 *
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtCore/QDir>
#include <QtGui/QLabel>

#include "VstSubPluginFeatures.h"
#include "config_mgr.h"


VstSubPluginFeatures::VstSubPluginFeatures( Plugin::PluginTypes _type ) :
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
	QStringList dlls = QDir( configManager::inst()->vstDir() ).
				entryList( QStringList() << "*.dll",
						QDir::Files, QDir::Name );
	// TODO: eval m_type
	for( QStringList::ConstIterator it = dlls.begin();
							it != dlls.end(); ++it )
	{
		EffectKey::AttributeMap am;
		am["file"] = *it;
		_kl.push_back( Key( _desc, QFileInfo( *it ).baseName(), am ) );
	}
}

