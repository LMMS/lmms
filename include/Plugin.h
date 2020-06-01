/*
 * Plugin.h - class plugin, the base-class and generic interface for all plugins
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

#ifndef PLUGIN_H
#define PLUGIN_H

#include <QtCore/QStringList>
#include <QtCore/QMap>
#include <QtXml/QDomDocument>

#include "JournallingObject.h"
#include "Model.h"
#include "MemoryManager.h"


class QWidget;

class PixmapLoader;
class PluginView;
class AutomatableModel;

/**
	Abstract representation of a plugin

	Such a plugin can be an Instrument, Effect, Tool plugin etc.

	Plugins have descriptors, containing meta info, which is used especially
	by PluginFactory and friends.

	There are also Plugin keys (class Key, confusingly under
	SubPluginFeatures), which contain pointers to the plugin descriptor.

	Some plugins have sub plugins, e.g. there is one CALF Plugin and for
	each CALF effect, there is a CALF sub plugin. For those plugins, there
	are keys for each sub plugin. These keys also link to the superior
	Plugin::Descriptor. Additionally, they contain attributes that help the
	superior Plugin saving them and recognizing them when loading.

	In case of sub plugins, the Descriptor has SubPluginFeatures. Those
	are a bit like values to the sub plugins' keys (in terms of a key-value-
	map).
*/
class LMMS_EXPORT Plugin : public Model, public JournallingObject
{
	MM_OPERATORS
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

	//! Descriptor holds information about a plugin - every external plugin
	//! has to instantiate such a Descriptor in an extern "C"-section so that
	//! the plugin-loader is able to access information about the plugin
	struct Descriptor
	{
		const char * name;
		const char * displayName;
		const char * description;
		const char * author;
		int version;
		PluginTypes type;
		const PixmapLoader * logo;
		const char * supportedFileTypes; //!< csv list of extensions

		inline bool supportsFileType( const QString& extension ) const
		{
			return QString( supportedFileTypes ).split( QChar( ',' ) ).contains( extension );
		}

		/**
			Access to non-key-data of a sub plugin

			If you consider sub plugin keys as keys in a
			key-value-map, this is the lookup for the corresponding
			values. In order to have flexibility between different
			plugin APIs, this is rather an array of fixed data,
			but a bunch of virtual functions taking the key and
			returning some values (or modifying objects of other
			classes).
		 */
		class LMMS_EXPORT SubPluginFeatures
		{
		public:
			/**
				Key reference a Plugin::Descriptor, and,
				if the plugin has sub plugins, also reference
				its sub plugin (using the attributes).
				When keys are saved, those attributes are
				written to XML in order to find the right sub
				plugin when realoading.

				@note Any data that is not required to reference
					the right Plugin or sub plugin should
					not be here (but rather in
					SubPluginFeatures, which are like values
					in a key-value map).
			*/
			struct Key
			{
				typedef QMap<QString, QString> AttributeMap;

				inline Key( const Plugin::Descriptor * desc = NULL,
						const QString & name = QString(),
						const AttributeMap & am = AttributeMap()
					)
					:
					desc( desc ),
					name( name ),
					attributes( am )
				{
				}

				Key( const QDomElement & key );

				QDomElement saveXML( QDomDocument & doc ) const;

				inline bool isValid() const
				{
					return desc != nullptr;
				}

				//! Key to subplugin: reference to parent descriptor
				//! Key to plugin: reference to its descriptor
				const Plugin::Descriptor* desc;
				//! Descriptive name like "Calf Phaser".
				//! Not required for key lookup and not saved
				//! only used sometimes to temporary store descriptive names
				//! @todo This is a bug, there should be a function
				//!   in SubPluginFeatures (to get the name) instead
				QString name;
				//! Attributes that make up the key and identify
				//! the sub plugin. They are being loaded and saved
				AttributeMap attributes;

				// helper functions to retrieve data that is
				// not part of the key, but mapped via desc->subPluginFeatures
				QString additionalFileExtensions() const;
				QString displayName() const;
				QString description() const;
				const PixmapLoader* logo() const;
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

			//! While PluginFactory only collects the plugins,
			//! this function is used by widgets like EffectSelectDialog
			//! to find all possible sub plugins
			virtual void listSubPluginKeys( const Plugin::Descriptor *, KeyList & ) const
			{
			}


		private:
			// You can add values mapped by "Key" below
			// The defaults are sane, i.e. redirect to sub plugin's
			// supererior descriptor

			virtual QString additionalFileExtensions(const Key&) const
			{
				return QString();
			}

			virtual QString displayName(const Key& k) const
			{
				return k.isValid() ? k.name : QString();
			}

			virtual QString description(const Key& k) const
			{
				return k.isValid() ? k.desc->description : QString();
			}

			virtual const PixmapLoader* logo(const Key& k) const
			{
				Q_ASSERT(k.desc);
				return k.desc->logo;
			}

		protected:
			const Plugin::PluginTypes m_type;
		} ;

		SubPluginFeatures * subPluginFeatures;

	} ;
	// typedef a list so we can easily work with list of plugin descriptors
	typedef QList<Descriptor*> DescriptorList;

	//! Constructor of a plugin
	//! @param key Sub plugins must pass a key here, optional otherwise.
	//!   See the key() function
	Plugin(const Descriptor * descriptor, Model * parent,
		const Descriptor::SubPluginFeatures::Key *key = nullptr);
	virtual ~Plugin();

	//! Return display-name out of sub plugin or descriptor
	QString displayName() const override;

	//! Return logo out of sub plugin or descriptor
	const PixmapLoader *logo() const;

	//! Return plugin type
	inline PluginTypes type( void ) const
	{
		return m_descriptor->type;
	}

	//! Return plugin Descriptor
	inline const Descriptor * descriptor() const
	{
		return m_descriptor;
	}

	//! Return the key referencing this plugin. If the Plugin has no
	//! sub plugin features, the key is pretty useless. If it has,
	//! this key will also contain the sub plugin attributes, and will be
	//! a key to those SubPluginFeatures.
	inline const Descriptor::SubPluginFeatures::Key & key() const
	{
		return m_key;
	}

	//! Can be called if a file matching supportedFileTypes should be
	//! loaded/processed with the help of this plugin
	virtual void loadFile( const QString & file );

	//! Called if external source needs to change something but we cannot
	//! reference the class header.  Should return null if not key not found.
	virtual AutomatableModel* childModel( const QString & modelName );

	//! Overload if the argument passed to the plugin is a subPluginKey
	//! If you can not pass the key and are aware that it's stored in
	//! Engine::pickDndPluginKey(), use this function, too
	static Plugin * instantiateWithKey(const QString& pluginName, Model * parent,
					const Descriptor::SubPluginFeatures::Key *key,
					bool keyFromDnd = false);

	//! Return an instance of a plugin whose name matches to given one
	//! if specified plugin couldn't be loaded, it creates a dummy-plugin
	//! @param data Anything the plugin expects. If this is a pointer to a sub plugin key,
	//!   use instantiateWithKey instead
	static Plugin * instantiate(const QString& pluginName, Model * parent, void *data);

	//! Create a view for the model
	PluginView * createView( QWidget * parent );

protected:
	//! Create a view for the model
	virtual PluginView* instantiateView( QWidget * ) = 0;
	void collectErrorForUI( QString errMsg );


private:
	const Descriptor * m_descriptor;

	Descriptor::SubPluginFeatures::Key m_key;

	// pointer to instantiation-function in plugin
	typedef Plugin * ( * InstantiationHook )( Model * , void * );

} ;


#endif
