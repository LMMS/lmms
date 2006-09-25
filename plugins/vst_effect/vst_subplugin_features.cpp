/*
 * vst_subplugin_features.cpp - derivation from
 *                              plugin::descriptor::subPluginFeatures for
 *                              hosting VST-plugins
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifdef QT4

#include <QtCore/QDir>
#include <QtGui/QGroupBox>
#include <QtGui/QLabel>
#include <QtGui/QLayout>

#else

#include <qdir.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>

#endif


#include "vst_subplugin_features.h"
#include "config_mgr.h"


vstSubPluginDescriptionWidget::vstSubPluginDescriptionWidget(
					QWidget * _parent, engine * _engine,
					const effectKey & _key ) :
	QWidget( _parent )
{
	QVBoxLayout * l = new QVBoxLayout( this );

#ifndef QT3
	QGroupBox * groupbox = new QGroupBox( tr( "Description" ), this );
#else
	QGroupBox * groupbox = new QGroupBox( 9, Qt::Vertical, 
						tr( "Description" ), this );
#endif

	new QLabel( tr( "Name: " ) + _key.name, groupbox );
	new QLabel( tr( "File: " ) + _key.user.toString(), groupbox );

	l->addWidget( groupbox );
}




vstSubPluginFeatures::vstSubPluginFeatures( plugin::pluginTypes _type ) :
	subPluginFeatures( _type )
{
}




QWidget * vstSubPluginFeatures::createDescriptionWidget(
			QWidget * _parent, engine * _eng, const key & _key  )
{
	return( new vstSubPluginDescriptionWidget( _parent, _eng, _key ) );
}




void vstSubPluginFeatures::listSubPluginKeys( engine * _eng,
				plugin::descriptor * _desc, keyList & _kl )
{
	QStringList dlls = QDir( configManager::inst()->vstDir() ).
#ifndef QT3
				entryList( QStringList() << "*.dll",
						QDir::Files, QDir::Name );
#else
				entryList( "*.dll", QDir::Files, QDir::Name );
#endif
	// TODO: eval m_type
	for( QStringList::const_iterator it = dlls.begin();
							it != dlls.end(); ++it )
	{
			_kl.push_back( key( _desc, QFileInfo( *it ).baseName(),
								*it ) );
	}

}

