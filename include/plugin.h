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

#include "journalling_object.h"
#include "mv_base.h"
#include "base64.h"



class QWidget;

class pixmapLoader;
class pluginView;


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
		class EXPORT subPluginFeatures
		{
		public:
			struct key
			{
				inline key( plugin::descriptor * _desc = NULL,
					const QString & _name = QString::null,
					const QVariant & _user = QVariant() )
					:
					desc( _desc ),
					name( _name ),
					user( _user )
				{
				}

				inline key( const QString & _dump_data )	
					:
					desc( NULL )
				{
					const QList<QVariant> l =
						base64::decode( _dump_data,
							QVariant::List ).
								toList();
					if( l.empty() )
					{
						name = QString::null;
						user = QVariant();
					}
					else
					{
						name = l[0].toString();
						user = l[1];
					}
				}

				inline QString dumpBase64( void ) const
				{
					return( base64::encode(
						QList<QVariant>()
							<< name << user ) );
				}

				inline bool isValid( void ) const
				{
					return( desc != NULL &&
							name != QString::null );
				}

				plugin::descriptor * desc;
				QString name;
				QVariant user;
			};

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

			virtual const QStringList & supportedExtensions( void )
			{
				static QStringList no_extensions;
				return( no_extensions );
			}


		protected:
			const plugin::PluginTypes m_type;
		}
			* sub_plugin_features;

	} ;

	// contructor of a plugin
	plugin( const descriptor * _descriptor, model * _parent );
	virtual ~plugin();

	// returns display-name out of descriptor
	virtual QString displayName( void ) const
	{
		return( model::displayName() != QString::null ?
					model::displayName() :
						m_descriptor->displayName );
	}

	// return plugin-type
	inline PluginTypes type( void ) const
	{
		return( m_descriptor->type );
	}

	// return plugin-descriptor for further information
	inline const descriptor * getDescriptor( void ) const
	{
		return( m_descriptor );
	}

	// plugins can overload this for making other classes able to change
	// settings of the plugin without knowing the actual class
	virtual void setParameter( const QString & _param,
						const QString & _value );

	// plugins can overload this for making other classes able to query
	// settings of the plugin without knowing the actual class
	virtual QString getParameter( const QString & _param );


	// returns an instance of a plugin whose name matches to given one
	// if specified plugin couldn't be loaded, it creates a dummy-plugin
	static plugin * instantiate( const QString & _plugin_name,
							model * _parent,
							void * _data );

	// some plugins run external programs for doing their actual work
	// (e.g. LVSL-server) or can run in separate worker-threads, so the
	// mixer can schedule processing for parallelizing work which is very
	// important for at least trying to use the full power of SMP-systems,
	// otherwise the mixer will create according threads on it's own which
	// of course isn't that efficient
	virtual bool supportsParallelizing( void ) const;

	// plugins supporting parallelization, should re-implement that as the
	// mixer will call this at the end of processing according chain
	// of plugins
	virtual void waitForWorkerThread( void );

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
