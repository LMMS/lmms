/*
 * Plugin.cpp - implementation of plugin-class including plugin-loader
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QMessageBox>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QLibrary>

#include "AutomatableModel.h"
#include "Plugin.h"
#include "embed.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "DummyPlugin.h"
#include "Lv2Instrument.h"
#include "Lv2Manager.h"
#include "Song.h"


static PixmapLoader dummyLoader;

static Plugin::Descriptor dummyPluginDescriptor =
{
	"dummy",
	"dummy",
	QT_TRANSLATE_NOOP( "pluginBrowser", "no description" ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0100,
	Plugin::Undefined,
  Plugin::Embedded,
	&dummyLoader,
	NULL
} ;




Plugin::Plugin( const Descriptor * descriptor, Model * parent ) :
	Model( parent ),
	JournallingObject(),
	m_descriptor( descriptor )
{
	if( m_descriptor == NULL )
	{
		m_descriptor = &dummyPluginDescriptor;
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

#include "EmbeddedPluginFactory.h"
Plugin* Plugin::instantiate(PluginProtocol protocol,
                            const QString& _plugin_id,
                            Model * parent,
                            void * data )
{
	switch (protocol)
	{
		case Embedded:
			{
				const EmbeddedPluginFactory::PluginInfo& pi =
					pluginFactory->pluginInfo(_plugin_id.toUtf8());
				if( pi.isNull() )
				{
					if( gui )
					{
						QMessageBox::information( NULL,
							tr( "Plugin not found" ),
							tr( "The plugin \"%1\" wasn't found or could not be loaded!\nReason: \"%2\"" ).
									arg(_plugin_id).arg( pluginFactory->errorString(_plugin_id)),
							QMessageBox::Ok | QMessageBox::Default );
					}
					return new DummyPlugin();
				}

				InstantiationHook instantiationHook = ( InstantiationHook ) pi.library->resolve( "lmms_plugin_main" );
				if( instantiationHook == NULL )
				{
					if( gui )
					{
						QMessageBox::information( NULL,
							tr( "Error while loading plugin" ),
							tr( "Failed to load plugin \"%1\"!").arg(_plugin_id),
							QMessageBox::Ok | QMessageBox::Default );
					}
					return new DummyPlugin();
				}

				Plugin * inst = instantiationHook( parent, data );
				return inst;

				break;
			}
		case Lv2:
			{
				return new Lv2Instrument(_plugin_id,
                   static_cast<InstrumentTrack*>(data));
				break;
			}
		case LADSPA:
			//
			break;
    case VST:

      break;
	}
	return nullptr;
}


void Plugin::collectErrorForUI( QString errMsg )
{
	Engine::getSong()->collectError( errMsg );
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




Plugin::Descriptor::SubPluginFeatures::Key::Key( const QDomElement & key ) :
	desc( NULL ),
	name( key.attribute( "key" ) ),
	attributes()
{
	QDomNodeList l = key.elementsByTagName( "attribute" );
	for( int i = 0; !l.item( i ).isNull(); ++i )
	{
		QDomElement e = l.item( i ).toElement();
		attributes[e.attribute( "name" )] = e.attribute( "value" );
	}

}




QDomElement Plugin::Descriptor::SubPluginFeatures::Key::saveXML(
						QDomDocument & doc ) const
{
	QDomElement e = doc.createElement( "key" );
	for( AttributeMap::ConstIterator it = attributes.begin();
		it != attributes.end(); ++it )
	{
		QDomElement a = doc.createElement( "attribute" );
		a.setAttribute( "name", it.key() );
		a.setAttribute( "value", it.value() );
		e.appendChild( a );
	}
	return e;
}



