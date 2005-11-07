/*
 * plugin.cpp - implemenation of plugin-class including plugin-loader
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#include "qt3support.h"

#ifdef QT4

#include <QMessageBox>
#include <QDir>

#else

#include <qmessagebox.h>
#include <qdir.h>

#endif


#include "plugin.h"
#include "mixer.h"
#include "config_mgr.h"
#include "dummy_plugin.h"



plugin::plugin( const QString & _public_name, pluginTypes _type ) :
	settings(),
	m_publicName( _public_name ),
	m_type( _type )
{
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
#ifdef HAVE_DLFCN_H
	void * handle = dlopen( QString( "lib" + _plugin_name + ".so" ).
#ifdef QT4
					toAscii().constData()
#else
					ascii()
#endif
					, RTLD_LAZY );
	if( handle == NULL )
	{
		QMessageBox::information( NULL,
					mixer::tr( "Plugin not found" ),
					mixer::tr( "The %1-plugin wasn't found!"
							).arg( _plugin_name ),
					QMessageBox::Ok |
						QMessageBox::Default );
		return( new dummyPlugin() );
	}
	dlerror();
	instantiationHook inst_hook = ( instantiationHook )( dlsym( handle,
							"lmms_plugin_main" ) );
	char * error = dlerror();
	if( error != NULL )
	{
		printf( "%s\n", error );
		QMessageBox::information( NULL,
				mixer::tr( "Error while loading plugin" ),
				mixer::tr( "Failed loading plugin \"%1\"!"
							).arg( _plugin_name ),
					QMessageBox::Ok |
						QMessageBox::Default );
		return( new dummyPlugin() );
	}
	plugin * inst = inst_hook( _data );
	//dlclose( handle );
	return( inst );
#endif
	return( new dummyPlugin() );
}




void plugin::getDescriptorsOfAvailPlugins( vvector<descriptor> & _plugin_descs )
{
	QDir directory( configManager::inst()->pluginDir() );
	const QFileInfoList * list = directory.entryInfoList( "*.so" );
	if( list == NULL )
	{
		return;
	}
	for( QFileInfoList::iterator file = list->begin();
						file != list->end(); ++file )
	{
		void * handle = dlopen( ( *file )->absFilePath().latin1(),
								RTLD_LAZY );
		char * msg = dlerror();
		if( msg != NULL || handle == NULL )
		{
			printf( "plugin-loader: %s\n", msg );
			continue;
		}
		void * foo = dlsym( handle, "lmms_plugin_main" );
		msg = dlerror();
		if( msg != NULL || foo == NULL )
		{
			printf( "plugin-loader: %s\n", msg );
			continue;
		}
		QString desc_name = ( *file )->fileName().mid( 3 ).section(
					'.', 0, 0 ) + "_plugin_descriptor";
		descriptor * plugin_desc =
			(descriptor *) dlsym( handle, desc_name.ascii() );
		msg = dlerror();
		if( msg != NULL || plugin_desc == NULL )
		{
			printf( "plugin-loader: %s\n", msg );
			continue;
		}
		_plugin_descs.push_back( *plugin_desc );
		//dlclose( handle );
	}

}



