/*
 * Plugin.cpp - implementation of plugin-class including plugin-loader
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QLibrary>
#include <QtGui/QMessageBox>

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




Plugin::Plugin( const Descriptor * _descriptor, Model * parent ) :
	Model( parent ),
	JournallingObject(),
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




Plugin * Plugin::instantiate( const QString & pluginName, Model * parent,
								void * data )
{
	QLibrary pluginLibrary( configManager::inst()->pluginDir() + pluginName );
	if( pluginLibrary.load() == false )
	{
		if( engine::hasGUI() )
		{
			QMessageBox::information( NULL,
				tr( "Plugin not found" ),
				tr( "The plugin \"%1\" wasn't found or could not be loaded!\nReason: \"%2\"" ).
						arg( pluginName ).arg( pluginLibrary.errorString() ),
				QMessageBox::Ok | QMessageBox::Default );
		}
		return new DummyPlugin();
	}

	InstantiationHook instantiationHook = ( InstantiationHook ) pluginLibrary.resolve( "lmms_plugin_main" );
	if( instantiationHook == NULL )
	{
		if( engine::hasGUI() )
		{
			QMessageBox::information( NULL,
				tr( "Error while loading plugin" ),
				tr( "Failed to load plugin \"%1\"!").arg( pluginName ),
				QMessageBox::Ok | QMessageBox::Default );
		}
		return new DummyPlugin();
	}

	Plugin * inst = instantiationHook( parent, data );
	return inst;
}




void Plugin::getDescriptorsOfAvailPlugins( DescriptorList& pluginDescriptors )
{
	QDir directory( configManager::inst()->pluginDir() );
#ifdef LMMS_BUILD_WIN32
	QFileInfoList list = directory.entryInfoList( QStringList( "*.dll" ) );
#else
	QFileInfoList list = directory.entryInfoList( QStringList( "lib*.so" ) );
#endif
	foreach( const QFileInfo& f, list )
	{
		QLibrary( f.absoluteFilePath() ).load();
	}

	foreach( const QFileInfo& f, list )
	{
		QLibrary pluginLibrary( f.absoluteFilePath() );
		if( pluginLibrary.load() == false ||
			pluginLibrary.resolve( "lmms_plugin_main" ) == NULL )
		{
			continue;
		}

		QString descriptorName = f.baseName() + "_plugin_descriptor";
		if( descriptorName.left( 3 ) == "lib" )
		{
			descriptorName = descriptorName.mid( 3 );
		}

		Descriptor* pluginDescriptor = (Descriptor *) pluginLibrary.resolve( descriptorName.toUtf8().constData() );
		if( pluginDescriptor == NULL )
		{
			qWarning() << tr( "LMMS plugin %1 does not have a plugin descriptor named %2!" ).
								arg( f.absoluteFilePath() ).arg( descriptorName );
			continue;
		}

		pluginDescriptors += *pluginDescriptor;
	}

}




PluginView * Plugin::createView( QWidget * parent )
{
	PluginView * pv = instantiateView( parent );
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

#include "moc_Plugin.cxx"
