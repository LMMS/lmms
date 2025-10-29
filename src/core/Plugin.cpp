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

#include "Plugin.h"

#include <QDomElement>
#include <QLibrary>
#include <QMessageBox>

#include "embed.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "DummyPlugin.h"
#include "AutomatableModel.h"
#include "Song.h"
#include "PluginFactory.h"

namespace lmms
{


static PixmapLoader dummyLoader;

static Plugin::Descriptor dummyPluginDescriptor =
{
	"dummy",
	"dummy",
	QT_TRANSLATE_NOOP( "PluginBrowser", "no description" ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0100,
	Plugin::Type::Undefined,
	&dummyLoader,
	nullptr
} ;




Plugin::Plugin(const Descriptor * descriptor, Model * parent, const
		Descriptor::SubPluginFeatures::Key* key) :
	Model(parent),
	JournallingObject(),
	m_descriptor(descriptor),
	m_key(key ? *key : Descriptor::SubPluginFeatures::Key(m_descriptor))
{
	if( m_descriptor == nullptr )
	{
		m_descriptor = &dummyPluginDescriptor;
	}
}




template<class T>
T use_this_or(T this_param, T or_param)
{
	return this_param ? this_param : or_param;
}




QString use_this_or(QString this_param, QString or_param)
{
	return this_param.isNull() ? or_param : this_param;
}




QString Plugin::displayName() const
{
	return Model::displayName().isEmpty() // currently always empty
		? (m_descriptor->subPluginFeatures && m_key.isValid())
			// get from sub plugin
			? m_key.displayName()
			// get from plugin
			: m_descriptor->displayName
		: Model::displayName();
}




const PixmapLoader* Plugin::logo() const
{
	return (m_descriptor->subPluginFeatures && m_key.isValid())
		? m_key.logo()
		: m_descriptor->logo;
}




QString Plugin::Descriptor::SubPluginFeatures::Key::additionalFileExtensions() const
{
	Q_ASSERT(isValid());
	return desc->subPluginFeatures
		// get from sub plugin
		? desc->subPluginFeatures->additionalFileExtensions(*this)
		// no sub plugin, so no *additional* file extensions
		: QString();
}




QString Plugin::Descriptor::SubPluginFeatures::Key::displayName() const
{
	Q_ASSERT(isValid());
	return desc->subPluginFeatures
		// get from sub plugin
		? use_this_or(desc->subPluginFeatures->displayName(*this),
			QString::fromUtf8(desc->displayName))
		// get from plugin
		: desc->displayName;
}




const PixmapLoader* Plugin::Descriptor::SubPluginFeatures::Key::logo() const
{
	Q_ASSERT(isValid());
	return desc->subPluginFeatures
		? use_this_or(desc->subPluginFeatures->logo(*this), desc->logo)
		: desc->logo;
}




QString Plugin::Descriptor::SubPluginFeatures::Key::description() const
{
	Q_ASSERT(isValid());
	return desc->subPluginFeatures
		? use_this_or(desc->subPluginFeatures->description(*this),
			QString::fromUtf8(desc->description))
		: desc->description;
}




void Plugin::loadFile( const QString & )
{
}




AutomatableModel * Plugin::childModel( const QString & )
{
	static FloatModel fm;
	return &fm;
}



Plugin * Plugin::instantiateWithKey(const QString& pluginName, Model * parent,
				const Descriptor::SubPluginFeatures::Key *key,
				bool keyFromDnd)
{
	if(keyFromDnd)
		Q_ASSERT(!key);
	const Descriptor::SubPluginFeatures::Key *keyPtr = keyFromDnd
		? static_cast<Plugin::Descriptor::SubPluginFeatures::Key*>(Engine::pickDndPluginKey())
		: key;
	const PluginFactory::PluginInfo& pi = getPluginFactory()->pluginInfo(pluginName.toUtf8());
	if(keyPtr)
	{
		// descriptor is not yet set when loading - set it now
		Descriptor::SubPluginFeatures::Key keyCopy = *keyPtr;
		keyCopy.desc = pi.descriptor;
		return Plugin::instantiate(pluginName, parent, &keyCopy);
	}
	else
		return Plugin::instantiate(pluginName, parent,
			// the keys are never touched anywhere
			const_cast<Descriptor::SubPluginFeatures::Key *>(keyPtr));
}




Plugin * Plugin::instantiate(const QString& pluginName, Model * parent,
								void *data)
{
	const PluginFactory::PluginInfo& pi = getPluginFactory()->pluginInfo(pluginName.toUtf8());

	Plugin* inst;
	if( pi.isNull() )
	{
		if (gui::getGUI() != nullptr)
		{
			QMessageBox::information( nullptr,
				tr( "Plugin not found" ),
				tr( "The plugin \"%1\" wasn't found or could not be loaded!\nReason: \"%2\"" ).
						arg( pluginName ).arg( getPluginFactory()->errorString(pluginName) ),
				QMessageBox::Ok | QMessageBox::Default );
		}
		inst = new DummyPlugin();
	}
	else
	{
		auto instantiationHook = reinterpret_cast<InstantiationHook>(pi.library->resolve("lmms_plugin_main"));
		if (instantiationHook)
		{
			inst = instantiationHook(parent, data);
			if(!inst) {
				inst = new DummyPlugin();
			}
		}
		else
		{
			if (gui::getGUI() != nullptr)
			{
				QMessageBox::information( nullptr,
					tr( "Error while loading plugin" ),
					tr( "Failed to load plugin \"%1\"!").arg( pluginName ),
					QMessageBox::Ok | QMessageBox::Default );
			}
			inst = new DummyPlugin();
		}
	}

	return inst;
}




void Plugin::collectErrorForUI( QString errMsg )
{
	Engine::getSong()->collectError( errMsg );
}




gui::PluginView * Plugin::createView( QWidget * parent )
{
	gui::PluginView * pv = instantiateView( parent );
	if( pv != nullptr )
	{
		pv->setModel( this );
	}
	return pv;
}




Plugin::Descriptor::SubPluginFeatures::Key::Key( const QDomElement & key ) :
	desc( nullptr ),
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



} // namespace lmms
