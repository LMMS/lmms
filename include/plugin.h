/*
 * plugin.h - class plugin, the base-class and generic interface for all plugins
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


#ifndef _PLUGIN_H
#define _PLUGIN_H


#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVector>
#include <QtXml/QDomDocument>

#include "journalling_object.h"
#include "mv_base.h"
#include "base64.h"



class QWidget;

class pixmapLoader;
class pluginView;
class automatableModel;


class EXPORT plugin : public journallingObject, public model
{
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
	struct descriptor
	{
		const char * name;
		const char * displayName;
		const char * description;
		const char * author;
		int version;
		PluginTypes type;
		const pixmapLoader * logo;
		const char * supportedFileTypes;
		inline bool supportsFileType( const QString & _ext ) const
		{
			return QString( supportedFileTypes ).
						split( QChar( ',' ) ).
							contains( _ext );
		}
		class EXPORT subPluginFeatures
		{
		public:
			struct key
			{
				typedef QMap<QString, QString> attributeMap;

				inline key( plugin::descriptor * _desc = NULL,
					const QString & _name = QString(),
					const attributeMap & _am =
							attributeMap() )
					:
					desc( _desc ),
					name( _name ),
					attributes( _am )
				{
				}

				key( const QDomElement & _key );

				QDomElement saveXML( QDomDocument &
								_doc ) const;

				inline bool isValid( void ) const
				{
					return desc != NULL &&
							name != QString::null;
				}

				plugin::descriptor * desc;
				QString name;
				attributeMap attributes;
			} ;

			typedef QList<key> keyList;


			subPluginFeatures( plugin::PluginTypes _type ) :
				m_type( _type )
			{
			}

			virtual ~subPluginFeatures()
			{
			}

			virtual void fillDescriptionWidget( QWidget *,
								const key * )
			{
			}

			virtual void listSubPluginKeys( plugin::descriptor *,
								keyList & )
			{
			}


		protected:
			const plugin::PluginTypes m_type;
		} ;

		subPluginFeatures * sub_plugin_features;

	} ;

	// contructor of a plugin
	plugin( const descriptor * _descriptor, model * _parent );
	virtual ~plugin();

	// returns display-name out of descriptor
	virtual QString displayName( void ) const
	{
		return model::displayName().isNull() ?
						m_descriptor->displayName :
							model::displayName();
	}

	// return plugin-type
	inline PluginTypes type( void ) const
	{
		return m_descriptor->type;
	}

	// return plugin-descriptor for further information
	inline const descriptor * getDescriptor( void ) const
	{
		return m_descriptor;
	}

	// can be called if a file matching supportedFileTypes should be
	// loaded/processed with the help of this plugin
	virtual void loadFile( const QString & _file );

	// Called if external source needs to change something but we cannot
	// reference the class header.  Should return null if not key not found.
	virtual automatableModel * getChildModel( const QString & _modelName );

	// returns an instance of a plugin whose name matches to given one
	// if specified plugin couldn't be loaded, it creates a dummy-plugin
	static plugin * instantiate( const QString & _plugin_name,
							model * _parent,
							void * _data );

	// fills given vector with descriptors of all available plugins
	static void getDescriptorsOfAvailPlugins(
					QVector<descriptor> & _plugin_descs );

	// create a view for the model 
	pluginView * createView( QWidget * _parent );


protected:
	// create a view for the model 
	virtual pluginView * instantiateView( QWidget * ) = 0;


private:
	const descriptor * m_descriptor;

	// pointer to instantiation-function in plugin
	typedef plugin * ( * instantiationHook )( model *, void * );

} ;


#endif
