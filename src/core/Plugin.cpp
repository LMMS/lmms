#ifndef SINGLE_SOURCE_COMPILE

/*
 * Plugin.cpp - implementation of plugin-class including plugin-loader
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QDir>
#include <QLibrary>
#include <QMessageBox>

#include "Plugin.h"
#include "embed.h"
#include "engine.h"
#include "Mixer.h"
#include "config_mgr.h"
#include "DummyPlugin.h"
#include "AutomatableModel.h"


static PixmapLoader __dummy_loader;

static Plugin::Descriptor dummy_plugin_descriptor =
{
	"dummy",
	"dummy",
	QT_TRANSLATE_NOOP( "pluginBrowser", "no description" ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0100,
	Plugin::Undefined,
	&__dummy_loader,
	NULL
} ;




Plugin::Plugin( const Descriptor * _descriptor, Model * _parent ) :
	JournallingObject(),
	Model( _parent ),
	m_descriptor( _descriptor )
{
	if( m_descriptor == NULL )
	{
		m_descriptor = &dummy_plugin_descriptor;
	}
}




Plugin::~Plugin()
{
}




void Plugin::loadFile( const QString & )
{
}




AutomatableModel * Plugin::childModel( const QString & )
{
	static FloatModel fm;
	return &fm;
}




Plugin * Plugin::instantiate( const QString & _plugin_name, Model * _parent,
								void * _data )
{
	QLibrary plugin_lib( configManager::inst()->pluginDir() +
								_plugin_name );
	if( plugin_lib.load() == false )
	{
		if( engine::hasGUI() )
		{
			QMessageBox::information( NULL,
				tr( "Plugin not found" ),
				tr( "The plugin \"%1\" wasn't found "
					"or could not be loaded!\n"
					"Reason: \"%2\"" ).arg( _plugin_name ).
						arg( plugin_lib.errorString() ),
					QMessageBox::Ok |
						QMessageBox::Default );
		}
		return new DummyPlugin();
	}
	instantiationHook inst_hook = ( instantiationHook ) plugin_lib.resolve(
							"lmms_plugin_main" );
	if( inst_hook == NULL )
	{
		if( engine::hasGUI() )
		{
			QMessageBox::information( NULL,
				tr( "Error while loading plugin" ),
				tr( "Failed to load plugin \"%1\"!"
							).arg( _plugin_name ),
					QMessageBox::Ok |
						QMessageBox::Default );
		}
		return new DummyPlugin();
	}
	Plugin * inst = inst_hook( _parent, _data );
	return inst;
}




void Plugin::getDescriptorsOfAvailPlugins( DescriptorList & _plugin_descs )
{
	QDir directory( configManager::inst()->pluginDir() );
#ifdef LMMS_BUILD_WIN32
	QFileInfoList list = directory.entryInfoList(
						QStringList( "*.dll" ) );
#else
	QFileInfoList list = directory.entryInfoList(
						QStringList( "lib*.so" ) );
#endif
	foreach( const QFileInfo & f, list )
	{
		QLibrary( f.absoluteFilePath() ).load();
	}

	foreach( const QFileInfo & f, list )
	{
		QLibrary plugin_lib( f.absoluteFilePath() );
		if( plugin_lib.load() == false ||
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
		Descriptor * plugin_desc =
			(Descriptor *) plugin_lib.resolve(
					desc_name.toUtf8().constData() );
		if( plugin_desc == NULL )
		{
			printf( "LMMS plugin %s does not have a "
				"plugin descriptor named %s!\n",
				f.absoluteFilePath().toUtf8().constData(),
					desc_name.toUtf8().constData() );
			continue;
		}
		_plugin_descs.push_back( *plugin_desc );
	}

}




PluginView * Plugin::createView( QWidget * _parent )
{
	PluginView * pv = instantiateView( _parent );
	if( pv != NULL )
	{
		pv->setModel( this );
	}
	return pv;
}



Plugin::Descriptor::SubPluginFeatures::Key::Key(
						const QDomElement & _key ) :
	desc( NULL ),
	name( _key.attribute( "key" ) ),
	attributes()
{
	QDomNodeList l = _key.elementsByTagName( "attribute" );
	for( int i = 0; !l.item( i ).isNull(); ++i )
	{
		QDomElement e = l.item( i ).toElement();
		attributes[e.attribute( "name" )] = e.attribute( "value" );
	}
		
}




QDomElement Plugin::Descriptor::SubPluginFeatures::Key::saveXML(
						QDomDocument & _doc ) const
{
	QDomElement e = _doc.createElement( "key" );
	for( AttributeMap::ConstIterator it = attributes.begin();
									it != attributes.end(); ++it )
	{
		QDomElement a = _doc.createElement( "attribute" );
		a.setAttribute( "name", it.key() );
		a.setAttribute( "value", it.value() );
		e.appendChild( a );
	}
	return e;
}


#endif
