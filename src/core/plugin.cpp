#ifndef SINGLE_SOURCE_COMPILE

/*
 * plugin.cpp - implementation of plugin-class including plugin-loader
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtCore/QDir>
#include <QtCore/QLibrary>
#include <QtGui/QMessageBox>
#include <QtGui/QPixmap>


#include "plugin.h"
#include "embed.h"
#include "mixer.h"
#include "config_mgr.h"
#include "dummy_plugin.h"


static pixmapLoader __dummy_loader;

static plugin::descriptor dummy_plugin_descriptor =
{
	"dummy",
	"dummy",
	QT_TRANSLATE_NOOP( "pluginBrowser", "no description" ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0100,
	plugin::Undefined,
	&__dummy_loader,
	NULL
} ;




plugin::plugin( const descriptor * _descriptor, model * _parent ) :
	journallingObject(),
	model( _parent ),
	m_descriptor( _descriptor )
{
	if( m_descriptor == NULL )
	{
		m_descriptor = &dummy_plugin_descriptor;
	}
}




plugin::~plugin()
{
}




void plugin::setParameter( const QString &, const QString & )
{
}




QString plugin::getParameter( const QString & )
{
	return( "" );
}




plugin * plugin::instantiate( const QString & _plugin_name, model * _parent,
								void * _data )
{
	QLibrary plugin_lib( configManager::inst()->pluginDir() +
								_plugin_name );
	if( plugin_lib.load() == FALSE )
	{
		QMessageBox::information( NULL,
					mixer::tr( "Plugin not found" ),
					mixer::tr( "The %1-plugin wasn't found!"
							).arg( _plugin_name ),
					QMessageBox::Ok |
						QMessageBox::Default );
		return( new dummyPlugin() );
	}
	instantiationHook inst_hook = ( instantiationHook ) plugin_lib.resolve(
							"lmms_plugin_main" );
	if( inst_hook == NULL )
	{
		QMessageBox::information( NULL,
				mixer::tr( "Error while loading plugin" ),
				mixer::tr( "Failed loading plugin \"%1\"!"
							).arg( _plugin_name ),
					QMessageBox::Ok |
						QMessageBox::Default );
		return( new dummyPlugin() );
	}
	plugin * inst = inst_hook( _parent, _data );
	return( inst );
}




bool plugin::supportsParallelizing( void ) const
{
	return( FALSE );
}




void plugin::waitForWorkerThread( void )
{
}




void plugin::getDescriptorsOfAvailPlugins( QVector<descriptor> & _plugin_descs )
{
	QDir directory( configManager::inst()->pluginDir() );
#ifdef LMMS_BUILD_WIN32
	QFileInfoList list = directory.entryInfoList(
						QStringList( "*.dll" ) );
#else
	QFileInfoList list = directory.entryInfoList(
						QStringList( "lib*.so" ) );
#endif
	for( QFileInfoList::iterator file = list.begin();
						file != list.end(); ++file )
	{
		const QFileInfo & f = *file;
		QLibrary plugin_lib( f.absoluteFilePath() );
		if( plugin_lib.load() == FALSE ||
			plugin_lib.resolve( "lmms_plugin_main" ) == NULL )
		{
			continue;
		}
		QString desc_name = f.fileName().section( '.', 0, 0 ) +
							"_plugin_descriptor";
		if( desc_name.left( 3 ) == "lib" )
		{
			desc_name = desc_name.mid( 3 );
		}
		descriptor * plugin_desc =
			(descriptor *) plugin_lib.resolve(
					desc_name.toAscii().constData() );
		if( plugin_desc == NULL )
		{
			printf( "LMMS-plugin %s does not have a "
				"plugin-descriptor named %s!\n",
				f.absoluteFilePath().toAscii().constData(),
					desc_name.toAscii().constData() );
			continue;
		}
		_plugin_descs.push_back( *plugin_desc );
	}

}




pluginView * plugin::createView( QWidget * _parent )
{
	pluginView * pv = instantiateView( _parent );
	if( pv != NULL )
	{
		pv->setModel( this );
	}
	return( pv );
}




#endif
