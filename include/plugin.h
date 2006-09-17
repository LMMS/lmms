/*
 * plugin.h - class plugin, the base-class and generic interface for all plugins
 *
 * Copyright (c) 2005-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "qt3support.h"

#ifdef QT4

#include <QtCore/QString>
#include <QtCore/QVector>

#else

#include <qstring.h>
#include <qvaluevector.h>

#endif


#include "types.h"
#include "journalling_object.h"
#include "embed.h"
#include "base64.h"


#define STRINGIFY_PLUGIN_NAME(s) STR(s)
#define STR(PN)	#PN


class QPixmap;
class QWidget;


class plugin : public journallingObject
{
public:
	enum pluginTypes
	{
		Instrument,	// instrument being used in channel-track
		Effect,		// effect-plugin for effect-board
		ImportFilter,	// filter for importing a file
		ExportFilter,	// filter for exporting a file
		AnalysisTools,	// analysis-tools (level-meter etc)
		Other,
		Undefined = 255
	} ;

	// descriptor holds information about a plugin - every external plugin
	// has to instantiate such a descriptor in an extern "C"-section so that
	// the plugin-loader is able to access information about the plugin
	struct descriptor
	{
		const char * name;
		const char * public_name;
		const char * description;
		const char * author;
		int version;
		pluginTypes type;
		const QPixmap * logo;
		class subPluginFeatures
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
					const vlist<QVariant> l =
						base64::decode( _dump_data ).
								toList();
					name = l[0].toString();
					user = l[1];
				}
				inline QString dumpBase64( void ) const
				{
					return( base64::encode(
						vlist<QVariant>()
							<< name << user ) );
				}
				plugin::descriptor * desc;
				QString name;
				QVariant user;
			};
			typedef vlist<key> keyList;

			subPluginFeatures( plugin::pluginTypes _type ) :
				m_type( _type )
			{
			}

			virtual ~subPluginFeatures()
			{
			}

			virtual QWidget * createDescriptionWidget(
					QWidget *, engine *, const key & )
			{
				return( NULL );
			}
			virtual void listSubPluginKeys( engine *,
							plugin::descriptor *,
							keyList & )
			{
			}

		protected:
			const plugin::pluginTypes m_type;
		}
			* sub_plugin_features;

	} ;

	// contructor of a plugin
	plugin( const descriptor * _descriptor, engine * _engine );
	virtual ~plugin();

	// returns public-name out of descriptor
	virtual inline QString publicName( void ) const
	{
		return( m_descriptor->public_name );
	}

	// return plugin-type
	inline pluginTypes type( void ) const
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
	virtual void FASTCALL setParameter( const QString & _param,
						const QString & _value );

	// plugins can overload this for making other classes able to query
	// settings of the plugin without knowing the actual class
	virtual QString FASTCALL getParameter( const QString & _param );


	// returns an instance of a plugin whose name matches to given one
	// if specified plugin couldn't be loaded, it creates a dummy-plugin
	static plugin * FASTCALL instantiate( const QString & _plugin_name,
							void * _data );

	// fills given vector with descriptors for all available plugins
	static void FASTCALL getDescriptorsOfAvailPlugins(
					vvector<descriptor> & _plugin_descs );

private:
	const descriptor * m_descriptor;

	// pointer to instantiation-function in plugin
	typedef plugin * ( * instantiationHook )( void * );

} ;


#endif
