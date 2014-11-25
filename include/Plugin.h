/*
 * Plugin.h - class plugin, the base-class and generic interface for all plugins
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

#ifndef PLUGIN_H
#define PLUGIN_H

#include <QtCore/QStringList>
#include <QtCore/QMap>
#include <QtXml/QDomDocument>

#include "JournallingObject.h"
#include "Model.h"


class QWidget;

class PixmapLoader;
class PluginView;
class AutomatableModel;


class EXPORT Plugin : public Model, public JournallingObject
{
	Q_OBJECT
public:
	enum PluginTypes
	{
		Instrument,	// instrument being used in channel-track
		Effect,		// effect-plugin for effect-board
		ImportFilter,	// filter for importing a file
		ExportFilter,	// filter for exporting a file
		Tool,		// additional tool (level-meter etc)
		Library,	// simple library holding a code-base for
				// several other plugins (e.g. VST-support)
		Other,
		Undefined = 255
	} ;

	// descriptor holds information about a plugin - every external plugin
	// has to instantiate such a descriptor in an extern "C"-section so that
	// the plugin-loader is able to access information about the plugin
	struct Descriptor
	{
		const char * name;
		const char * displayName;
		const char * description;
		const char * author;
		int version;
		PluginTypes type;
		const PixmapLoader * logo;
		const char * supportedFileTypes;

		inline bool supportsFileType( const QString& extension ) const
		{
			return QString( supportedFileTypes ).split( QChar( ',' ) ).contains( extension );
		}

		class EXPORT SubPluginFeatures
		{
		public:
			struct Key
			{
				typedef QMap<QString, QString> AttributeMap;

				inline Key( const Plugin::Descriptor * _desc = NULL,
							const QString & _name = QString(),
							const AttributeMap & _am = AttributeMap() )
					:
					desc( _desc ),
					name( _name ),
					attributes( _am )
				{
				}

				Key( const QDomElement & _key );

				QDomElement saveXML( QDomDocument & _doc ) const;

				inline bool isValid() const
				{
					return desc != NULL && name.isNull() == false;
				}

				const Plugin::Descriptor* desc;
				QString name;
				AttributeMap attributes;
			} ;

			typedef QList<Key> KeyList;


			SubPluginFeatures( Plugin::PluginTypes type ) :
				m_type( type )
			{
			}

			virtual ~SubPluginFeatures()
			{
			}

			virtual void fillDescriptionWidget( QWidget *, const Key * ) const
			{
			}

			virtual void listSubPluginKeys( const Plugin::Descriptor *, KeyList & ) const
			{
			}


		protected:
			const Plugin::PluginTypes m_type;
		} ;

		SubPluginFeatures * subPluginFeatures;

	} ;

	// typedef a list so we can easily work with list of plugin descriptors
	typedef QList<Descriptor> DescriptorList;

	// contructor of a plugin
	Plugin( const Descriptor* descriptor, Model* parent );
	virtual ~Plugin();

	// returns display-name out of descriptor
	virtual QString displayName() const
	{
		return Model::displayName().isEmpty() ?  m_descriptor->displayName : Model::displayName();
	}

	// return plugin-type
	inline PluginTypes type( void ) const
	{
		return m_descriptor->type;
	}

	// return plugin-descriptor for further information
	inline const Descriptor* descriptor() const
	{
		return m_descriptor;
	}

	// can be called if a file matching supportedFileTypes should be
	// loaded/processed with the help of this plugin
	virtual void loadFile( const QString& file );

	// Called if external source needs to change something but we cannot
	// reference the class header.  Should return null if not key not found.
	virtual AutomatableModel* childModel( const QString& modelName );

	// returns an instance of a plugin whose name matches to given one
	// if specified plugin couldn't be loaded, it creates a dummy-plugin
	static Plugin * instantiate( const QString& pluginName, Model * parent, void * data );

	// fills given list with descriptors of all available plugins
	static void getDescriptorsOfAvailPlugins( DescriptorList& pluginDescriptors );

	// create a view for the model 
	PluginView * createView( QWidget* parent );


protected:
	// create a view for the model 
	virtual PluginView* instantiateView( QWidget* ) = 0;


private:
	const Descriptor* m_descriptor;

	// pointer to instantiation-function in plugin
	typedef Plugin * ( * InstantiationHook )( Model*, void* );

} ;


#endif
