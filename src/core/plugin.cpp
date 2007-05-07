#ifndef SINGLE_SOURCE_COMPILE

/*
 * plugin.cpp - implementation of plugin-class including plugin-loader
 *
 * Copyright (c) 2005-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "qt3support.h"

#ifdef QT4

#include <QtCore/QDir>
#include <QtCore/QLibrary>
#include <QtGui/QMessageBox>
#include <QtGui/QPixmap>

#else

#include <qmessagebox.h>
#include <qdir.h>
#include <qlibrary.h>
#include <qpixmap.h>

#endif


#include "plugin.h"
#include "mixer.h"
#include "config_mgr.h"
#include "dummy_plugin.h"


//static embed::descriptor dummy_embed = { 0, NULL, "" } ;

static plugin::descriptor dummy_plugin_descriptor =
{
	"dummy",
	"dummy",
	QT_TRANSLATE_NOOP( "pluginBrowser", "no description" ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0100,
	plugin::Undefined,
	NULL,
	NULL
} ;



plugin::plugin( const descriptor * _descriptor ) :
	m_descriptor( _descriptor )
{
	if( dummy_plugin_descriptor.logo == NULL )
	{
		dummy_plugin_descriptor.logo = new QPixmap();
	}

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




plugin * plugin::instantiate( const QString & _plugin_name, void * _data )
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
#ifndef QT4
	plugin_lib.setAutoUnload( FALSE );
#endif
	plugin * inst = inst_hook( _data );
	return( inst );
}




bool plugin::supportsParallelizing( void ) const
{
	return( FALSE );
}




void plugin::waitForWorkerThread( void )
{
}




void plugin::getDescriptorsOfAvailPlugins( vvector<descriptor> & _plugin_descs )
{
	QDir directory( configManager::inst()->pluginDir() );
#ifdef QT4
	QFileInfoList list = directory.entryInfoList(
						QStringList( "lib*.so" ) );
#else
	const QFileInfoList * lp = directory.entryInfoList( "lib*.so" );
	// if directory doesn't exist or isn't readable, we get NULL which would
	// crash LMMS...
	if( lp == NULL )
	{
		return;
	}
	QFileInfoList list = *lp;
#endif
	for( QFileInfoList::iterator file = list.begin();
						file != list.end(); ++file )
	{
#ifdef QT4
		const QFileInfo & f = *file;
#else
		const QFileInfo & f = **file;
#endif
		QLibrary plugin_lib( f.absoluteFilePath() );
		if( plugin_lib.load() == FALSE ||
			plugin_lib.resolve( "lmms_plugin_main" ) == NULL )
		{
			continue;
		}
#ifndef QT4
		plugin_lib.setAutoUnload( FALSE );
#endif
		QString desc_name = f.fileName().section( '.', 0, 0 ) +
							"_plugin_descriptor";
		if( desc_name.left( 3 ) == "lib" )
		{
			desc_name = desc_name.mid( 3 );
		}
		descriptor * plugin_desc = (descriptor *) plugin_lib.resolve(
						desc_name.
#ifdef QT4
							toAscii().constData()
#else
							ascii()
#endif
								);
		if( plugin_desc == NULL )
		{
			continue;
		}
		_plugin_descs.push_back( *plugin_desc );
	}

}




#endif
